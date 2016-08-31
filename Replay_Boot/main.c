/*
 WWW.FPGAArcade.COM

 REPLAY Retro Gaming Platform
 No Emulation No Compromise

 All rights reserved
 Mike Johnson Wolfgang Scherr

 SVN: $Id:

--------------------------------------------------------------------

 Redistribution and use in source and synthezised forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

 Redistributions in synthesized form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.

 Neither the name of the author nor the names of other contributors may
 be used to endorse or promote products derived from this software without
 specific prior written permission.

 THIS CODE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.

 You are responsible for any legal issues arising from your use of this code.

 The latest version of this file can be found at: www.FPGAArcade.com

 Email support@fpgaarcade.com
*/
/** @file main.c */

#include "board.h"
#include "swi.h"
#include "hardware.h"
#include "card.h"
#include "fpga.h"
#include "twi.h"
#include "fullfat.h"
#include "fileio.h"
#include "iniparser.h"
#include "config.h"
#include "menu.h"
#include "osd.h"
#include "messaging.h"

extern char _binary_buildnum_start;	// from ./buildnum.elf > buildnum && arm-none-eabi-objcopy -I binary -O elf32-littlearm -B arm buildnum buildnum.o

// GLOBALS
FF_IOMAN *pIoman = NULL;  // file system handle
const char* version = &_binary_buildnum_start; // actual build version

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
  CFG_set_status_defaults(&current_status, TRUE);

  // setup message structure
  MSG_init(&current_status,1);

  // directory scan structure
  tDirScan dir_status;
  current_status.dir_scan = &dir_status;
  memset((void *)&dir_status,0,sizeof(tDirScan));
  // end of variables

  SPI_Init();
  SSC_Configure_Boot();
  TWI_Configure(); // requires timer init
  //
  ACTLED_OFF;

  //
  //
  //
  /*
  CFG_vid_timing_HD27(F60HZ);
  if (FPGA_Default()) {
    // didn't work
    DEBUG(0,"FPGA inital boot failed.");
    MSG_fatal_error(1); // halt and reboot
  }
  IO_DriveHigh_OD(PIN_FPGA_RST_L);
  // end of critical fast boot
  CFG_set_coder(CODER_DISABLE);
  Timer_Wait(200);

  OSD_Reset(OSDCMD_CTRL_RES|OSDCMD_CTRL_HALT);
  CFG_set_CH7301_HD();
  // dynamic/static setup bits
  OSD_ConfigSendUserS(0x00000000);
  OSD_ConfigSendUserD(0x00000000); // 60HZ progressive

  DEBUG(0,"done");

  while (TRUE) {
    for (int i = 0; i < 3; i++) {
      ACTLED_ON;
      Timer_Wait(250);
      ACTLED_OFF;
      Timer_Wait(250);
    }
    Timer_Wait(1000);
  }
  */

  // to be sure
  IO_DriveHigh_OD(PIN_FPGA_RST_L);

  // INIT
  DEBUG(1,"\033[2J");
  //
  DEBUG(0,"");
  DEBUG(0,"== FPGAArcade Replay Board ==");
  DEBUG(0,"Mike Johnson & Wolfgang Scherr");
  DEBUG(0,"");
  MSG_warning("NON-RELEASED BETA VERSION");
  DEBUG(0,"");
  DEBUG(0,"ARM Firmware: %s",version);
  DEBUG(0,"");

  DEBUG(0,"Built upon work by Dennis van Weeren & Jakub Bednarski");
  MSG_info("Using %s by James Walmsley",FF_REVISION);
  DEBUG(0,"");

  //MSG_warning("A test warning.");
  //MSG_error("A test error.");

  // create file system and set up I/O
  pIoman = FF_CreateIOMAN(fatBuf, FS_FATBUF_SIZE, 512, NULL);

  // register file system handlers
  FF_RegisterBlkDevice(pIoman, 512,(FF_WRITE_BLOCKS) Card_WriteM, (FF_READ_BLOCKS) Card_ReadM, NULL);

  DEBUG(1,"");

  // start up virtual drives
  FileIO_FCh_Init();

  // Loop forever
  while (TRUE) {
    // eject all drives
    for (int i=0;i<4;++i) {
      if (FileIO_FCh_GetInserted(0,i)) FileIO_FCh_Eject(0,i);
      if (FileIO_FCh_GetInserted(1,i)) FileIO_FCh_Eject(1,i);
    }

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
      //CFG_free_bak(&current_status);

      // pre FPGA load ini file parse: FPGA bin, post ini, clocking, coder, video filter
      // will configure clocks/video buffer with default values if non in INI file

      CFG_pre_init(&current_status, full_filename); // check status - no clocks if error occurs

      // load FPGA if a configuration file is given and either it is not set or a reload is requested
      if ( (!IO_Input_H(PIN_FPGA_DONE)) && strlen(current_status.bin_file) ) {
        int32_t status;

        // make it "absolute"
        sprintf(full_filename, "%s%s",current_status.ini_dir,current_status.bin_file);

        current_status.fpga_load_ok = 0;
        status = CFG_configure_fpga(full_filename);
        if (status) {
          // will not get here if config fails, but just in case...
          MSG_fatal_error(status);  // flash we fail to configure FPGA
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
        // Add support for interlaced/codec standards (selectable by menu button?)
        CFG_vid_timing_HD27(F60HZ);
        CFG_set_coder(CODER_DISABLE);
        if (!FPGA_Default()) {
          DEBUG(1,"FPGA default set.");

          current_status.fpga_load_ok=2;
          current_status.twi_enabled=1;
          current_status.spi_fpga_enabled=1;
          current_status.spi_osd_enabled=1;

          sprintf(current_status.status[0], "ARM |FW:%s (%ldkB free)", version,
                                              CFG_get_free_mem()>>10);
          sprintf(current_status.status[1], "FPGA|NO VALID SETUP ON SDCARD!");
        } else {
          // didn't work
          MSG_fatal_error(1); // halt and reboot
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
          // THIS will set up DAC defaults if non found
          CFG_free_menu(&current_status);
          /*CFG_free_bak(&current_status);*/
        }
        CFG_add_default(&current_status);

        if (current_status.menu_top) {
          //
          DEBUG(1,"--------------------------");
          DEBUG(1,"POSTINIT (%ld bytes free)",CFG_get_free_mem());
        }
      } else {
        // fall back to baked in version to report error
        if (OSD_ConfigReadSysconVer() != 0xA5) {
          WARNING("FPGA Syscon not detected !!");
        }
        uint32_t config_ver    = OSD_ConfigReadVer();
        DEBUG(1,"FPGA ver: 0x%08x",config_ver);

        // NO DRAM in the embedded core
        OSD_Reset(OSDCMD_CTRL_RES|OSDCMD_CTRL_HALT);

        CFG_vid_timing_HD27(F60HZ);
        CFG_set_coder(CODER_DISABLE);
        CFG_set_CH7301_HD();

        // dynamic/static setup bits
        OSD_ConfigSendUserS(0x00000000);
        OSD_ConfigSendUserD(0x00000000); // 60HZ progressive

        OSD_Reset(OSDCMD_CTRL_RES);
        WARNING("Using hardcoded fallback!");
      }

      // we do a final update of MENU / settings and let the core run
      // afterwards, we show the generic status menu
      MENU_init_ui(&current_status); // must be called after core is running, but clears status
      OSD_Reset(0);
      Timer_Wait(100);

      if (current_status.osd_init == OSD_INIT_ON) {
        current_status.show_status=1;
        current_status.update=1;
        OSD_Enable(DISABLE_KEYBOARD);
      } else {
        current_status.show_status=0;
        current_status.show_menu=0;
        current_status.update=0;
      }

      // we run in here as long as there is no need to reload the FPGA
      while (current_status.fpga_load_ok) {
        // MAIN LOOP
        uint16_t key;

        // track memory usage, and detect heap/stack stomp
        if (2<=debuglevel)
        {
          static uint16_t loop = 0;
          if ((loop++) == 0)
            CFG_dump_mem_stats();
        }

        // get keys (from Replay button, RS232 or PS/2 via OSD/FPGA)
        key = OSD_GetKeyCode(current_status.spi_osd_enabled, current_status.hotkey);

        if (key && (key & KF_RELEASED) == 0) {
          DEBUG(3,"Key: 0x%04X - '%s'",key, OSD_GetStringFromKeyCode(key));
        } else if (key) {
          DEBUG(3,"Key: 0x%04X",key);
        }

        // check RS232
        USART_update();

        // check menu
        if (current_status.spi_osd_enabled) {
          if (MENU_handle_ui(key,&current_status)) {
            // do further update stuff here...
          }
          // this key restarts the core only
          if (key == (KEY_F11|KF_SHIFT)) {
            // perform soft-reset
            OSD_Reset(OSDCMD_CTRL_RES);
            Timer_Wait(1);
            // we should not need this, but just in case...
            OSD_Reset(OSDCMD_CTRL);
            Timer_Wait(100);
          }
        }

        // this key sequence starts the bootloader
        if ((key == KEY_RESET) || (key == (KEY_F12|KF_CTRL|KF_ALT))) {
          CFG_call_bootloader();
        }

        // this key or key sequence starts the bootloader
        if ((key == KEY_FLASH) || (key == (KEY_F11|KF_CTRL|KF_ALT))) {
          // set new INI file and force reload of FPGA configuration
          strcpy(current_status.ini_dir,"\\flash\\");
          strcpy(current_status.act_dir,"\\flash\\");
          strcpy(current_status.ini_file,"rApp.ini");
          // make sure FPGA is held in reset
          IO_DriveLow_OD(PIN_FPGA_RST_L);
          ACTLED_ON;
          // set PROG low to reset FPGA (open drain)
          IO_DriveLow_OD(PIN_FPGA_PROG_L);
          Timer_Wait(1);
          IO_DriveHigh_OD(PIN_FPGA_PROG_L);
          Timer_Wait(2);
          // invalidate FPGA configuration here as well
          current_status.fpga_load_ok = 0;
        }

        if (key == KEY_F12) {
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
          OSD_ConfigSendCtrl((kDRAM_SEL << 8) | kDRAM_PHASE, kCTRL_DRAM_MASK); // default phase
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

        // Handle virtual drives
        if (current_status.fileio_cha_ena !=0)
          FileIO_FCh_Process(0);
        if (current_status.fileio_chb_ena !=0)
          FileIO_FCh_Process(1);

        if (current_status.clockmon)
          FPGA_ClockMon(&current_status);

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
