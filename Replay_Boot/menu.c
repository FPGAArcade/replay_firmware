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
/** @file menu.c */

#include "menu.h"

#include "osd.h"
#include "hardware/io.h"
#include "hardware/timer.h"
#include "filesel.h"
#include "fileio.h"
#include "messaging.h"
#include "usb.h"
#include "rdb.h"
#include "flash.h"


typedef enum {
    ACTION_DISPLAY = 0,
    ACTION_MENU_EXECUTE = 1,
    ACTION_BROWSER_EXECUTE = 2,
    ACTION_INIT = 255
} tActionType;

// handle display action ===================================================
static uint8_t _MENU_action_display(menuitem_t* item, status_t* current_status)
{
    // cha select,0...3 ----------------------------------
    if MATCH(item->action_name, "cha_select") {
        if ((current_status->fileio_cha_ena >> item->action_value) & 1) { // protects agains illegal values also
            if (!FileIO_FCh_GetInserted(0, item->action_value)) {
                if (current_status->fileio_cha_mode == REMOVABLE) {
                    strcpy(item->selected_option->option_name, "<RETURN> to set");

                } else {
                    strcpy(item->selected_option->option_name, "Not mounted");
                }

            } else {
                strncpy(item->selected_option->option_name, FileIO_FCh_GetName(0, item->action_value), MAX_OPTION_STRING - 1);
            }

        } else {
            item->selected_option->option_name[0] = 0;
        }

        return 1;
    }

    // chb select,0...3 ----------------------------------
    else if MATCH(item->action_name, "chb_select") {
        if ((current_status->fileio_chb_ena >> item->action_value) & 1) { // protects agains illegal values also
            if (!FileIO_FCh_GetInserted(1, item->action_value)) {
                if (current_status->fileio_chb_mode == REMOVABLE) {
                    strcpy(item->selected_option->option_name, "<RETURN> to set");

                } else {
                    strcpy(item->selected_option->option_name, "Not mounted");
                }

            } else {
                strncpy(item->selected_option->option_name, FileIO_FCh_GetName(1, item->action_value), MAX_OPTION_STRING - 1);
            }

        } else {
            item->selected_option->option_name[0] = 0;
        }

        return 1;
    }

    // iniselect ----------------------------------
    else if MATCH(item->action_name, "iniselect") {
        strcpy(item->selected_option->option_name, "<RETURN> to set");
        return 1;
    }

    // loadselect ----------------------------------
    else if MATCH(item->action_name, "loadselect") {
        //nothing to do here
        return 1;
    }

    // storeselect ---------------------------------
    else if MATCH(item->action_name, "storeselect") {
        //nothing to do here
        return 1;
    }

    // backup ini ----------------------------------
    else if MATCH(item->action_name, "backup") {
        strcpy(item->selected_option->option_name, "<RETURN> saves");
        return 1;
    }

    // reset ----------------------------------
    else if MATCH(item->action_name, "reset") {
        strcpy(item->selected_option->option_name, "<RETURN> resets");
        return 1;
    }

    // reboot ----------------------------------
    else if MATCH(item->action_name, "reboot") {
        strcpy(item->selected_option->option_name, "<RETURN> boots");
        return 1;
    }

    // mount sdcard over usb -------------------
    else if MATCH(item->action_name, "mountmsc") {
        strcpy(item->selected_option->option_name, "<RETURN> mounts");
        return 1;
    }

    // synchronize mbr with rdb ----------------
    else if MATCH(item->action_name, "syncrdb") {
        strcpy(item->selected_option->option_name, "<RETURN> syncs");
        return 1;
    }

    // flash arm firmware ----------------------
    else if MATCH(item->action_name, "flashfw") {
        strcpy(item->selected_option->option_name, "<RETURN> flash");
        return 1;
    }

    return 0;
}

