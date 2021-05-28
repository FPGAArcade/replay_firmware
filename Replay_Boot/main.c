/*--------------------------------------------------------------------
 *                       Replay Firmware
 *                      www.fpgaarcade.com
 *                     All rights reserved.
 *
 *                     admin@fpgaarcade.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *--------------------------------------------------------------------
 *
 * Copyright (c) 2020, The FPGAArcade community (see AUTHORS.txt)
 *
 */

/** @file main.c */

#include "board.h"
#include "hardware.h"
#include "hardware/io.h"
#include "hardware/spi.h"
#include "hardware/ssc.h"
#include "hardware/twi.h"
#include "hardware/timer.h"
#include "hardware/usart.h"
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
#include <stdio.h>
#undef printf
#undef sprintf
#include "printf.h"
#include <unistd.h> // sbrk()

#include "usb/ptp_usb.h"
//#define PTP_USB 1

#include "replay.h"
#include "usb.h"
#include "tests/tests.h"

#ifndef HOSTED
#include <malloc.h>
#endif

#if defined(ARDUINO_SAMD_MKRVIDOR4000)
void NINA_Update();
#include "hardware_vidor/usbblaster.h"
#include <setjmp.h>
jmp_buf exit_env;
static __attribute__ ((noinline)) void FPGA_WriteGeneratedImage(uint32_t base);
#endif

extern char _binary_buildnum_start;     // from ./buildnum.elf > buildnum && arm-none-eabi-objcopy -I binary -O elf32-littlearm -B arm buildnum buildnum.o

extern char _binary_replay_ini_start;
extern char _binary_replay_ini_end;
extern char _binary_build_replayhand_start;
extern char _binary_build_replayhand_end;

static status_t current_status;
static void load_core_from_sdcard();
static void load_embedded_core();
static void init_core();
static void main_update();
static void prepare_sdcard();

// GLOBALS
FF_IOMAN* pIoman = NULL;  // file system handle

