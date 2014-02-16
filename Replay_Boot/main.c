/** @file main.c */

#include "board.h"
#include "swi.h"
#include "hardware.h"
#include "card.h"
#include "fpga.h"
#include "twi.h"
#include "fullfat.h"

#include "amiga_fdd.h"
#include "amiga_hdd.h"
#include "iniparser.h"
#include "config.h"
#include "menu.h"
#include "osd.h"
#include "messaging.h"

// GLOBALS
FF_IOMAN *pIoman = NULL;  // file system handle
const char version[] = {__BUILDNUMBER__}; // actual build version

int main(void)
{
  // used by file system
  uint8_t fatBuf[FS_FATBUF_SIZE];

  // initialise
  Hardware_Init(); // Initialise board hardware
  IO_DriveLow_OD(PIN_FPGA_RST_L); // make sure FPGA is held in reset
  ACTLED_ON;

  USART_Init(115200); // Initialize debug USART
  init_printf(NULL, USART_Putc); // Initialise printf

  Timer_Init();

  // replay main status structure
  status_t current_status;
  memset((void *)&current_status,0,sizeof(status_t));
  strcpy(current_status.ini_dir,"\\");
  strcpy(current_status.act_dir,"\\");
  strcpy(current_status.ini_file,"replay.ini");
  strcpy(current_status.bin_file,"replay.bin");

  // setup message structure
  MSG_init(&current_status,1);

  // directory scan structure
  tDirScan dir_status;
  current_status.dir_scan = &dir_status;
  memset((void *)&dir_status,0,sizeof(tDirScan));
  // end of variables

  // INIT
  DEBUG(1,"\033[2J");

  SPI_Init();
  SSC_Configure_Boot();
  TWI_Configure(); // requires timer init
  //
  ACTLED_OFF;
  //
  DEBUG(1,"");
  DEBUG(0,"== FPGAArcade Replay Board ==");
  DEBUG(0,"Mike Johnson & Wolfgang Scherr");
  DEBUG(1,"");
  MSG_warning("NON-RELEASED BETA VERSION");
  DEBUG(1,"");
  DEBUG(1,"ARM Firmware: %s",version);
  DEBUG(1,"");

  DEBUG(1,"Built upon work by Dennis van Weeren & Jakub Bednarski");
  MSG_info("Using %s",FF_REVISION);
  DEBUG(1,"               by James Walmsley");
  DEBUG(1,"");
  DEBUG(1,"");

  //MSG_warning("A test warning.");
  //MSG_error("A test error.");

  // create file system and set up I/O
  pIoman = FF_CreateIOMAN(fatBuf, FS_FATBUF_SIZE, 512, NULL);

  // register file system handlers
  FF_RegisterBlkDevice(pIoman, 512,(FF_WRITE_BLOCKS) Card_WriteM, (FF_READ_BLOCKS) Card_ReadM, NULL);

  DEBUG(1,"");

  FDD_Init(); // here?

  // Loop forever
  while (TRUE) {
    char full_filename[FF_MAX_PATH];

    // read inputs
    CFG_update_status(&current_status);
    CFG_print_status(&current_status);

    // note, this will init filescan, but INI file not read yet
    CFG_card_start(&current_status);

    // we require an functional sdcard filesystem to continue
    if (current_status.fs_mounted_ok) {
      sprintf(full_filename,"%s%s",current_status.ini_dir,current_status.ini_file);

      // free any backup stuff
      CFG_free_bak(&current_status);

      // pre FPGA load ini file parse: FPGA bin, post ini, clocking, coder, video filter
      CFG_pre_init(&current_status, full_filename);

      // load FPGA if a configuration file is given and either it is not set or a reload is requested
      if ( (!IO_Input_H(PIN_FPGA_DONE)) && strlen(current_status.bin_file) ) {
        int32_t status;

        // make it "absolute"
        sprintf(full_filename, "%s%s",current_status.ini_dir,current_status.bin_file);

        current_status.fpga_load_ok = 0;
        status = CFG_configure_fpga(full_filename);
        if (status) {
          CFG_fatal_error(status);  // flash we fail to configure FPGA
          current_status.fpga_load_ok = 0;
        } else {
          DEBUG(1,"FPGA CONFIG \"%s\" done.",full_filename);
          current_status.fpga_load_ok = 1;
        }
      }
    } else {
      // set up some default to have OSD enabled
      if (!IO_Input_H(PIN_FPGA_DONE)) {
        // initially configure default clocks, video filter and diable video coder (may not be fitted)
        CFG_vid_timing_HD27(F60HZ);
        CFG_set_coder(CODER_DISABLE);
        if (FPGA_Default()) {
          DEBUG(1,"FPGA default set.");
          current_status.fpga_load_ok=2;
          current_status.twi_enabled=1;
          current_status.spi_fpga_enabled=1;
          current_status.spi_osd_enabled=1;
          sprintf(current_status.status[0], "ARM |FW:%s (%ldkB free)", version,
                                              CFG_get_free_mem()>>10);
          sprintf(current_status.status[1], "FPGA|NO VALID SETUP ON SDCARD!");
        }
      }
    }

    if ((current_status.fpga_load_ok) || (IO_Input_H(PIN_FPGA_DONE))) {
      sprintf(full_filename,"%s%s",current_status.ini_dir,current_status.ini_file);

      if (!current_status.fpga_load_ok) {
        DEBUG(1,"FPGA has been configured by debugger.");
        current_status.fpga_load_ok = 1;
      }

      // post load set up, release reset and wait for FPGA to settle first
      IO_DriveHigh_OD(PIN_FPGA_RST_L);
      Timer_Wait(200);

      uint32_t spiFreq = BOARD_MCK /
                         ((AT91C_BASE_SPI->SPI_CSR[0] & AT91C_SPI_SCBR) >> 8) /
                         1000000;
      DEBUG(0,"SPI clock: %d MHz", spiFreq);

      if (current_status.fpga_load_ok!=2) {
        // we free the memory of a previous setup
        DEBUG(1,"--------------------------");
        DEBUG(1,"CLEANUP (%ld bytes free)",CFG_get_free_mem());
        CFG_free_menu(&current_status);

        // initialize root entry properly, it is the seed of this menu tree
        DEBUG(1,"--------------------------");
        DEBUG(1,"PRE-INIT (%ld bytes free)",CFG_get_free_mem());

        // post FPGA load ini file parse: video DAC, ROM files, etc.
        if (CFG_init(&current_status, full_filename)) {
          CFG_free_menu(&current_status);
          CFG_free_bak(&current_status);
        }
        CFG_add_default(&current_status);

        if (current_status.menu_top) {
          //
          DEBUG(1,"--------------------------");
          DEBUG(1,"POSTINIT (%ld bytes free)",CFG_get_free_mem());
        }
      } else {
          if (OSD_ConfigReadSysconVer() != 0xA5) {
            ERROR("FPGA Syscon not detected !!");
            // need to disable all OSD access
          }
          uint32_t config_ver    = OSD_ConfigReadVer();
          DEBUG(1,"FPGA ver: 0x%08x",config_ver);
          // setup DRAM, DAC, etc.
          OSD_ConfigSendCtrl((kDRAM_SEL << 8) | kDRAM_PHASE); // default phase
          OSD_Reset(OSDCMD_CTRL_RES|OSDCMD_CTRL_HALT);
          Timer_Wait(100);
          if (config_ver & 0x8000) {
            FPGA_DramTrain();
          }
          const vidconfig_t vid_config = { 0x00,0x48,0xC0,0x80,0x00,0x01,0x00,0x80,0x08,0x16,0x30,0x60,0x00,0x18,0xC0,0x00 };
          Configure_CH7301(&vid_config);
          //CFG_set_CH7301_HD();  --> does not work ???
          // dynamic/static setup bits
          OSD_ConfigSendUserD(0);
          OSD_ConfigSendUserS(0);
          OSD_Reset(OSDCMD_CTRL_RES);
          WARNING("Using hardcoded fallback!");
      }

      // we do a final update of MENU / settings and let the core run
      // afterwards, we show the generic status menu
      MENU_init_ui(&current_status);
      OSD_Reset(0);
      Timer_Wait(100);
      current_status.show_status=1;
      current_status.update=1;
      OSD_Enable(DISABLE_KEYBOARD);

      // we run in here as long as there is no need to reload the FPGA
      while (current_status.fpga_load_ok) {
        uint16_t key;

        if (current_status.spi_osd_enabled) {
          // get user control codes
          key = OSD_GetKeyCode();

          USART_update();

          if (MENU_handle_ui(key,&current_status)) {
            // do further update stuff here...
          }
          // this key starts the bootloader - TODO: find better key or remove again...
          if (key == KEY_RESET) CFG_call_bootloader();
          // this key restarts the core only
          if (key == KEY_REST) {
            // perform soft-reset
            OSD_Reset(OSDCMD_CTRL_RES);
            Timer_Wait(1);
            // we should not need this, but just in case...
            OSD_Reset(OSDCMD_CTRL);
            Timer_Wait(100);
          }
        }
        else {
          if (IO_Input_L(PIN_MENU_BUTTON)) {
            key=KEY_MENU;
          } else {
            key=0;
          }
        }

        if (key == KEY_MENU) {
          if (current_status.button==BUTTON_RESET) {
            strcpy(current_status.ini_dir,"\\");
            strcpy(current_status.act_dir,"\\");
            strcpy(current_status.ini_file,"replay.ini");
            IO_DriveLow_OD(PIN_FPGA_RST_L); // make sure FPGA is held in reset
            ACTLED_ON;
            // set PROG low to reset FPGA (open drain)
            IO_DriveLow_OD(PIN_FPGA_PROG_L);
            Timer_Wait(1);
            IO_DriveHigh_OD(PIN_FPGA_PROG_L);
            Timer_Wait(2);
            // invalidate FPGA configuration here as well
            current_status.fpga_load_ok = 0;
          }
        }

        CFG_update_status(&current_status);
        CFG_card_start(&current_status); // restart file system if card re-inserted

        // we deconfigured externally!
        if ((!IO_Input_H(PIN_FPGA_DONE)) && (current_status.fpga_load_ok)) {
          MSG_warning("FPGA has been deconfigured.");
          // assume this is the programmer and wait for it to be reconfigured
          while (!IO_Input_H(PIN_FPGA_DONE)) {
            MSG_warning("    waiting for reconfig....");
            Timer_Wait(1000);
          }
          OSD_ConfigSendCtrl((kDRAM_SEL << 8) | kDRAM_PHASE); // default phase
          FPGA_DramTrain();
          break;
        }
        // we check if we are in fallback mode and a proper sdcard is available now
        if ((current_status.fpga_load_ok==2)&&(current_status.fs_mounted_ok)) {
          IO_DriveLow_OD(PIN_FPGA_RST_L); // make sure FPGA is held in reset
          ACTLED_ON;
          // set PROG low to reset FPGA (open drain)
          IO_DriveLow_OD(PIN_FPGA_PROG_L);
          Timer_Wait(1);
          IO_DriveHigh_OD(PIN_FPGA_PROG_L);
          Timer_Wait(2);
          // invalidate FPGA configuration here as well
          current_status.fpga_load_ok = 0;
        }

        CFG_handle_fpga();
      }
    }

    // we try again!
    IO_DriveLow_OD(PIN_FPGA_RST_L); // reset FPGA

    // loop again
    AT91C_BASE_PIOA->PIO_SODR = PIN_ACT_LED;
    Timer_Wait(500);
    AT91C_BASE_PIOA->PIO_CODR = PIN_ACT_LED;
    Timer_Wait(500);
  }
  return 0; /* never reached */
}