// menu execute action =====================================================
static uint8_t _MENU_action_menu_execute(menuitem_t* item, status_t* current_status)
{
    // fileio cha select
    if MATCH(item->action_name, "cha_select") {
        if ((current_status->fileio_cha_ena >> item->action_value) & 1) {
            if (FileIO_FCh_GetInserted(0, item->action_value)) {
                if (current_status->fileio_cha_mode == REMOVABLE) {
                    // deselect file
                    strcpy(item->selected_option->option_name, "<RETURN> to set");
                    item->selected_option = NULL;
                    FileIO_FCh_Eject(0, item->action_value);
                    return 1;

                } else {
                    // tried to eject fixed drive... nothing for now.
                }
            } else {
                // open file browser
                // (skip re-initing 'act_dir' here in order to keep the 'current' dir when selecting additional files)
                // save the file_filter, so that we can restore it (in case we already had a filter)
                MENU_set_state(current_status, FILE_BROWSER);
                // search for CHA extension files
                Filesel_Init(current_status->dir_scan, current_status->act_dir, current_status->fileio_cha_ext);
                // initialize browser
                Filesel_ScanFirst(current_status->dir_scan);
                return 1;
            }
        }

        return 0;
    }

    // fileio chb select
    else if MATCH(item->action_name, "chb_select") {
        if ((current_status->fileio_chb_ena >> item->action_value) & 1) {
            if (FileIO_FCh_GetInserted(1, item->action_value)) {
                if (current_status->fileio_chb_mode == REMOVABLE) {
                    // deselect file
                    strcpy(item->selected_option->option_name, "<RETURN> to set");
                    item->selected_option = NULL;
                    FileIO_FCh_Eject(1, item->action_value);
                    return 1;

                } else {
                    // tried to eject fixed drive... nothing for now.
                }
            } else {
                // allocate dir_scan structure
                MENU_set_state(current_status, FILE_BROWSER);
                // open file browser
                strcpy(current_status->act_dir, current_status->ini_dir);
                // search for INI files
                Filesel_Init(current_status->dir_scan, current_status->act_dir, current_status->fileio_chb_ext);
                // initialize browser
                Filesel_ScanFirst(current_status->dir_scan);
                return 1;
            }
        }

        return 0;
    }

    // iniselect ----------------------------------
    else if MATCH(item->action_name, "iniselect") {
        // allocate dir_scan structure
        MENU_set_state(current_status, FILE_BROWSER);
        // open file browser
        strcpy(current_status->act_dir, current_status->ini_dir);
        // search for INI files
        static const file_ext_t ini_ext[2] = { {"INI"}, {"\0"} };
        Filesel_Init(current_status->dir_scan, current_status->act_dir, ini_ext);
        // initialize browser
        Filesel_ScanFirst(current_status->dir_scan);
        return 1;
    }

    // iniload -------------------------------------
    else if MATCH(item->action_name, "iniload") {
        // set new INI file and force reload of FPGA configuration
        strcpy(current_status->ini_file, item->option_list->option_name);
        strcpy(current_status->ini_dir,
               (item->option_list->next ?
                item->option_list->next->option_name : ""));
        MENU_set_state(current_status, NO_MENU);
        OSD_Disable();
        // "kill" FPGA content and re-run setup
        current_status->fpga_load_ok = NO_CORE;

        // reset config
        CFG_set_status_defaults(current_status, FALSE);

        // set PROG low to reset FPGA (open drain)
        IO_DriveLow_OD(PIN_FPGA_PROG_L);
        Timer_Wait(1);
        IO_DriveHigh_OD(PIN_FPGA_PROG_L);
        Timer_Wait(2);
        // NO update here!
        return 0;
    }

    // loadselect ----------------------------------
    else if MATCH(item->action_name, "loadselect") {
        if ((item->option_list) && (item->option_list->option_name[0] = '*')
                && (item->option_list->option_name[1] = '.') && (item->option_list->option_name[2])) {
            DEBUG(1, "LOAD from: %s ext: %s", current_status->act_dir, (item->option_list->option_name) + 2);
            // allocate dir_scan structure
            MENU_set_state(current_status, FILE_BROWSER);
            // open file browser
            strcpy(current_status->act_dir, current_status->ini_dir);
            // search for files with given extension
            static file_ext_t load_ext[2] = { {"\0"}, {"\0"} };
            _strlcpy(load_ext[0].ext, (item->option_list->option_name) + 2, sizeof(file_ext_t));
            Filesel_Init(current_status->dir_scan, current_status->act_dir, load_ext);
            // initialize browser
            Filesel_ScanFirst(current_status->dir_scan);
            return 1;
        }
    }

    // storeselect ----------------------------------
    else if MATCH(item->action_name, "storeselect") {
        if ((item->option_list) && !(item->option_list->option_name[0])) {
            char full_filename[FF_MAX_PATH];
            DEBUG(1, "STORE to: %s file: %s", current_status->act_dir, (item->option_list->option_name));

            // store with given size to given adress and optional verification run
            // conf value is the memory size to store and a 1 bit halt flag (LSB)
            if ((item->option_list->conf_value) & 1) {
                // halt the core if requested
                OSD_Reset(OSDCMD_CTRL_HALT);
                Timer_Wait(1);
            }

            sprintf(full_filename, "%s%s", current_status->act_dir, item->option_list->option_name);

            CFG_download_rom(full_filename, item->action_value, item->option_list->conf_value >> 1);
            MENU_set_state(current_status, NO_MENU);
            OSD_Disable();
            Timer_Wait(1);

            if ((item->option_list->conf_value) & 1) {
                // continue operation of the core if requested
                OSD_Reset(0);
            }

            Timer_Wait(1);
            return 1;

            return 1;
        }
    }

    // backup ----------------------------------
    else if MATCH(item->action_name, "backup") {
        CFG_save_all(current_status, current_status->act_dir, "backup.ini");
        current_status->update = 1;
        return 1;
    }

    // reset ----------------------------------
    else if MATCH(item->action_name, "reset") {
        MENU_set_state(current_status, POPUP_MENU);
        strcpy(current_status->popup_msg, " Reset Target? ");
        current_status->selections = MENU_POPUP_YESNO;
        current_status->selected = 0;
        current_status->update = 1;
        current_status->do_reboot = 0;
        return 1;
    }

    // reboot ----------------------------------
    else if MATCH(item->action_name, "reboot") {
        MENU_set_state(current_status, POPUP_MENU);
        strcpy(current_status->popup_msg, " Reboot Board? ");
        current_status->selections = MENU_POPUP_YESNO;
        current_status->selected = 0;
        current_status->update = 1;
        current_status->do_reboot = 1;
        return 1;
    }

    // mount sdcard over usb -------------------
    else if MATCH(item->action_name, "mountmsc") {
        MENU_set_state(current_status, POPUP_MENU);
        strcpy(current_status->popup_msg, "This will unmount all images!");
        current_status->popup_msg2 = "Continue?";
        current_status->selections = MENU_POPUP_YESNO;
        current_status->selected = 0;
        current_status->update = 1;
        current_status->toggle_usb = 1;
        current_status->do_reboot = 0;
        return 1;
    }

    // synchronize mbr with rdb ----------------
    else if MATCH(item->action_name, "syncrdb") {
        MENU_set_state(current_status, POPUP_MENU);
        strcpy(current_status->popup_msg, "Existing RDB will be lost!");
        current_status->popup_msg2 = "Continue?";
        current_status->selections = MENU_POPUP_YESNO;
        current_status->selected = 0;
        current_status->update = 1;
        current_status->sync_rdb = 1;
        current_status->do_reboot = 0;
        return 1;
    }

    // flash arm firmware ----------------------
    else if MATCH(item->action_name, "flashfw") {
        // allocate dir_scan structure
        MENU_set_state(current_status, FILE_BROWSER);
        // open file browser
        strcpy(current_status->act_dir, "/");
        // search for INI files
        static const file_ext_t ini_ext[2] = { {"S19"}, {"\0"} };
        Filesel_Init(current_status->dir_scan, current_status->act_dir, ini_ext);
        // initialize browser
        Filesel_ScanFirst(current_status->dir_scan);
        return 1;
    }

    // rom upload -----------------------------
    else if MATCH(item->action_name, "rom") {
        // allocate dir_scan structure
        MENU_set_state(current_status, FILE_BROWSER);
        // search for INI files
        Filesel_Init(current_status->dir_scan, current_status->act_dir, NULL);
        // initialize browser
        Filesel_ScanFirst(current_status->dir_scan);
        return 1;
    }

    return 0;
}