#if defined(ARDUINO)   // sketches already have a main()
int replay_main(void)
#else
int main(void)
#endif
{
    HARDWARE_TICK ts;
    // used by file system
    uint8_t fatBuf[FS_FATBUF_SIZE];

#if defined(ARDUINO_SAMD_MKRVIDOR4000)
    int exit = setjmp(exit_env);

    if (exit) {
        return exit;
    }

#endif

    // initialise
    Hardware_Init(); // Initialise board hardware
    IO_DriveLow_OD(PIN_FPGA_RST_L); // make sure FPGA is held in reset
    ACTLED_ON;

    USART_Init(115200); // Initialize debug USART
    init_printf(NULL, USART_Putc); // Initialise printf
    Timer_Init();

    ts = Timer_Get(0);

#if PTP_USB
    PTP_USB_Stop();
#endif

    // replay main status structure
    memset((void*)&current_status, 0, sizeof(status_t));
    CFG_set_status_defaults(&current_status, TRUE);

    // setup message structure
    MSG_init(&current_status, 1);

    // directory scan structure
    // this is kept private in menu.c for now..
    //  tDirScan dir_status;
    //  current_status.dir_scan = &dir_status;
    //  memset((void *)&dir_status,0,sizeof(tDirScan));
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

    /*
        while(1) {
            RunFullTestSuite();
            Timer_Wait(2000);
            return 0;
        }
    */

    // RunFullFatTests();


    // INIT
    DEBUG(1, "\033[2J");
    //
    DEBUG(0, "");
    DEBUG(0, "== FPGAArcade Replay Board ==");
    DEBUG(0, "Mike Johnson & Wolfgang Scherr");
    DEBUG(0, "");
    DEBUG(0, "ARM Firmware: %s", BUILD_VERSION);
#if defined(AT91SAM7S256)
    DEBUG(0, "ARM Firmware Size: %d bytes", *(uint32_t*)0x102024);
#endif
    DEBUG(0, "");

    DEBUG(0, "Built upon work by Dennis van Weeren & Jakub Bednarski");
#if !defined(FF_DEFINED)
    MSG_info("Using %s by James Walmsley", FF_REVISION);
#endif
    DEBUG(0, "");

    //MSG_warning("A test warning.");
    //MSG_error("A test error.");

    // create file system and set up I/O
    pIoman = FF_CreateIOMAN(fatBuf, FS_FATBUF_SIZE, 512, NULL);

    // register file system handlers
    FF_RegisterBlkDevice(pIoman, 512, (FF_WRITE_BLOCKS) Card_WriteM, (FF_READ_BLOCKS) Card_ReadM, NULL);

    DEBUG(1, "");

    // start up virtual drives
    FileIO_FCh_Init();

    DEBUG(0, "Firmware startup in %d ms", Timer_Convert(Timer_Get(0) - ts));
    ts = Timer_Get(0);

    // Loop forever
    while (TRUE) {
        // eject all drives
        for (int i = 0; i < 4; ++i) {
            if (FileIO_FCh_GetInserted(0, i)) {
                FileIO_FCh_Eject(0, i);
            }

            if (FileIO_FCh_GetInserted(1, i)) {
                FileIO_FCh_Eject(1, i);
            }
        }

        // at this point we must've free _all_ dynamically allocated memory!
        CFG_free_menu(&current_status);
#if !defined(HOSTED) && !defined(ARDUINO_SAMD_MKRVIDOR4000)
        Assert(!mallinfo().uordblks);
#endif

        // read inputs
        CFG_update_status(&current_status);
        CFG_print_status(&current_status);

        // note, this will init filescan, but INI file not read yet
        CFG_card_start(&current_status);

        // we need to handle this here because of the massive stack pressure..
        if (current_status.fs_mounted_ok && current_status.prepare_sdcard) {
            DEBUG(2, "time to prep the sdcard! ");
            current_status.prepare_sdcard = 0;
            prepare_sdcard();
        }

        // we require an functional sdcard filesystem to continue
        if (current_status.fs_mounted_ok) {
            load_core_from_sdcard();
        }

        // if we failed loading from the sdcard, fallback to the embedded core (make sure we didn't reconfigure through JTAG)
        if (current_status.fpga_load_ok == NO_CORE && !IO_Input_H(PIN_FPGA_DONE)) {
            load_embedded_core();
            // 3 reasons we end up here : no sdcard inserted, unable to mount sdcard, error loading core.
            // if the mounting was not successful (== no sdcard or bad format), retry mounting the card later
            current_status.card_detected = current_status.fs_mounted_ok;
        }

        if ((current_status.fpga_load_ok != NO_CORE) || (IO_Input_H(PIN_FPGA_DONE))) {
            init_core();

#if PTP_USB
            PTP_USB_Start();
#endif
#if defined(ARDUINO_SAMD_MKRVIDOR4000)
            USBBlaster_EnableAndInit();
#endif

            INFO("Configured in %d ms", Timer_Convert(Timer_Get(0) - ts));

            // we run in here as long as there is no need to reload the FPGA
            while (current_status.fpga_load_ok != NO_CORE) {
                main_update();
            }

#if defined(ARDUINO_SAMD_MKRVIDOR4000)
            USBBlaster_Disable();
#endif

            ts = Timer_Get(0);
        }

        // we try again!
        IO_DriveLow_OD(PIN_FPGA_RST_L); // reset FPGA
    }

    return 0; /* never reached */
}

static __attribute__ ((noinline)) void load_core_from_sdcard()
{
    HARDWARE_TICK ts = Timer_Get(0);
    char full_filename[FF_MAX_PATH];
    pathcat(full_filename, current_status.ini_dir, current_status.ini_file);

    // free any backup stuff
    //CFG_free_bak(&current_status);

    // pre FPGA load ini file parse: FPGA bin, post ini, clocking, coder, video filter
    // will configure clocks/video buffer with default values if non in INI file

    CFG_pre_init(&current_status, full_filename); // check status - no clocks if error occurs

    // load FPGA if a configuration file is given and either it is not set or a reload is requested
    if ( (!IO_Input_H(PIN_FPGA_DONE)) && strlen(current_status.bin_file) ) {
        int32_t status;

        // make it "absolute"
        pathcat(full_filename, current_status.ini_dir, current_status.bin_file);

        current_status.fpga_load_ok = NO_CORE;
        status = CFG_configure_fpga(full_filename);

        if (status) {
            // will not get here if config fails, but just in case...
            current_status.fpga_load_ok = NO_CORE;

        } else {
            DEBUG(1, "FPGA CONFIG \"%s\" done.", full_filename);
            current_status.fpga_load_ok = CORE_LOADED;
        }
    }

    DEBUG(0, "load_core_from_sdcard() took %d ms", Timer_Convert(Timer_Get(0) - ts));
}

