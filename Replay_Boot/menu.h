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
/** @file menu.h */

#ifndef MENU_H_INCLUDED
#define MENU_H_INCLUDED

#include "board.h"
#include "config.h"

/** number of lines used for menu (starting at line 2) */
#define MENU_HEIGHT 12

/** status line position */
#define MENU_STATUS 15

/** indentation for main menu (menu list) */
#define MENU_INDENT 1

/** indentation for sub menu (item list) */
#define MENU_ITEM_INDENT 2

/** indentation for options of actual item */
#define MENU_OPTION_INDENT 17

/** menu total width */
#define MENU_WIDTH 32

/** pop-up window height */
#define MENU_POPUP_HEIGHT 5

/** pop-up window height */
#define MENU_POPUP_WIDTH 20

/** pop-up window option OK */
#define MENU_POPUP_OK 0

/** pop-up window option YES/NO */
#define MENU_POPUP_YESNO 1

/** pop-up window option SAVE/IGNORE */
#define MENU_POPUP_SAVEIGNORE 2


/** @brief ITEM ACTION HANDLER (PRIVATE!)

    Sets first pointer on configuration structure given any key input.
    Up/down key is used to run through menu lists or item lists.
    Left/right key is used to run through options of a item.
    Escape key is used to step out of a item list (or closes the menu)
    Enter or right key is used to step in an item list of a menu.
    TODO: maybe a "init" mode may be needed as well...?

    @param action string identifying the action to perform
    @param item pointer to currectly selected item
    @param current_status status structure containing configuration structure
    @param mode execution mode - 0:disp, 1:menu-exec, 2:file-exec, 255:init
    @return 1 if executed succesfully
*/

uint8_t _MENU_action(menuitem_t* item, status_t* current_status, uint8_t mode);

/** @brief OSD CONFIG INIT

    Sets pointer of configuration structure initially.

    @param current_status status structure containing configuration structure
*/
void MENU_init_ui(status_t* current_status);

/** @brief OSD CONFIG UPDATER (PRIVATE!)

    Runs through configure structure beginning from "first" pointer and
    updates "last" pointer accordingly.

    @param current_status status structure containing configuration structure
*/
void _MENU_update_ui(status_t* current_status);

/** @brief REMOVE LAST PATH ENTRY (PRIVATE)

    Helper function to remove last directory entry to go one hierarchy up.

    @param pPath string containing the path to modify
*/
void _MENU_back_dir(char* pPath);

/** @brief OSD CONFIG HANDLER

    Sets first pointer on configuration structure given any key input.
    Up/down key is used to run through menu lists or item lists.
    Left/right key is used to run through options of a item.
    Escape key is used to step out of a item list (or closes the menu)
    Enter or right key is used to step in an item list of a menu.

    @param key key code of the key pressed
    @param current_status status structure containing configuration structure
    @return one if key press caused a UI update run
*/
uint8_t MENU_handle_ui(uint16_t key, status_t* current_status);

/** @brief CONFIG SETUP FROM OSD

    Runs through the menu structure and updates all static and
    dynamic configuration settings.

    @param current_status status structure containing configuration structure
*/
void _MENU_update_bits(status_t* current_status);

/** @brief EXIT HANDLER (PRIVATE!)

    Called from menu structure if user selects "yes" in the pop-up
    shown when leaving the menu (and accepting to perform the updates)

    @param current_status status structure containing configuration structure
    @return 1 if executed succesfully
*/
uint8_t _MENU_update(status_t* current_status);

/** @brief SET MENU STATE

    @param current_status status structure containing configuration structure
    @param state - new state to transition into
*/
void MENU_set_state(status_t* current_status, tOSDMenuState state);

//void menu_insert_fd(char *path, adfTYPE *drive);

#endif