// browser execute action ==================================================
static uint8_t _MENU_action_browser_execute(menuitem_t* item, status_t* current_status)
{
    // *****
    FF_DIRENT mydir = Filesel_GetEntry(current_status->dir_scan,
                                       current_status->dir_scan->sel);

    // insert is ok for fixed or removable
    // cha_select
    if MATCH(item->action_name, "cha_select") {
        if ((current_status->fileio_cha_ena >> item->action_value) & 1) {
            item->selected_option = item->option_list;

            char file_path[FF_MAX_PATH] = "";
            sprintf(file_path, "%s%s", current_status->act_dir, mydir.FileName);
            FileIO_FCh_Insert(0, item->action_value, file_path);

            strncpy(item->option_list->option_name, FileIO_FCh_GetName(0, item->action_value), MAX_OPTION_STRING - 1);
            item->option_list->option_name[MAX_OPTION_STRING - 1] = 0;

            MENU_set_state(current_status, SHOW_MENU);
            return 1;
        }
    }

    // chb_select
    else if MATCH(item->action_name, "chb_select") {
        if ((current_status->fileio_chb_ena >> item->action_value) & 1) {
            item->selected_option = item->option_list;

            char file_path[FF_MAX_PATH] = "";
            sprintf(file_path, "%s%s", current_status->act_dir, mydir.FileName);
            FileIO_FCh_Insert(1, item->action_value, file_path);

            strncpy(item->option_list->option_name, FileIO_FCh_GetName(1, item->action_value), MAX_OPTION_STRING - 1);
            item->option_list->option_name[MAX_OPTION_STRING - 1] = 0;

            MENU_set_state(current_status, SHOW_MENU);
            return 1;
        }
    }

    // iniselect ----------------------------------
    else if MATCH(item->action_name, "iniselect") {
        // set new INI file and force reload of FPGA configuration
        strcpy(current_status->ini_dir, current_status->act_dir);
        strcpy(current_status->ini_file, mydir.FileName);
        MENU_set_state(current_status, NO_MENU);
        OSD_Disable();
        // "kill" FPGA content and re-run setup
        current_status->fpga_load_ok = NO_CORE;

        // reset config
        CFG_set_status_defaults(current_status, FALSE);

        // set PROG low to reset FPGA (open drain)
        IO_DriveLow_OD(PIN_FPGA_PROG_L);
        Timer_Wait(1);
        IO_DriveHigh_OD(PIN_FPGA_PROG_L);
        Timer_Wait(2);
        // NO update here!
        return 0;
    }

    // loadselect ----------------------------------
    else if MATCH(item->action_name, "loadselect") {
        if ((current_status->act_dir[0]) && (mydir.FileName[0])) {
            char full_filename[FF_MAX_PATH];

            // upload with auto-size to given adress and optional verification run
            // conf value is 8 bit format, 1 bit halt flag, 1 bit reset flag, 1 bit verification flag (LSB)
            if ((item->option_list->conf_value >> 2) & 1) {
                // halt the core if requested
                OSD_Reset(OSDCMD_CTRL_HALT);
                Timer_Wait(1);
            }

            sprintf(full_filename, "%s%s", current_status->act_dir, mydir.FileName);

            uint32_t staticbits = current_status->config_s;
            uint32_t dynamicbits = current_status->config_d;
            DEBUG(1, "OLD config - S:%08lx D:%08lx", staticbits, dynamicbits);
            CFG_upload_rom(full_filename, item->action_value, 0, item->option_list->conf_value & 1, (item->option_list->conf_value >> 3) & 255, &staticbits, &dynamicbits);
            DEBUG(1, "NEW config - S:%08lx D:%08lx", staticbits, dynamicbits);
            current_status->config_s = staticbits;
            //not used yet: current_status->config_d = dynamicbits;
            // send bits to FPGA
            OSD_ConfigSendUserS(staticbits);
            //not used yet: OSD_ConfigSendUserD(dynamicbits);

            MENU_set_state(current_status, NO_MENU);
            OSD_Disable();

            Timer_Wait(1);

            if ((item->option_list->conf_value >> 2) & 1) {
                // continue operation of the core if requested
                OSD_Reset(0);
            }

            if ((item->option_list->conf_value >> 1) & 1) {
                // perform soft-reset if requested
                OSD_Reset(OSDCMD_CTRL_RES);
            }

            Timer_Wait(1);
            return 1;
        }
    }

    // flash arm firmware ----------------------
    else if MATCH(item->action_name, "flashfw") {
        MENU_set_state(current_status, SHOW_STATUS);

        if (!(current_status->act_dir[0]) || !(mydir.FileName[0])) {
            WARNING("No flash image selected!");
            return 1;
        }

        sprintf(current_status->act_dir, "%s%s", current_status->act_dir, mydir.FileName);
        INFO("Firmware file = %s", current_status->act_dir);
        INFO("Verifying file ...");
        current_status->verify_flash_fw = 1;
        return 1;
    }

    // rom upload ----------------------------
    else if MATCH(item->action_name, "rom") {
        MENU_set_state(current_status, SHOW_MENU);

        if (!(current_status->act_dir[0]) || !(mydir.FileName[0])) {
            WARNING("No image selected!");
            return 1;
        }

        char full_filename[FF_MAX_PATH];
        sprintf(full_filename, "%s%s", current_status->act_dir, mydir.FileName);
        INFO("ROM file = %s", full_filename);

        _strlcpy(item->selected_option->option_name, mydir.FileName, sizeof(item->selected_option->option_name));

        uint32_t size = item->conf_mask;
        uint32_t base = item->action_value;
        uint8_t format = item->conf_dynamic;

        uint32_t staticbits = current_status->config_s;
        uint32_t dynamicbits = current_status->config_d;
        DEBUG(1, "OLD config - S:%08lx D:%08lx", staticbits, dynamicbits);
        DEBUG(2, "ROM upload @ 0x%08lX (%ld byte)", base, size);

        OSD_Reset(OSDCMD_CTRL_RES | OSDCMD_CTRL_HALT);
        Timer_Wait(100);

        if (CFG_upload_rom(full_filename, base, size,
                           current_status->verify_dl, format, &staticbits, &dynamicbits)) {
            WARNING("ROM upload to FPGA failed");
            return 0;
        }

        DEBUG(1, "NEW config - S:%08lx D:%08lx", staticbits, dynamicbits);
        current_status->config_s = staticbits;
        //not used yet: pStatus->config_d = dynamicbits;
        // send bits to FPGA
        OSD_ConfigSendUserS(staticbits);

        OSD_Reset(OSDCMD_CTRL_RES);
        Timer_Wait(100);

        return 1;
    }

    return 0;
}

// ==============================================
// == PLATFORM DEPENDENT MENU CHANGES --> HERE !
// ==============================================
uint8_t _MENU_action(menuitem_t* item, status_t* current_status, tActionType mode)
{
    // check first if we need to do anything...
    if (!item || !item->action_name[0]) {
        return 0;
    }

    // INIT ====================================================================
    if (mode == ACTION_INIT) {
        // not used yet - take care, *item is NULL for this callback
    }

    // handle display action ===================================================
    if (mode == ACTION_DISPLAY) {
        return _MENU_action_display(item, current_status);
    }

    // menu execute action =====================================================
    if (mode == ACTION_MENU_EXECUTE) {
        return _MENU_action_menu_execute(item, current_status);
    }

    // browser execute action ==================================================
    if (mode == ACTION_BROWSER_EXECUTE) {
        return _MENU_action_browser_execute(item, current_status);
    }

    return 0;
}

// ==============================================
// == PLATFORM INDEPENDENT CODE STARTS HERE !
// ==============================================

void MENU_init_ui(status_t* current_status)
{

    // set up initial state of menu osd pointers
    if (current_status->menu_top) {
        current_status->menu_act = current_status->menu_top;
        current_status->menu_item_act = current_status->menu_top->item_list;
        // initially update bitfields if a menu exists to merge
        // any default bit settings with default menu entries
        _MENU_update_bits(current_status);

    } else {
        current_status->menu_act = NULL;
        current_status->menu_item_act = NULL;
    }

    // we start with the list of menu entries
    current_status->item_first = NULL;
    current_status->item_last = NULL;

    // we may do some initialization here...
    _MENU_action(NULL, current_status, ACTION_INIT);

    // clean up both OSD pages
    OSD_SetPage(0);
    OSD_Clear();
    OSD_SetPage(1);
    OSD_Clear();

    OSD_SetDisplay(0);
    OSD_SetPage(1);

    // clean up display flags
    MENU_set_state(current_status, NO_MENU);
}

void _MENU_back_dir(char* pPath)
{
    uint32_t len = strlen(pPath);
    uint32_t i = 0;

    if (len < 2) {
        _strlcpy(pPath, "\\", FF_MAX_FILENAME);
        return;
    }

    for (i = len - 1; i > 0; --i) {
        if (pPath[i - 1] == '\\') {
            pPath[i] = '\0';
            break;
        }
    }
}

// A bit ugly construct due to the lack of support for variable format precision in (s)printf
#define print_centered_status(fmt, ...)                         \
    do {                                                          \
        char buffer[64];                                            \
        const char center = 16;                                     \
        sprintf(buffer, "                " fmt, __VA_ARGS__);       \
        size_t padding = (strlen(buffer) - center) / 2;             \
        OSD_WriteRC(MENU_STATUS, 0,                                 \
                    buffer + (padding < center ? padding : center), \
                    0, WHITE, BLACK);                            \
    } while (0)