static __attribute__ ((noinline)) void load_embedded_core()
{
#if defined(FPGA_DISABLE_EMBEDDED_CORE) && !defined(ARDUINO_SAMD_MKRVIDOR4000)
    MSG_fatal_error(9);
#else

    // set up some default to have OSD enabled
    //if (!IO_Input_H(PIN_FPGA_DONE)) {
    // initially configure default clocks, video filter and diable video coder (may not be fitted)
    // Add support for interlaced/codec standards (selectable by menu button?)
    CFG_vid_timing_HD27(F60HZ);
    CFG_set_coder(CODER_DISABLE);

    if (!FPGA_Default()) {
        DEBUG(1, "FPGA default set.");

        HARDWARE_TICK time = Timer_Get(0);
#if defined(ARDUINO_SAMD_MKRVIDOR4000)
        FPGA_WriteGeneratedImage(0x00400000);
#else
        FPGA_DecompressToDRAM(&_binary_build_replayhand_start, &_binary_build_replayhand_end - &_binary_build_replayhand_start, 0x00400000);
#endif
        time = Timer_Get(0) - time;
        DEBUG(0, "FPGA background image uploaded in %d ms.", Timer_Convert(time));

        current_status.fpga_load_ok = EMBEDDED_CORE;
        current_status.twi_enabled = 1;
        current_status.spi_fpga_enabled = 1;
        current_status.spi_osd_enabled = 1;

        snprintf(current_status.status[0], sizeof(current_status.status[0]), "ARM |FW:%s", BUILD_VERSION);
        sprintf(current_status.status[1], "FPGA|NO VALID SETUP ON SDCARD!");

    } else {
        // didn't work
        MSG_fatal_error(1); // halt and reboot
    }

    //}
#endif
}

static __attribute__ ((noinline)) void init_core()
{
    HARDWARE_TICK ts = Timer_Get(0);
    char full_filename[FF_MAX_PATH];
    pathcat(full_filename, current_status.ini_dir, current_status.ini_file);

    if (current_status.fpga_load_ok == NO_CORE) {
        DEBUG(1, "FPGA has been configured by debugger.");
        current_status.fpga_load_ok = CORE_LOADED;
    }

    // post load set up, release reset and wait for FPGA to settle first
    IO_DriveHigh_OD(PIN_FPGA_RST_L);
    Timer_Wait(200);

    uint32_t spiFreq = SPI_GetFreq();
    DEBUG(0, "SPI clock: %d MHz", spiFreq);

    if (current_status.fpga_load_ok != EMBEDDED_CORE) {
        // we free the memory of a previous setup
        DEBUG(1, "--------------------------");
        DEBUG(1, "CLEANUP (%ld bytes free)", CFG_get_free_mem());
        CFG_free_menu(&current_status);

        // initialize root entry properly, it is the seed of this menu tree
        DEBUG(1, "--------------------------");
        DEBUG(1, "PRE-INIT (%ld bytes free)", CFG_get_free_mem());

        // post FPGA load ini file parse: video DAC, ROM files, etc.
        if (CFG_init(&current_status, full_filename)) {
            // THIS will set up DAC defaults if non found
            //CFG_free_menu(&current_status);
            /*CFG_free_bak(&current_status);*/
        }

        CFG_add_default(&current_status);

        if (current_status.menu_top) {
            //
            DEBUG(1, "--------------------------");
            DEBUG(1, "POSTINIT (%ld bytes free)", CFG_get_free_mem());
        }

    } else {
        // fall back to baked in version to report error
        if (OSD_ConfigReadSysconVer() != 0xA5) {
            WARNING("FPGA Syscon not detected !!");
        }

        uint32_t config_ver    = OSD_ConfigReadVer();
        DEBUG(1, "FPGA ver: 0x%08x", config_ver);

        OSD_ConfigSendCtrl((kDRAM_SEL << 8) | kDRAM_PHASE); // default phase
        OSD_Reset(OSDCMD_CTRL_RES | OSDCMD_CTRL_HALT);
        Timer_Wait(100);
        FPGA_DramTrain();
        CFG_set_CH7301_SD();

        // dynamic/static setup bits
        OSD_ConfigSendUserS(0x00000000);
        OSD_ConfigSendUserD(0x00000000); // 60HZ progressive
        current_status.button = BUTTON_OFF;
#if !defined(FPGA_DISABLE_EMBEDDED_CORE) || defined(ARDUINO_SAMD_MKRVIDOR4000)
        int32_t status = ParseIniFromString(&_binary_replay_ini_start, &_binary_replay_ini_end - &_binary_replay_ini_start, _CFG_parse_handler, &current_status);

        if (status != 0 ) {
            ERROR("Error at INI line %d", status);
        }

#if defined(ARDUINO_SAMD_MKRVIDOR4000)
        current_status.config_d &= 0x62;    // kludge for VIDOR/full loader core (to disable unused bits)
#endif

        _MENU_update_bits(&current_status);
#endif
        OSD_Reset(OSDCMD_CTRL_RES);
        WARNING("Using hardcoded fallback!");
    }

    // we do a final update of MENU / settings and let the core run
    // afterwards, we show the generic status menu
    MENU_init_ui(&current_status); // must be called after core is running, but clears status
    OSD_Reset(0);
    Timer_Wait(100);

    if (current_status.osd_init == OSD_INIT_ON) {
        MENU_set_state(&current_status, SHOW_STATUS);
        current_status.update = 1;
        OSD_Enable(DISABLE_KEYBOARD);

    } else {
        MENU_set_state(&current_status, NO_MENU);
        current_status.update = 0;
    }

    DEBUG(0, "init_core() took %d ms", Timer_Convert(Timer_Get(0) - ts));
}

