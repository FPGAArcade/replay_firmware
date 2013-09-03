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

const char version[] = {"24July13_r2"}; // note /0 added automatically

// GLOBALS
uint8_t  fatBuf[FS_FATBUF_SIZE]; // used by file system
uint8_t  fileBuf[FS_FILEBUF_SIZE];
uint8_t  *pFileBuf = NULL;
FF_IOMAN *pIoman = NULL;

int main(void)
{
// some hardcoded profiles

  // PLL CONFIGURATION
  // =================
  //
  // Can be set independent of FPGA
  //
  // input clock = 27MHz
  //
  // output = (27 * n / m) / p
  //
  // Examples:
  //
  // PAL  17.73447                M= 46 N= 423 Fvco=248     p=14 output = 17.73447
  // PAL  28.37516 x 2 = 56.75032 M=481 N=4044 Fvco=227     p=4  output = 56.75052 error = 3.5ppm
  // NTSC 14.31818                M= 33 N= 280 Fvco=229     p=16 output = 14.31818
  // NTSC 28.63636 x 2 = 57.27272 M= 33 N= 280 Fvco=229     p=4  output = 57.27272
  //                              M=375 n=2048 Fvco=147.456 p=12 output = 12.288MHz
  //
  // px sel 0 = bypass(input clock) 1 = pll1 2 = pll2 4 = pll3
  // dx div (divider) 0-127 divider value
  // yx sel 1 = on, 0 = off
  //
  // y0 - FPGA DRAM
  // y1 - Coder
  // y2 - FPGA aux/sys clk
  // y3 - Expansion Main
  // y4 - FPGA video
  // y5 - Expansion Small
  //


  // initialise
  Hardware_Init(); // Initialise board hardware
  init_printf(NULL, USART_Putc); // Initialise printf

  ACTLED_ON;

  // replay main status structure
  status_t current_status;
  memset((void *)&current_status,0,sizeof(status_t));
  strcpy(current_status.ini_dir,"\\");
  strcpy(current_status.ini_file,"replay.ini");
  strcpy(current_status.bin_file,"replay.bin");

  // directory scan structure
  tDirScan dir_status;
  current_status.dir_scan = &dir_status;
  memset((void *)&dir_status,0,sizeof(tDirScan));
  // end of variables

  // INIT
  Timer_Init();
  SPI_Init();
  IO_DriveLow_OD(PIN_FPGA_RST_L); // make sure FPGA is held in reset
  SSC_Configure_Boot();
  TWI_Configure();
  USART_Init(115200); // Initialize debug USART
  ACTLED_OFF;

  DEBUG(0,"\033[2J");
  DEBUG(0,"");
  DEBUG(0," == FPGAArcade Replay Board ==");
  DEBUG(0," Mike Johnson & Wolfgang Scherr");
  DEBUG(0,"");
  MSG_warning(&current_status, "NON-RELEASED BETA VERSION");
  DEBUG(0,"");
  DEBUG(0,"ARM Firmware: %s",version);
  DEBUG(0,"");
  DEBUG(0,"Built upon work by Dennis van Weeren & Jakub Bednarski");
  MSG_info(&current_status, "Using %s",FF_REVISION);
  DEBUG(0,"               by James Walmsley");
  DEBUG(0,"");
  DEBUG(0,"");

  //MSG_warning(&current_status, "A test warning.");
  //MSG_error(&current_status, "A test error.");

  // create file system and set up I/O
  pFileBuf = (uint8_t *)fileBuf;
  pIoman = FF_CreateIOMAN(fatBuf, FS_FATBUF_SIZE, 512, NULL);

  // register file system handlers
  FF_RegisterBlkDevice(pIoman, 512,(FF_WRITE_BLOCKS) Card_WriteM, (FF_READ_BLOCKS) Card_ReadM, NULL);

  // initially configure default clocks, video filter and diable video coder (may not be fitted)
  CFG_vid_timing_HD27(F60HZ);
  CFG_set_coder(CODER_DISABLE);

  printf("\n\r");

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
          printf("FPGA CONFIG \"%s\" done.\n\r",full_filename);
          current_status.fpga_load_ok = 1;
        }
      }
    }

    if ((current_status.fpga_load_ok) || (IO_Input_H(PIN_FPGA_DONE))) {
      sprintf(full_filename,"%s%s",current_status.ini_dir,current_status.ini_file);

      if (!current_status.fpga_load_ok) {
        printf("FPGA has been configured by debugger.\n\r");
        current_status.fpga_load_ok = 1;
      }

      // post load set up, release reset and wait for FPGA to settle first
      IO_DriveHigh_OD(PIN_FPGA_RST_L);
      Timer_Wait(200);

      // configure some video DAC defaults, if we are allowed to
      if (current_status.twi_enabled) {
        /*Configure_CH7301(&default_vid_config);*/
        CFG_set_CH7301_SD();
      }

      // we free the memory of a previous setup
      if (current_status.menu_top) {
        DEBUG(0,"--------------------------\n\r");
        DEBUG(0,"CLEANUP (%ld bytes free)\n\r",CFG_get_free_mem());
        CFG_free_menu(&current_status);
      }

      // initialize root entry properly, it is the seed of this menu tree
      DEBUG(0,"--------------------------\n\r");
      DEBUG(0,"PRE-INIT (%ld bytes free)\n\r",CFG_get_free_mem());

      // post FPGA load ini file parse: video DAC, ROM files, etc.
      if (CFG_init(&current_status, full_filename)) {
        CFG_free_menu(&current_status);
//      }
//      else {
//        snprintf(current_status.info2,MENU_INFO_WIDTH," f:%s",full_filename);
      }

      CFG_add_default(&current_status);

      if (current_status.menu_top) {
        //
        DEBUG(0,"--------------------------\n\r");
        DEBUG(0,"POSTINIT (%ld bytes free)\n\r",CFG_get_free_mem());
      /* DETAILED DEBUG ONLY!!!
       *
       *  current_status.pMenuAct = current_status.pMenuTop;
       *  while (current_status.pMenuAct && current_status.pMenuAct->pItems) {
       *    DEBUG(2,"T:%s\n\r",current_status.pMenuAct->title);
       *    current_status.pItemAct = current_status.pMenuAct->pItems;
       *    while (current_status.pItemAct && current_status.pItemAct->pOptions) {
       *      DEBUG(2,"  I: %s %08lx %s\n\r",current_status.pItemAct->item,current_status.pItemAct->mask,current_status.pItemAct->dynamic?"(dynamic)":"(static)");
       *      current_status.pOptionAct = current_status.pItemAct->pOptions;
       *      while (current_status.pOptionAct && (current_status.pOptionAct->option[0])) {
       *        DEBUG(2,"    O: %s %08lx %s\n\r",current_status.pOptionAct->option,current_status.pOptionAct->value,current_status.pOptionAct==current_status.pItemAct->pOptSet?"(default)":"");
       *        current_status.pOptionAct = current_status.pOptionAct->pNext;
       *      }
       *      current_status.pItemAct = current_status.pItemAct->pNext;
       *    }
       *    current_status.pMenuAct = current_status.pMenuAct->pNext;
       *  }
       */
      }
      //snprintf(current_status.info1,MENU_INFO_WIDTH," v:%s, %ldkB free",version,CFG_get_free_mem()>>10);

      uint32_t spiFreq = BOARD_MCK /
                         ((AT91C_BASE_SPI->SPI_CSR[0] & AT91C_SPI_SCBR) >> 8) /
                         1000000;
      MSG_info(&current_status,"SPI clock: %d MHz", spiFreq);

      // we do a final update of MENU / settings and let the core run
      // afterwards, we show the generic status menu
      MENU_init_ui(&current_status);
      /*ConfigUpdate();*/
      /*OSD_ConfigSendFileIO(1); // one disk at startup*/
      OSD_Reset(0);
      Timer_Wait(100);
      current_status.show_status=1;
      current_status.update=1;
      OSD_Enable(1);

      // we run in here as long as there is no need to reload the FPGA
      while (current_status.fpga_load_ok) {
        uint16_t key;

        if (current_status.spi_osd_enabled) {
          // get user control codes
          key = OSD_GetKeyCode();

          //CFG_handle_fpga();
          if (MENU_handle_ui(key,&current_status)) {
            // do further update stuff here...
          }
          // this key starts the bootloader - TODO: find better key or remove again...
          if (key == KEY_F1) CFG_call_bootloader();
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
            strcpy(current_status.ini_file,"replay.ini");
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
          printf("FPGA has been deconfigured.\n\r");
          // assume this is the programmer and wait for it to be reconfigured
          while (!IO_Input_H(PIN_FPGA_DONE)) {
            printf("    waiting for reconfig....\n\r");
            Timer_Wait(1000);
          }
          OSD_ConfigSendCtrl((kDRAM_SEL << 8) | kDRAM_PHASE); // default phase
          FPGA_DramTrain();
          break;
        }
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