//
// STATUS SCREEN
//
static void _MENU_show_status(status_t* current_status)
{
    tOSDColor col; // used for selecting text fg color
    uint32_t i, j; // used for loops

    // print fixed status lines and a separator line
    for (i = 0; i < 3; i++) {
        OSD_WriteRC(2 + i, 0, current_status->status[i], 0, GRAY, BLACK);
    }

    // output refresh rate
    {
        char buffer[8];
        sprintf(buffer, "%3dHz", OSD_GetVerticalRefreshRate());
        OSD_WriteRC(2, OSDLINELEN - strlen(buffer), buffer, 0, YELLOW, BLACK);
    }

    OSD_WriteRC(2 + i, 0,   "                                ", 0, BLACK, DARK_BLUE);

    // finally print the "scrolling" info lines, we start with the oldest
    // entry which is always the next entry after the latest (given by idx)
    j = current_status->info_start_idx + 1;

    for (i = 0; i < 8; i++) {
        j = j & 7;

        // do colouring of different messages
        switch (current_status->info[j][0]) {
            case 'W': // WARNING
                col = DARK_YELLOW;
                break;

            case 'E': // ERROR
                col = RED;
                break;

            case 'I': // INFO
                col = GRAY;
                break;

            default: // DEBUG
                col = DARK_CYAN;
                break;
        }

        OSD_WriteRC(6 + i, 0, current_status->info[j++], 0, col, BLACK);
    }

    // print status line
    print_centered_status("LEFT/RIGHT - %s/ESC", current_status->hotkey_string);
}

//
// USB STATUS SCREEN
//
static void _MENU_show_usb_status(status_t* current_status)
{
    const uint32_t num_infos = sizeof(current_status->info) / sizeof(current_status->info[0]);
    const uint32_t start_index = current_status->info_start_idx;
    tOSDColor col; // used for selecting text fg color

    const tOSDColor line_color = MSC_PreventMediaRemoval() ? RED : DARK_BLUE;

    // print fixed status lines and a separator line
    for (int i = 0; i < 3; i++) {
        OSD_WriteRC(2 + i, 0, current_status->status[i], 0, GRAY, BLACK);
    }

    OSD_WriteRC(5, 0,   "                                ", 0, BLACK, line_color);

    for (int i = 8, j = 0; i > 0 && j < num_infos; j++) {
        uint32_t k = (start_index - j) & 7;

        // do colouring of different messages
        switch (current_status->info[k][0]) {
            case 'W': // WARNING
                col = DARK_YELLOW;
                break;

            case 'E': // ERROR
                col = RED;
                break;

            case 'I': // INFO
                col = GRAY;
                break;

            default: // DEBUG
                col = DARK_CYAN;
                break;
        }

        OSD_WriteRC(6 + --i, 0, current_status->info[k], 0, col, BLACK);
    }

    OSD_WriteRC(14, 0,   "                                ", 0, BLACK, line_color);

    // print status line
    print_centered_status("ESC to unmount%s", "");
}

//
// POP UP MESSAGE HANDLER
//

#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
        __typeof__ (b) _b = (b); \
        _a < _b ? _a : _b; })

static void _MENU_show_popup(status_t* current_status)
{
    const uint8_t popup_msg_len = strlen(current_status->popup_msg);
    const uint8_t popup_width = MENU_POPUP_WIDTH < popup_msg_len + 1 ? min(popup_msg_len + 1, MENU_WIDTH) : MENU_POPUP_WIDTH;
    const uint8_t startrow = (16 - MENU_POPUP_HEIGHT) >> 1;
    const uint8_t startcol = (MENU_WIDTH - popup_width) >> 1;
    // Currently there are 2 popups. If we add more, things could be more generic
    const static char* popup_choices[2][2] = {{"yes", "no"}, {"save", "ignore"}};
    const char** choices = popup_choices[current_status->selections == MENU_POPUP_YESNO ? 0 : 1];
    uint8_t row = startrow;
    uint8_t chwidths[2] = {strlen(choices[0]), strlen(choices[1])};
    uint8_t chcols[2] = {current_status->selected ? DARK_RED : WHITE,
                         current_status->selected ? WHITE : DARK_RED
                        };

    // draw red popup 'box'
    do {
        const char line[MENU_WIDTH] = "                                ";
        OSD_WriteRCt(row, startcol, line + (MENU_WIDTH - popup_width), popup_width, 0, BLACK, DARK_RED);
    } while (++row < startrow + MENU_POPUP_HEIGHT);

    // write pop-up message, centered
    OSD_WriteRC(startrow + 1, startcol + (popup_width >> 1) - (popup_msg_len >> 1),
                current_status->popup_msg, 0, WHITE, DARK_RED);

    if (current_status->popup_msg2 != NULL) {
        OSD_WriteRC(startrow + 2, startcol + (popup_width >> 1) - (strlen(current_status->popup_msg2) >> 1),
                    current_status->popup_msg2, 0, WHITE, DARK_RED);
    }

    // write choices
    OSD_WriteRC(row - 2, startcol + 2, choices[0], 0, chcols[0], chcols[1]);
    OSD_WriteRC(row - 2, startcol + popup_width - 2 - chwidths[1], choices[1], 0, chcols[1], chcols[0]);
}

//
// FILEBROWSER HANDLER
//
void _MENU_show_file_browser(status_t* current_status)
{
    uint32_t i; // used for loops

    // handling file browser part
    OSD_WriteRC(0, 3,    "      FILE  BROWSER       ", 0, YELLOW, BLACK);

    char* filename;
    FF_DIRENT entry;

    // show filter in header
    uint8_t sel = Filesel_GetSel(current_status->dir_scan);

    if (current_status->dir_scan->file_filter[0]) {
        char s[OSDMAXLEN + 1];
        size_t len = strlen(current_status->dir_scan->file_filter);
        s[0] = '*';
        memcpy(s + 1, current_status->dir_scan->file_filter, len);
        s[len + 1] = '*';
        s[len + 2] = 0;
        OSD_WriteRC(1, 0, s , 0, WHITE, DARK_BLUE);
    }

    // show file/directory list
    if (current_status->dir_scan->total_entries == 0) {
        // nothing there
        char s[OSDMAXLEN + 1];
        strcpy(s, "            No files            ");
        OSD_WriteRC(1 + 2, 0, s, 1, RED, BLACK);

    } else {
        for (i = 0; i < MENU_HEIGHT; i++) {
            char s[OSDMAXLEN + 1];
            memset(s, ' ', OSDMAXLEN); // clear line buffer
            s[OSDMAXLEN] = 0; // set temporary string length to OSD line length

            entry = Filesel_GetLine(current_status->dir_scan, i);
            filename = entry.FileName;

            uint16_t len = strlen(filename);

            if (i == sel) {
                if (len > OSDLINELEN) { // enable scroll if long name
                    strncpy (current_status->scroll_txt, filename, FF_MAX_FILENAME);
                    current_status->scroll_pos = i + 2;
                    current_status->scroll_len = len;
                }
            }

            if (entry.Attrib & FF_FAT_ATTR_DIR) {
                // a directory
                strncpy (s, filename, OSDMAXLEN);
                OSD_WriteRC(i + 2, 0, s, i == sel, GREEN, BLACK);

            } else {
                // a file
                strncpy (s, filename, OSDMAXLEN);
                OSD_WriteRC(i + 2, 0, s, i == sel, CYAN, BLACK);
            }

        }
    }

    // print status line
    print_centered_status("UP/DOWN/RET/ESC - %s", current_status->hotkey_string);
}