static __attribute__ ((noinline)) void main_update()
{
    // MAIN LOOP
    uint16_t key;

    // track memory usage, and detect heap/stack stomp
    if (1 <= debuglevel) {
        static uint16_t loop = 0;

        if ((loop++) == 0) {
            CFG_dump_mem_stats(TRUE);
        }
    }

#if PTP_USB

    if (PTP_USB_Poll()) {
        DEBUG(1, "usb packet received");
    }

#endif

    USB_Update(&current_status);

    // get keys (from Replay button, RS232 or PS/2 via OSD/FPGA)
    key = OSD_GetKeyCode(&current_status);

    if (key && (key & KF_RELEASED) == 0) {
        DEBUG(2, "Key: 0x%04X - '%s'", key, OSD_GetStringFromKeyCode(key));

    } else if (key) {
        DEBUG(2, "Key: 0x%04X", key);
    }

    // check RS232
    USART_update();

#if defined(ARDUINO_SAMD_MKRVIDOR4000)
    USBBlaster_Update();
    NINA_Update();
#endif

    // check menu
    if (current_status.spi_osd_enabled) {
        if (MENU_handle_ui(key, &current_status)) {
            // do further update stuff here...
        }

        // this key restarts the core only
        if (key == (KEY_F11 | KF_SHIFT)) {
            // perform soft-reset
            OSD_Reset(OSDCMD_CTRL_RES);
            Timer_Wait(1);
            // we should not need this, but just in case...
            OSD_Reset(OSDCMD_CTRL);
            Timer_Wait(100);
        }
    }

    // this key sequence starts the bootloader
    if ((key == KEY_RESET) || (key == (KEY_F12 | KF_CTRL | KF_ALT))) {
        CFG_call_bootloader();
    }

    // this key or key sequence starts the bootloader
    if ((key == KEY_FLASH) || (key == (KEY_F11 | KF_CTRL | KF_ALT))) {
        // set new INI file and force reload of FPGA configuration
        strcpy(current_status.ini_dir, "\\flash\\");
        strcpy(current_status.act_dir, "\\flash\\");
        strcpy(current_status.ini_file, "rApp.ini");
        // make sure FPGA is held in reset
        IO_DriveLow_OD(PIN_FPGA_RST_L);
        ACTLED_ON;
        // set PROG low to reset FPGA (open drain)
        IO_DriveLow_OD(PIN_FPGA_PROG_L);
        Timer_Wait(1);
        IO_DriveHigh_OD(PIN_FPGA_PROG_L);
        Timer_Wait(2);
        // invalidate FPGA configuration here as well
        current_status.fpga_load_ok = NO_CORE;
    }

    if (key == KEY_F12) {
        if (current_status.button == BUTTON_RESET) {
            strcpy(current_status.ini_dir, "\\");
            strcpy(current_status.act_dir, "\\");
            strcpy(current_status.ini_file, "replay.ini");
            IO_DriveLow_OD(PIN_FPGA_RST_L); // make sure FPGA is held in reset
            ACTLED_ON;
            // set PROG low to reset FPGA (open drain)
            IO_DriveLow_OD(PIN_FPGA_PROG_L);
            Timer_Wait(1);
            IO_DriveHigh_OD(PIN_FPGA_PROG_L);
            Timer_Wait(2);
            // invalidate FPGA configuration here as well
            current_status.fpga_load_ok = NO_CORE;
        }
    }

    const uint8_t card_already_detected = current_status.card_detected;
    CFG_update_status(&current_status);

    const uint8_t card_was_inserted = !card_already_detected && current_status.card_detected;
    const uint8_t card_was_removed = card_already_detected && !current_status.card_detected;
    const uint8_t card_found_but_no_fs = current_status.card_detected && card_was_inserted && !current_status.fs_mounted_ok;

    if (card_was_inserted || card_was_removed || card_found_but_no_fs) {
        CFG_card_start(&current_status);    // restart file system if card was removed or (re-)inserted
    }

    // we deconfigured externally!
    if ((!IO_Input_H(PIN_FPGA_DONE)) && (current_status.fpga_load_ok != NO_CORE)) {
        MSG_warning("FPGA has been deconfigured.");

        // assume this is the programmer and wait for it to be reconfigured
        HARDWARE_TICK delay = 0;

        while (!IO_Input_H(PIN_FPGA_DONE)) {
            if (Timer_Check(delay)) {
                MSG_warning("    waiting for reconfig....");
                delay = Timer_Get(1000);
            }

#if defined(ARDUINO_SAMD_MKRVIDOR4000)
            USBBlaster_Update();
#endif
        }

        OSD_ConfigSendCtrl((kDRAM_SEL << 8) | kDRAM_PHASE); // default phase
        FPGA_DramTrain();
        current_status.fpga_load_ok = NO_CORE;  // break the main loop
        return;
    }

    static HARDWARE_TICK sd_button_delay = -1;

    if (IO_Input_L(PIN_MENU_BUTTON) && current_status.fpga_load_ok == EMBEDDED_CORE) {
        if (sd_button_delay == -1) {
            current_status.config_d ^= 0x00000020;
            OSD_ConfigSendUserD(current_status.config_d);
            DEBUG(1, "Setting config_d = %08x", current_status.config_d);
        }

        sd_button_delay = Timer_Get(BUTTONDELAY);

    } else if (Timer_Check(sd_button_delay)) {
        sd_button_delay = -1;
    }

    // we check if we are in fallback mode and a proper sdcard is available now
    if (current_status.fpga_load_ok == EMBEDDED_CORE) {
        if (current_status.prepare_sdcard) {
            IO_DriveLow_OD(PIN_FPGA_RST_L);
            IO_DriveLow_OD(PIN_FPGA_PROG_L);
            Timer_Wait(1);
            IO_DriveHigh_OD(PIN_FPGA_PROG_L);
            Timer_Wait(2);
            current_status.fpga_load_ok = NO_CORE;

        } else if (card_was_inserted && current_status.fs_mounted_ok) {
            IO_DriveLow_OD(PIN_FPGA_RST_L); // make sure FPGA is held in reset
            ACTLED_ON;
            // reset config
            CFG_set_status_defaults(&current_status, FALSE);
            // set PROG low to reset FPGA (open drain)
            IO_DriveLow_OD(PIN_FPGA_PROG_L);
            Timer_Wait(1);
            IO_DriveHigh_OD(PIN_FPGA_PROG_L);
            Timer_Wait(2);
            // invalidate FPGA configuration here as well
            current_status.fpga_load_ok = NO_CORE;

            // } else if (card_was_inserted && !current_status.fs_mounted_ok && current_status.menu_state == SHOW_STATUS) {

            // DEBUG(2, "grace period before asking to format .. ");

            // Timer_Wait(1000);

            // DEBUG(2, "time to format the sdcard? ");

            // MENU_set_state(&current_status, POPUP_MENU);
            // strcpy(current_status.popup_msg, "Format SDCARD?");
            // current_status.popup_msg2 = "(takes ~3mins)";
            // current_status.selections = MENU_POPUP_YESNO;
            // current_status.selected = 0;
            // current_status.update = 1;
            // current_status.format_sdcard = 1;
            // current_status.do_reboot = 0;

        }
    }

    if (card_was_removed) {
        INFO("SDCARD: removed!");

        // we should do this somewhere else..
        for (int ch = 0; ch < 2; ++ch) {
            for (int drive = 0; drive < FCH_MAX_NUM; ++drive) {
                DEBUG(1, "eject %d / %d..", ch, drive);

                if (!FileIO_FCh_GetInserted(ch, drive)) {
                    continue;
                }

                DEBUG(1, "Ejecting '%s'...", FileIO_FCh_GetName(ch, drive));
                FileIO_FCh_Eject(ch, drive);
            }
        }

        FileIO_FCh_Init();

        MENU_set_state(&current_status, SHOW_STATUS);
        current_status.update = 1;

    }

    // Handle virtual drives
    if (current_status.fileio_cha_ena != 0) {
        FileIO_FCh_Process(0);
    }

    if (current_status.fileio_chb_ena != 0) {
        FileIO_FCh_Process(1);
    }

    if (current_status.clockmon) {
        FPGA_ClockMon(&current_status);
    }

}

