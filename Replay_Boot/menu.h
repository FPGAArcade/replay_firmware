/** @file menu.h */

#ifndef MENU_H_INCLUDED
#define MENU_H_INCLUDED

#include "board.h"

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

/** @brief ITEM ACTION HANDLER

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

uint8_t _MENU_action(menuitem_t *item, status_t *current_status, uint8_t mode);

/** @brief OSD CONFIG INIT

    Sets pointer of configuration structure initially.

    @param current_status status structure containing configuration structure
*/
void MENU_init_ui(status_t *current_status);

/** @brief OSD CONFIG UPDATER

    Runs through configure structure beginning from "first" pointer and
    updates "last" pointer accordingly.

    @param current_status status structure containing configuration structure
*/
void _MENU_update_ui(status_t *current_status);

/** @brief REMOVE LAST PATH ENTRY

    Helper function to remove last directory entry to go one hierarchy up.

    @param pPath string containing the path to modify
*/
void _MENU_back_dir(char *pPath);

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
uint8_t MENU_handle_ui(uint16_t key, status_t *current_status);

/** @brief CONFIG SETUP FROM OSD

    Runs through the menu structure and updates all static and
    dynamic configuration settings.

    @param current_status status structure containing configuration structure
*/
void _MENU_update_bits(status_t *current_status);

/** @brief EXIT HANDLER

    Called from menu structure if user selects "yes" in the pop-up
    shown when leaving the menu (and accepting to perform the updates)

    @param current_status status structure containing configuration structure
    @return 1 if executed succesfully
*/
uint8_t _MENU_update(status_t *current_status);

void menu_insert_fd(char *path, adfTYPE *drive);

#endif