//
// SETUP MENUS / GENERAL CONFIG HANDLER
//
void _MENU_show_config_menu(status_t* current_status)
{
    tOSDColor col; // used for selecting text fg color
    menuitem_t* pItem = current_status->item_first;
    uint8_t line = 0, row = 2;

    OSD_WriteRC(0, 3, "      REPLAY CONFIG      ", 0, RED, BLACK);

    if (!pItem) {
        // print  "selected" menu title (we pan through menus)
        OSD_WriteRC(row++, MENU_INDENT, current_status->menu_act->menu_title,
                    1, YELLOW, BLACK);
        pItem = current_status->menu_act->item_list;
        current_status->menu_item_act = NULL;

    } else {
        // print  "not selected" menu title  (we select and modify items)
        OSD_WriteRC(row++, MENU_INDENT, current_status->menu_act->menu_title,
                    0, YELLOW, BLACK);
    }

    // show the item_name list to browse
    while (pItem && ((line++) < (MENU_HEIGHT - 1))) {

        OSD_WriteRCt(row, MENU_ITEM_INDENT, pItem->item_name,
                     MENU_OPTION_INDENT - MENU_ITEM_INDENT, 0, CYAN, BLACK);

        // temp bodge to show read only flags
        if MATCH(pItem->action_name, "cha_select") {
            if ((current_status->fileio_cha_ena >> pItem->action_value) & 1) {
                if (FileIO_FCh_GetReadOnly(0, pItem->action_value)) {
                    OSD_WriteRC(row, 0, "R", 0, GREEN, BLACK);

                } else if (FileIO_FCh_GetProtect(0, pItem->action_value)) {
                    OSD_WriteRC(row, 0, "P", 0, GREEN, BLACK);
                }
            }
        }

        if MATCH(pItem->action_name, "chb_select") {
            if ((current_status->fileio_chb_ena >> pItem->action_value) & 1) {
                if (FileIO_FCh_GetReadOnly(1, pItem->action_value)) {
                    OSD_WriteRC(row, 0, "R", 0, GREEN, BLACK);

                } else if (FileIO_FCh_GetProtect(1, pItem->action_value)) {
                    OSD_WriteRC(row, 0, "P", 0, GREEN, BLACK);
                }
            }
        }

        // no selection yet set, set to first option in list as default
        if (!pItem->selected_option) {
            pItem->selected_option = pItem->option_list;
        }

        // check for action before displaying
        _MENU_action(pItem, current_status, ACTION_DISPLAY);

        col = pItem->conf_dynamic ? WHITE : GREEN;

        // check if we really have an option_name, otherwise show placeholder
        if (pItem->selected_option->option_name[0]) {
            OSD_WriteRCt(row, MENU_OPTION_INDENT,
                         pItem->selected_option->option_name,
                         MENU_WIDTH - MENU_OPTION_INDENT, // len limit
                         pItem == current_status->menu_item_act ? 1 : 0, col, BLACK);

        } else {
            OSD_WriteRC(row, MENU_OPTION_INDENT, " ",
                        pItem == current_status->menu_item_act ? 1 : 0, WHITE, BLACK);
        }

        row++;
        // keep last item_name shown
        current_status->item_last = pItem;
        pItem = pItem->next;
    }

    // print status line
    print_centered_status("UP/DWN/LE/RI - %s/ESC", current_status->hotkey_string);
}

// TODO: colours from INI file instead of hardcoding...?
void _MENU_update_ui(status_t* current_status)
{
    current_status->scroll_pos = 0;

    // clear OSD area and set the minimum header/footer lines we always need
    OSD_Clear();
    OSD_Write  (    0,  "***   F P G A  A R C A D E   ***", 0);
    OSD_WriteBase(1, 0, "", 0, 0, BLACK, DARK_BLUE, 1); // clears row
    OSD_WriteBase(1, 14, "", 0, 0, BLACK, DARK_BLUE, 1); // clears row

    if (current_status->menu_state == SHOW_STATUS) {
        /*OSD_WriteRC(1,  0,  "= REPLAY RETRO GAMING PLATFORM =", 0, BLACK, DARK_BLUE);*/
        /*OSD_WriteRC(14, 0,  "= NO EMULATION - NO COMPROMISE =", 0, DARK_MAGENTA, DARK_BLUE);*/
        // temp revert while waiting for new look and feel ...
        OSD_WriteRC(1,  0,  "                                ", 0, BLACK, DARK_BLUE);
        OSD_WriteRC(14, 0,  "                                ", 0, DARK_MAGENTA, DARK_BLUE);
        _MENU_show_status(current_status);

    } else if (current_status->menu_state == POPUP_MENU) {
        _MENU_show_popup(current_status) ;

    } else if (current_status->menu_state == FILE_BROWSER) {
        _MENU_show_file_browser(current_status);

    } else if (current_status->menu_state == USB_STATUS) {
        _MENU_show_usb_status(current_status);

    } else {
        _MENU_show_config_menu(current_status);
    }
}

typedef uint8_t (*tKeyMappingCallback)(status_t*, const uint16_t);
typedef struct _tKeyMapping {
    uint16_t            key;
    uint16_t            mask;
    tKeyMappingCallback action;
} tKeyMapping;


static uint8_t key_action_menu_left(status_t* current_status, const uint16_t key)
{
    // option_name select
    if (current_status->item_first) {
        // on item_name level
        if (!current_status->menu_item_act->action_name[0]) {
            // last option_name (only if no item with certain action)
            if (current_status->menu_item_act->selected_option &&
                    current_status->menu_item_act->selected_option->last) {
                current_status->menu_item_act->selected_option =
                    current_status->menu_item_act->selected_option->last;
                // update bitfield
                _MENU_update_bits(current_status);
            }
        }

    } else {
        // on menu level
        if (current_status->menu_act->last) {
            // not the first entry
            current_status->menu_act = current_status->menu_act->last;

        } else {
            // we had the first entry, so switch to status page
            MENU_set_state(current_status, SHOW_STATUS);
        }
    }

    return 1; // update
}

static uint8_t key_action_menu_right(status_t* current_status, const uint16_t key)
{
    if (current_status->item_first) {
        // on item_name level
        if (!current_status->menu_item_act->action_name[0]) {
            // next option_name (only if no item with certain action)
            if (current_status->menu_item_act->selected_option &&
                    current_status->menu_item_act->selected_option->next) {
                current_status->menu_item_act->selected_option =
                    current_status->menu_item_act->selected_option->next;
                // update bitfield
                _MENU_update_bits(current_status);
            }
        }

    } else {
        // on menu level
        if (current_status->menu_act->next) {
            // not the last entry
            current_status->menu_act = current_status->menu_act->next;

        } else {
            // we had the last entry, so switch to status page
            MENU_set_state(current_status, SHOW_STATUS);
        }
    }

    return 1; // update
}

static uint8_t key_action_menu_up(status_t* current_status, const uint16_t key)
{
    // scoll menu/item_name list
    if (current_status->item_first) {
        // on item_name level
        if (current_status->menu_item_act == current_status->item_first) {
            // we are on top
            if (current_status->menu_item_act->last) {
                current_status->menu_item_act = current_status->menu_item_act->last;
                current_status->item_first = current_status->menu_item_act;

            } else {
                // "exit" from item_name list
                current_status->item_first = NULL;
                current_status->menu_item_act = NULL;
            }

        } else {
            // we are in the middle
            if (current_status->menu_item_act->last) {
                current_status->menu_item_act = current_status->menu_item_act->last;
            }
        }
    }

    return 1;
}