#if defined(ARDUINO_SAMD_MKRVIDOR4000)
static __attribute__ ((noinline)) void FPGA_WriteGeneratedImage(uint32_t base)
{
    SPI_SetFreq25MHz();
    // Creates a nice R/G pattern ;)
    uint8_t buffer[512];
    uint32_t i = 0;
    uint32_t write_offset = base;

    for (int y = 0; y < 576; ++y) {
        for (int x = 0; x < 720; ++x) {
            uint8_t c, r, g, b;

            r = x * 255 / 720;
            g = y * 255 / 576;
            b = 0;

            c = (r & 0xe0) | ((g & 0xe0) >> 3) | ((b & 0xc0) >> 6);
            buffer[i++] = c;

            if (i == sizeof(buffer)) {
                if ((write_offset >> 10) & 1) {
                    ACTLED_ON;

                } else {
                    ACTLED_OFF;
                }

                if (FileIO_MCh_BufToMem((void*)buffer, write_offset, i) != 0) {
                    WARNING("DRAM write failed!");
                    return;
                }

                write_offset += i;
                i = 0;
            }
        }
    }

    if (i) {
        if (FileIO_MCh_BufToMem((void*)buffer, write_offset, i) != 0) {
            WARNING("DRAM write failed!");
        }
    }
}
#endif