static uint8_t key_action_menu_down(status_t* current_status, const uint16_t key)
{
    if (current_status->item_first) {
        // on item_name level
        if (current_status->menu_item_act == current_status->item_last) {
            // we are at the end
            if (current_status->menu_item_act->next) {
                current_status->item_first = current_status->item_first->next;
                current_status->menu_item_act = current_status->menu_item_act->next;
            }

        } else {
            // we are in the middle
            if (current_status->menu_item_act->next) {
                current_status->menu_item_act = current_status->menu_item_act->next;
            }
        }

    } else {
        // on menu level
        if (current_status->menu_act->item_list->item_name[0]) {
            current_status->item_first = current_status->menu_act->item_list;
            current_status->menu_item_act = current_status->item_first;
        }
    }

    return 1;
}

static uint8_t key_action_menu_enter(status_t* current_status, const uint16_t key)
{
    if (current_status->menu_item_act->action_name[0]) {
        // check for menu action and update display if succesfully
        return _MENU_action(current_status->menu_item_act, current_status, ACTION_MENU_EXECUTE);
    }

    return 0;
}

static uint8_t key_action_menu_esc(status_t* current_status, const uint16_t key)
{
    // step out of item_name list to menu list
    if (current_status->item_first) {
        // exit from item_name list
        current_status->item_first = NULL;
        current_status->menu_item_act = NULL;

    } else {
        // exit from menu, but ask what to do
        MENU_set_state(current_status, POPUP_MENU);
        strcpy(current_status->popup_msg, " Reset Target? ");
        current_status->selections = MENU_POPUP_YESNO;
        current_status->selected = 0;
        current_status->do_reboot = 0;
    }

    return 1;
}

static uint8_t key_action_menu_protect(status_t* current_status, const uint16_t key)
{
    if MATCH(current_status->menu_item_act->action_name, "cha_select") {
        if ((current_status->fileio_cha_ena >> current_status->menu_item_act->action_value) & 1) {
            FileIO_FCh_TogProtect(0, current_status->menu_item_act->action_value);
            return 1;
        }
    }

    if MATCH(current_status->menu_item_act->action_name, "chb_select") {
        if ((current_status->fileio_chb_ena >> current_status->menu_item_act->action_value) & 1) {
            FileIO_FCh_TogProtect(1, current_status->menu_item_act->action_value);
            return 1;
        }
    }

    return 0;
}




static uint8_t key_action_filebrowser_left(status_t* current_status, const uint16_t key)
{
    Filesel_Update(current_status->dir_scan, SCAN_PREV_PAGE);
    return 1; // update
}

static uint8_t key_action_filebrowser_right(status_t* current_status, const uint16_t key)
{
    Filesel_Update(current_status->dir_scan, SCAN_NEXT_PAGE);
    return 1; // update
}

static uint8_t key_action_filebrowser_up(status_t* current_status, const uint16_t key)
{
    Filesel_Update(current_status->dir_scan, SCAN_PREV);
    return 1;
}

static uint8_t key_action_filebrowser_down(status_t* current_status, const uint16_t key)
{
    Filesel_Update(current_status->dir_scan, SCAN_NEXT);
    return 1;
}

static uint8_t key_action_filebrowser_enter(status_t* current_status, const uint16_t key)
{
    FF_DIRENT mydir = Filesel_GetEntry(current_status->dir_scan, current_status->dir_scan->sel);

    if (mydir.Attrib & FF_FAT_ATTR_DIR) {
        //dir_sel
        if (_strnicmp(mydir.FileName, ".", FF_MAX_FILENAME) != 0) {
            if (_strnicmp(mydir.FileName, "..", FF_MAX_FILENAME) == 0) {
                _MENU_back_dir(current_status->act_dir);

            } else {
                // should use safe copy
                sprintf(current_status->act_dir, "%s%s\\", current_status->act_dir,
                        mydir.FileName);
            }

            // sets path
            Filesel_ChangeDir(current_status->dir_scan, current_status->act_dir);
            Filesel_ScanFirst(current_status->dir_scan); // scan first
            return 1;
        }

        return 0;

    } else {
        // check for filebrowser action and update display if successful
        return _MENU_action(current_status->menu_item_act,
                            current_status, ACTION_BROWSER_EXECUTE);
    }
}

static uint8_t key_action_filebrowser_esc(status_t* current_status, const uint16_t key)
{
    // quit filebrowsing w/o changes
    MENU_set_state(current_status, SHOW_MENU);
    return 1;
}

static uint8_t key_action_filebrowser_back(status_t* current_status, const uint16_t key)
{
    Filesel_DelFilterChar(current_status->dir_scan);
    // keep grace period here before re-scanning - user might be typing..
    current_status->filescan_timer = Timer_Get(250);
    current_status->delayed_filescan = 1;
    return 0;
}

static uint8_t key_action_filebrowser_home(status_t* current_status, const uint16_t key)
{
    Filesel_ChangeDir(current_status->dir_scan, current_status->act_dir);
    Filesel_ScanFirst(current_status->dir_scan); // scan first
    return 1;
}


static uint8_t key_action_filebrowser_default(status_t* current_status, const uint16_t key)
{
    if ( ((key >= '0') && (key <= '9')) || ((key >= 'A') && (key <= 'Z')) ) {
        Filesel_AddFilterChar(current_status->dir_scan, key & 0x7F);
        // keep grace period here before re-scanning - user might be typing..
        current_status->filescan_timer = Timer_Get(250);
        current_status->delayed_filescan = 1;
    }

    return 0;
}



static uint8_t key_action_showstatus_left(status_t* current_status, const uint16_t key)
{
    // go to first menu entry
    while (current_status->menu_act->next) {
        current_status->menu_act = current_status->menu_act->next;
    }

    MENU_set_state(current_status, SHOW_MENU);
    return 1;
}

static uint8_t key_action_showstatus_right(status_t* current_status, const uint16_t key)
{
    // go to last menu entry
    while (current_status->menu_act->last) {
        current_status->menu_act = current_status->menu_act->last;
    }

    MENU_set_state(current_status, SHOW_MENU);
    return 1;
}



static uint8_t key_action_popup_left_right(status_t* current_status, const uint16_t key)
{
    current_status->selected = current_status->selected == 1 ? 0 : 1;
    return 1;
}

static uint8_t key_action_popup_enter(status_t* current_status, const uint16_t key)
{
    // ok, we don't have anything set yet to handle the result
    MENU_set_state(current_status, NO_MENU);
    OSD_Disable();

    if (current_status->selected) {
        return _MENU_update(current_status);
    }

    current_status->do_reboot = 0;
    current_status->toggle_usb = 0;
    current_status->usb_mounted = 0;
    current_status->sync_rdb = 0;
    current_status->flash_fw = 0;
    current_status->verify_flash_fw = 0;
    current_status->format_sdcard = 0;

    return 0;
}

static uint8_t key_action_popup_esc(status_t* current_status, const uint16_t key)
{
    MENU_set_state(current_status, SHOW_MENU);
    return 1;
}

static uint8_t key_action_usb_esc(status_t* current_status, const uint16_t key)
{
    // exit from menu, but ask what to do
    MENU_set_state(current_status, POPUP_MENU);
    strcpy(current_status->popup_msg, "Make sure USB is ejected!");
    current_status->popup_msg2 = "Continue?";
    current_status->selections = MENU_POPUP_YESNO;
    current_status->selected = 0;
    current_status->toggle_usb = 1;
    current_status->update = 1;

    return 1;
}