static __attribute__ ((noinline)) void write_background(FF_FILE* file)
{
    // Creates a nice R/G pattern ;)
    uint8_t buffer[1024];
    uint32_t i = 0;

    for (int y = 0; y < 576; ++y) {
        for (int x = 0; x < 720; ++x) {
            uint8_t c, r, g, b;

            r = x * 255 / 720;
            g = y * 255 / 576;
            b = 0;

            c = (r & 0xe0) | ((g & 0xe0) >> 3) | ((b & 0xc0) >> 6);
            buffer[i++] = c;

            if (i == sizeof(buffer)) {
                FF_Write(file, 1, i, buffer);
                i = 0;
            }
        }
    }

    if (i) {
        FF_Write(file, 1, i, buffer);
    }
}

static __attribute__ ((noinline)) void prepare_sdcard()
{
    // Hacky, but we know we're right (and this saves a lot of time.. ;)
    pIoman->pPartition->FreeClusterCount = pIoman->pPartition->NumClusters - 1;

    FF_ERROR err;
    DEBUG(2, "Writing replay.ini");

    FF_FILE* file = FF_Open(pIoman, "\\replay.ini", FF_MODE_WRITE | FF_MODE_CREATE | FF_MODE_TRUNCATE, &err);

    if (!file) {
        ERROR("Failed to write .ini");
        return;
    }

    FF_Write(file, 1, &_binary_replay_ini_end - &_binary_replay_ini_start, (FF_T_UINT8*)&_binary_replay_ini_start);
    FF_Close(file);

    DEBUG(2, "Writing loader.bin");
    file = FF_Open(pIoman, "\\loader.bin", FF_MODE_WRITE | FF_MODE_CREATE | FF_MODE_TRUNCATE, &err);

    if (!file) {
        ERROR("Failed to write core");
        return;
    }

    FPGA_WriteEmbeddedToFile(file);
    FF_Close(file);

    DEBUG(2, "Writing background.raw");
    file = FF_Open(pIoman, "\\background.raw", FF_MODE_WRITE | FF_MODE_CREATE | FF_MODE_TRUNCATE, &err);

    if (!file) {
        ERROR("Failed to write core");
        return;
    }

    write_background(file);
    FF_Close(file);

    FF_FlushCache(pIoman);
}