const tKeyMapping keymappings_menu[] = {
    {.mask = KEY_MASK_ASCII, .key = 'P',       .action = key_action_menu_protect},
    {.mask = KEY_MASK,       .key = KEY_LEFT,  .action = key_action_menu_left},
    {.mask = KEY_MASK,       .key = KEY_RIGHT, .action = key_action_menu_right},
    {.mask = KEY_MASK,       .key = KEY_UP,    .action = key_action_menu_up},
    {.mask = KEY_MASK,       .key = KEY_DOWN,  .action = key_action_menu_down},
    {.mask = KEY_MASK,       .key = KEY_ENTER, .action = key_action_menu_enter},
    {.mask = KEY_MASK,       .key = KEY_ESC,   .action = key_action_menu_esc}
};

const tKeyMapping keymappings_filebrowser[] = {
    {.mask = KEY_MASK, .key = KEY_LEFT,  .action = key_action_filebrowser_left},
    {.mask = KEY_MASK, .key = KEY_PGUP,  .action = key_action_filebrowser_left},
    {.mask = KEY_MASK, .key = KEY_RIGHT, .action = key_action_filebrowser_right},
    {.mask = KEY_MASK, .key = KEY_PGDN, .action = key_action_filebrowser_right},
    {.mask = KEY_MASK, .key = KEY_UP,    .action = key_action_filebrowser_up},
    {.mask = KEY_MASK, .key = KEY_DOWN,  .action = key_action_filebrowser_down},
    {.mask = KEY_MASK, .key = KEY_ENTER, .action = key_action_filebrowser_enter},
    {.mask = KEY_MASK, .key = KEY_ESC,   .action = key_action_filebrowser_esc},
    {.mask = KEY_MASK, .key = KEY_BACK,  .action = key_action_filebrowser_back},
    {.mask = KEY_MASK, .key = KEY_HOME,  .action = key_action_filebrowser_home},
    {.mask = KEY_MASK_ASCII, .key = 0,   .action = key_action_filebrowser_default}
};

const tKeyMapping keymappings_showstatus[] = {
    {.mask = KEY_MASK, .key = KEY_LEFT,  .action = key_action_showstatus_left},
    {.mask = KEY_MASK, .key = KEY_RIGHT, .action = key_action_showstatus_right}
};

const tKeyMapping keymappings_popup[] = {
    {.mask = KEY_MASK, .key = KEY_LEFT,  .action = key_action_popup_left_right},
    {.mask = KEY_MASK, .key = KEY_RIGHT, .action = key_action_popup_left_right},
    {.mask = KEY_MASK, .key = KEY_ENTER, .action = key_action_popup_enter},
    {.mask = KEY_MASK, .key = KEY_ESC,   .action = key_action_popup_esc}
};

const tKeyMapping keymappings_showusb[] = {
    {.mask = KEY_MASK, .key = KEY_ESC,   .action = key_action_usb_esc}
};

uint8_t MENU_handle_ui(const uint16_t key, status_t* current_status)
{
    // flag OSD update may be set already externally
    uint8_t update = current_status->update;
    static HARDWARE_TICK osd_timeout;
    static uint8_t  osd_timeout_cnt = 0;
    static HARDWARE_TICK scroll_timer;
    static uint16_t scroll_text_offset = 0;
    static uint8_t  scroll_started = 0;

    const tKeyMapping* key_mappings = NULL;    // set if there is a keymapping list to check
    uint8_t key_mappings_length = 0;     // # items in keymapping list

    // Ignore released keys
    if ((key & KF_RELEASED) != 0) {
        DEBUG(3, "ignored released key");
        return 0;
    }

    current_status->update = 0;

    // timeout of OSD after noticing last key press (or external forced update)
    if (current_status->menu_state != NO_MENU &&
            current_status->osd_timeout != 0) {  // 0 = disabled
        if (key || update) {
            // we set our timeout with any update (1 sec)
            osd_timeout = Timer_Get(1000);
            osd_timeout_cnt = 0;
            //DEBUG(3, "OSD timeout set to 0");

        } else if (Timer_Check(osd_timeout)) {
            if (osd_timeout_cnt++ < current_status->osd_timeout) {
                // we set our timeout again with any update (1 sec)
                osd_timeout = Timer_Get(1000);
                //DEBUG(3, "OSD timeout tick");

            } else {
                // hide menu after ~30 sec
                DEBUG(3, "OSD timed out, hiding");
                MENU_set_state(current_status, NO_MENU);
                OSD_Disable();
                return 0;
            }
        }

    } else {
        if (update) {
            MENU_set_state(current_status, SHOW_STATUS);
            osd_timeout = Timer_Get(1000);
            // show half the time
            osd_timeout_cnt = 15;
            DEBUG(3, "OSD timeout set to 15");
        }
    }


    if (current_status->verify_flash_fw) {
        current_status->verify_flash_fw = 0;
        uint32_t crc_file, crc_flash;

        if (!FLASH_VerifySRecords(current_status->act_dir, &crc_file, &crc_flash)) {
            WARNING("Flash image is not valid!");

        } else {
            MENU_set_state(current_status, POPUP_MENU);
            sprintf(current_status->popup_msg, "CRC32 New:%08x Old:%08x", crc_file, crc_flash);
            current_status->popup_msg2 = "Reboot and flash?";
            current_status->selections = MENU_POPUP_YESNO;
            current_status->selected = 0;
            current_status->update = 1;
            current_status->flash_fw = 1;
        }
    }

    // --------------------------------------------------

    if (key == KEY_MENU) {
        // OSD menu handling, switch it on/off
        DEBUG(1, "KEY_MENU detected.");

        if (current_status->menu_state) {
            // hide menu
            MENU_set_state(current_status, NO_MENU);
            OSD_Disable();
            DEBUG(1, "OSD disabled as no menu_state.");
            return 0;

        } else {
            // show status
            MENU_set_state(current_status, SHOW_STATUS);
            update = 1;
            // set timeout
            osd_timeout = Timer_Get(1000);
            osd_timeout_cnt = 0;
            DEBUG(1, "OSD enabled for SHOW_STATUS");
        }
    }

    // --------------------------------------------------

    else if (current_status->menu_state == POPUP_MENU) {
        // special pop-up handler
        key_mappings = keymappings_popup;
        key_mappings_length = sizeof(keymappings_popup) / sizeof(tKeyMapping);

    } else if (current_status->menu_state == FILE_BROWSER) {
        key_mappings = keymappings_filebrowser;
        key_mappings_length = sizeof(keymappings_filebrowser) / sizeof(tKeyMapping);

    } else if ((current_status->menu_state == SHOW_STATUS) /*&&
               (current_status->fpga_load_ok != EMBEDDED_CORE)*/) {
        key_mappings = keymappings_showstatus;
        key_mappings_length = sizeof(keymappings_showstatus) / sizeof(tKeyMapping);

    } else if (current_status->menu_state == SHOW_MENU) {
        key_mappings = keymappings_menu;
        key_mappings_length = sizeof(keymappings_menu) / sizeof(tKeyMapping);

    } else if (current_status->menu_state == USB_STATUS) {
        key_mappings = keymappings_showusb;
        key_mappings_length = sizeof(keymappings_showusb) / sizeof(tKeyMapping);
    }


    if (key_mappings != NULL && key_mappings_length > 0) {
        uint8_t i;

        for (i = 0; i < key_mappings_length; i++) {
            // Mask out KF_REPEATED, as we don't care if it's repeated or not..
            uint16_t mskd = (key & key_mappings[i].mask) & ~KF_REPEATED;

            if ((key_mappings[i].key == 0 && mskd != 0) || // 'default' (ascii chars)
                    (mskd != 0 && mskd == key_mappings[i].key)) { // 'specific' key
                update = (*key_mappings[i].action)(current_status, key);
                break;
            }
        }
    }

    // Add/DelFilterChar waits 250ms before re-scanning the directory and updating the UI
    if (current_status->delayed_filescan && Timer_Check(current_status->filescan_timer)) {
        Filesel_ScanFirst(current_status->dir_scan);
        current_status->delayed_filescan = 0;
        update = 1;
    }

    if (current_status->menu_state == SHOW_STATUS) {
        static uint32_t last_rate = 0;
        uint32_t refresh_rate = OSD_GetVerticalRefreshRate();
        update |= (refresh_rate != last_rate);
        last_rate = refresh_rate;
    }

    // update menu if needed
    if (update) {
        _MENU_update_ui(current_status);

        OSD_SetDisplay(OSD_GetPage()); //  OSD_GetPage());
        OSD_SetPage(OSD_NextPage());   //  OSD_NextPage());

        scroll_timer = Timer_Get(1000); // restart scroll timer
        scroll_started = 0;

        if (current_status->menu_state) {
            OSD_Enable(DISABLE_KEYBOARD); // just in case, enable OSD and disable KEYBOARD for core
        }

    } else {

        // scroll if needed
        if ((current_status->menu_state == FILE_BROWSER) && current_status->scroll_pos) {
            const uint16_t len    = current_status->scroll_len;

            if (!scroll_started) {
                if (Timer_Check(scroll_timer)) { // scroll if timer elapsed
                    scroll_started = 1;
                    scroll_text_offset = 0;
                    /*scroll_pix_offset  = 0;*/

                    OSD_WriteScroll(current_status->scroll_pos, current_status->scroll_txt, 0, len, 1, CYAN, 0);
                    scroll_timer = Timer_Get(20); // restart scroll timer
                }

            } else if (Timer_Check(scroll_timer)) { // scroll if timer elapsed

                scroll_timer = Timer_Get(20); // restart scroll timer
                scroll_text_offset = scroll_text_offset + 2;

                if (scroll_text_offset >= (len + OSD_SCROLL_BLANKSPACE) << 4) {
                    scroll_text_offset = 0;
                }

                if ((scroll_text_offset & 0xF) == 0)
                    OSD_WriteScroll(current_status->scroll_pos, current_status->scroll_txt, scroll_text_offset >> 4,
                                    len, 1, CYAN, 0);

                OSD_SetHOffset(current_status->scroll_pos, 0, (uint8_t) (scroll_text_offset & 0xF));
            }
        }
    }

    return update;
}

void _MENU_update_bits(status_t* current_status)
{
    menu_t* pMenu = current_status->menu_top;
    uint32_t staticbits = current_status->config_s;
    uint32_t dynamicbits = current_status->config_d;

    DEBUG(2, "OLD config - S:%08lx D:%08lx", staticbits, dynamicbits);

    // run through config structure and update configuration settings
    while (pMenu) {
        menuitem_t* pItem = pMenu->item_list;

        while (pItem) {
            DEBUG(3, "%s: ", pItem->item_name);

            // we add only items which are no action item and got selected once
            // only this entries will (must) have an item mask and an option value
            if ((!pItem->action_name[0]) && pItem->selected_option) {
                DEBUG(3, "%08lx ", pItem->conf_mask);

                if (pItem->conf_dynamic) {
                    DEBUG(3, "%08lx dyn", pItem->selected_option->conf_value);
                    dynamicbits &= ~pItem->conf_mask; //(pItem->conf_mask ^ 0xffffffff);
                    dynamicbits |= pItem->selected_option->conf_value;

                } else {
                    DEBUG(3, "%08lx stat", pItem->selected_option->conf_value);
                    staticbits &= ~pItem->conf_mask; //(pItem->conf_mask ^ 0xffffffff);
                    staticbits |= pItem->selected_option->conf_value;
                }
            }

            DEBUG(3, "");
            pItem = pItem->next;
        }

        pMenu = pMenu->next;
    }

    DEBUG(2, "NEW config - S:%08lx D:%08lx", staticbits, dynamicbits);
    current_status->config_s = staticbits;
    current_status->config_d = dynamicbits;

    // send bits to FPGA
    OSD_ConfigSendUserD(dynamicbits);
    OSD_ConfigSendUserS(staticbits);
}

uint8_t _MENU_update(status_t* current_status)
{
    DEBUG(1, "Config accepted: S:%08lx D:%08lx / %d", current_status->config_s,
          current_status->config_d,
          current_status->do_reboot);

    if (current_status->do_reboot) {
        // set PROG low to reset FPGA configuration (open drain)
        IO_DriveLow_OD(PIN_FPGA_PROG_L);
        Timer_Wait(1);
        IO_DriveHigh_OD(PIN_FPGA_PROG_L);
        Timer_Wait(2);
        // Replay board reboot
        CFG_call_bootloader();
        // --> we never return back here !!!!

    } else if (current_status->toggle_usb) {
        if (current_status->usb_mounted) {
            USB_UnmountMassStorage(current_status);

        } else {
            USB_MountMassStorage(current_status);
        }

        MENU_set_state(current_status, SHOW_STATUS);
        current_status->toggle_usb = 0;
        current_status->update = 1;

    } else if (current_status->sync_rdb) {

        SynchronizeRdbWithMbr();

        MENU_set_state(current_status, SHOW_STATUS);
        current_status->sync_rdb = 0;
        current_status->update = 1;

    } else if (current_status->flash_fw) {

        FLASH_RebootAndFlash(current_status->act_dir);

        ERROR("Flash failed!");

        OSD_Disable();
        // "kill" FPGA content and re-run setup
        current_status->fpga_load_ok = NO_CORE;

        // reset config
        CFG_set_status_defaults(current_status, FALSE);

        current_status->flash_fw = 0;
        current_status->update = 1;

    } else if (current_status->format_sdcard) {

        MENU_set_state(current_status, SHOW_STATUS);
        OSD_Enable(DISABLE_KEYBOARD);

        current_status->update = 1;
        MENU_handle_ui(0, current_status);

        CFG_format_sdcard(current_status);

        WARNING("Preparing... ");
        MENU_handle_ui(0, current_status);

        MENU_set_state(current_status, SHOW_STATUS);

        current_status->prepare_sdcard = 1;
        current_status->format_sdcard = 0;
        current_status->update = 1;

    } else {
        // perform soft-reset
        OSD_Reset(OSDCMD_CTRL_RES);
        Timer_Wait(1);
        // we should not need this, but just in case...
        OSD_Reset(OSDCMD_CTRL);
        Timer_Wait(100);
        // fall back to status screen
        current_status->update = 1;
        MENU_set_state(current_status, SHOW_STATUS);
    }

    return 1;
}

void MENU_set_state(status_t* current_status, tOSDMenuState state)
{
    if (current_status->usb_mounted) {
        if (state != NO_MENU && state != POPUP_MENU) {
            state = USB_STATUS;
            DEBUG(1, "USB mounted - overriding state to %d", state);
        }
    }

    if (current_status->menu_state == state) {
        return;
    }

    DEBUG(1, "_MENU_set_state(%d)", state);

    current_status->popup_msg2 = NULL;

    // const_cast and write the new value
    *(tOSDMenuState*)(&current_status->menu_state) = state;

    // track memory usage, and detect heap/stack stomp
    if (3 <= debuglevel) {
        CFG_dump_mem_stats(FALSE);
    }
}
