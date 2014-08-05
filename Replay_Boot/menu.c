/** @file menu.c */

#include "config.h"
#include "osd.h"
#include "hardware.h"
#include "menu.h"
#include "filesel.h"
#include "messaging.h"

// ==============================================
// == PLATFORM DEPENDENT MENU CHANGES --> HERE !
// ==============================================
uint8_t _MENU_action(menuitem_t *item, status_t *current_status, uint8_t mode)
{
  // check first if we need to do anything...
  if (!item->action_name[0]) return 0;

  // INIT ====================================================================
  if (mode==255) {
    // not used yet - take care, *item is NULL for this callback
  }
  // handle display action ===================================================
  if (mode==0) {

    // fddselect,0...3 ----------------------------------
    if MATCH(item->action_name,"fddselect") {
      if ((current_status->fileio_cha_ena >> item->action_value) & 1) {
        // use inserted flag here?
        if (!FDD_Inserted(item->action_value)) {
          // set only if still blank
          strcpy(item->selected_option->option_name,"<RETURN> to set");
        }
      } else {
        item->selected_option->option_name[0]=0;
      }
      return 1;
    }
    // hddselect,0...3 ----------------------------------
    if MATCH(item->action_name,"hddselect") {
      if ((current_status->fileio_chb_ena >> item->action_value) & 1) {

        if (!HDD_Inserted(item->action_value)) {
          strcpy(item->selected_option->option_name,"<NOT MOUNTED>");
        } else {
          strncpy(item->selected_option->option_name, HDD_GetName(item->action_value), MAX_OPTION_STRING-1);
        }
      } else {
        item->selected_option->option_name[0]=0;
      }
      return 1;
    }
    // iniselect ----------------------------------
    if MATCH(item->action_name,"iniselect") {
      strcpy(item->selected_option->option_name,"<RETURN> to set");
      return 1;
    }
    // loadselect ----------------------------------
    if MATCH(item->action_name,"loadselect") {
      //nothing to do here
      return 1;
    }
    // storeselect ---------------------------------
    if MATCH(item->action_name,"storeselect") {
      //nothing to do here
      return 1;
    }
    // backup ini ----------------------------------
    if MATCH(item->action_name,"backup") {
      strcpy(item->selected_option->option_name,"<RETURN> saves");
      return 1;
    }
    // reset ----------------------------------
    if MATCH(item->action_name,"reset") {
      strcpy(item->selected_option->option_name,"<RETURN> resets");
      return 1;
    }
    // reboot ----------------------------------
    if MATCH(item->action_name,"reboot") {
      strcpy(item->selected_option->option_name,"<RETURN> boots");
      return 1;
    }
  }
  // menu execute action =====================================================
  if (mode==1) {
    // fddselect,0...3 ----------------------------------
    if MATCH(item->action_name,"fddselect") {
      if ((current_status->fileio_cha_ena >> item->action_value) & 1) {
        if (FDD_Inserted(item->action_value)) {
          // deselect file
          strcpy(item->selected_option->option_name,"<RETURN> to set");
          item->selected_option=NULL;

          // EJECT
          FDD_Eject(item->action_value);

          return 1;
        } else {
          // open file browser
          strcpy(current_status->act_dir,current_status->ini_dir);
          // search for INI files
          Filesel_Init(current_status->dir_scan, current_status->act_dir, current_status->fileio_cha_ext);
          // initialize browser
          Filesel_ScanFirst(current_status->dir_scan);
          current_status->file_browser = 1;
          current_status->show_menu=0;
          return 1;
        }
      }
      return 0;
    }
    // hddselect,0...3 ----------------------------------
    if MATCH(item->action_name,"hddselect") {
      if ((current_status->fileio_chb_ena >> item->action_value) & 1) {
        /*
        if (item->selected_option->option_name[0]!='<') {
          // deselect file
          strcpy(item->selected_option->option_name,"<RETURN> to set");
          item->selected_option=NULL;
          return 1;
        } else {
          // open file browser
          strcpy(current_status->act_dir,current_status->ini_dir);
          // search for INI files
          Filesel_Init(current_status->dir_scan, current_status->act_dir, current_status->fileio_chb_ext);
          // initialize browser
          Filesel_ScanFirst(current_status->dir_scan);
          current_status->file_browser = 1;
          current_status->show_menu=0;
          return 1;
        }
        */
      }
      return 0;
    }
    // iniselect ----------------------------------
    if MATCH(item->action_name,"iniselect") {
      // open file browser
      strcpy(current_status->act_dir,current_status->ini_dir);
      // search for INI files
      Filesel_Init(current_status->dir_scan, current_status->act_dir, "INI");
      // initialize browser
      Filesel_ScanFirst(current_status->dir_scan);
      current_status->file_browser = 1;
      current_status->show_menu=0;
      return 1;
    }
    // loadselect ----------------------------------
    if MATCH(item->action_name,"loadselect") {
      if ((item->option_list) && (item->option_list->option_name[0]='*')
          && (item->option_list->option_name[1]='.') && (item->option_list->option_name[2])) {
        DEBUG(1,"LOAD from: %s ext: %s",current_status->act_dir,(item->option_list->option_name)+2);
        // open file browser
        strcpy(current_status->act_dir,current_status->ini_dir);
        // search for files with given extension
        Filesel_Init(current_status->dir_scan, current_status->act_dir, (item->option_list->option_name)+2);
        // initialize browser
        Filesel_ScanFirst(current_status->dir_scan);
        current_status->file_browser = 1;
        current_status->show_menu=0;
        return 1;
      }
    }
    // storeselect ----------------------------------
    if MATCH(item->action_name,"storeselect") {
      if ((item->option_list) && (item->option_list->option_name)) {
        char full_filename[FF_MAX_PATH];
        DEBUG(1,"STORE to: %s file: %s",current_status->act_dir,(item->option_list->option_name));
        // store with given size to given adress and optional verification run
        // conf value is the memory size to store and a 1 bit halt flag (LSB)
        if ((item->option_list->conf_value)&1) {
          // halt the core if requested
          OSD_Reset(OSDCMD_CTRL_HALT);
          Timer_Wait(1);
        }
        sprintf(full_filename,"%s%s",current_status->act_dir,item->option_list->option_name);

        CFG_download_rom(full_filename,item->action_value,item->option_list->conf_value>>1);
        current_status->show_menu=0;
        current_status->file_browser=0;
        current_status->popup_menu=0;
        current_status->show_status=0;
        OSD_Disable();
        Timer_Wait(1);
        if ((item->option_list->conf_value)&1) {
          // continue operation of the core if requested
          OSD_Reset(0);
        }
        Timer_Wait(1);
        return 1;

        return 1;
      }
    }
    // backup ----------------------------------
    if MATCH(item->action_name,"backup") {
      CFG_save_all(current_status, current_status->act_dir, "backup.ini");
      current_status->update=1;
      return 1;
    }
    // reset ----------------------------------
    if MATCH(item->action_name,"reset") {
      current_status->popup_menu=1;
      current_status->show_menu=0;
      current_status->show_status=0;
      strcpy(current_status->popup_msg," Reset Target? ");
      current_status->selections = MENU_POPUP_YESNO;
      current_status->selected = 0;
      current_status->update=1;
      current_status->do_reboot=0;
      return 1;
    }
    // reboot ----------------------------------
    if MATCH(item->action_name,"reboot") {
      current_status->popup_menu=1;
      current_status->show_menu=0;
      current_status->show_status=0;
      strcpy(current_status->popup_msg," Reboot Board? ");
      current_status->selections = MENU_POPUP_YESNO;
      current_status->selected = 0;
      current_status->update=1;
      current_status->do_reboot=1;
      return 1;
    }
  }
  // browser execute action ==================================================
  if (mode==2) { // *****
    FF_DIRENT mydir = Filesel_GetEntry(current_status->dir_scan,
                                       current_status->dir_scan->sel);
    // fddselect,0...3 ----------------------------------
    if MATCH(item->action_name,"fddselect") {
      // store selected file in option_name structure

      item->selected_option=item->option_list;


      char file_path[FF_MAX_PATH] = "";
      sprintf(file_path, "%s%s", current_status->act_dir, mydir.FileName);

      FDD_Insert(item->action_value, file_path);

      strncpy(item->option_list->option_name, FDD_GetName(item->action_value), MAX_OPTION_STRING-1);
      item->option_list->option_name[MAX_OPTION_STRING-1]=0;

      current_status->show_menu=1;
      current_status->file_browser=0;

      return 1;
    }
    // hddselect,0...1 ----------------------------------
    if MATCH(item->action_name,"hddselect") {
      // store selected file in option_name structure
      /*item->selected_option=item->option_list;*/
      /*strncpy(item->option_list->option_name,*/
              /*mydir.FileName, MAX_OPTION_STRING-1);*/
      /*item->option_list->option_name[MAX_OPTION_STRING-1]=0;*/
      current_status->show_menu=1;
      current_status->file_browser=0;
      return 1;
    }
    // iniselect ----------------------------------
    if MATCH(item->action_name,"iniselect") {
      // set new INI file and force reload of FPGA configuration
      strcpy(current_status->ini_dir,current_status->act_dir);
      strcpy(current_status->ini_file,mydir.FileName);
      current_status->show_menu=0;
      current_status->popup_menu=0;
      current_status->show_status=0;
      current_status->file_browser=0;
      OSD_Disable();
      // "kill" FPGA content and re-run setup
      current_status->fpga_load_ok=0;
      // set PROG low to reset FPGA (open drain)
      IO_DriveLow_OD(PIN_FPGA_PROG_L);
      Timer_Wait(1);
      IO_DriveHigh_OD(PIN_FPGA_PROG_L);
      Timer_Wait(2);
      // NO update here!
      return 0;
    }
    // loadselect ----------------------------------
    if MATCH(item->action_name,"loadselect") {
      if ((current_status->act_dir[0]) && (mydir.FileName[0])) {
        char full_filename[FF_MAX_PATH];
        // upload with auto-size to given adress and optional verification run
        // conf value is 8 bit format, 1 bit halt flag, 1 bit reset flag, 1 bit verification flag (LSB)
        if ((item->option_list->conf_value>>2)&1) {
          // halt the core if requested
          OSD_Reset(OSDCMD_CTRL_HALT);
          Timer_Wait(1);
        }
        sprintf(full_filename,"%s%s",current_status->act_dir,mydir.FileName);
        CFG_upload_rom(full_filename,item->action_value,0,item->option_list->conf_value&1,(item->option_list->conf_value>>3)&255);
        current_status->show_menu=0;
        current_status->file_browser=0;
        current_status->popup_menu=0;
        current_status->show_status=0;
        OSD_Disable();
        Timer_Wait(1);
        if ((item->option_list->conf_value>>2)&1) {
          // continue operation of the core if requested
          OSD_Reset(0);
        }
        if ((item->option_list->conf_value>>1)&1) {
          // perform soft-reset if requested
          OSD_Reset(OSDCMD_CTRL_RES);
        }
        Timer_Wait(1);
        return 1;
      }
    }
  }
  return 0;
}

// ==============================================
// == PLATFORM INDEPENDENT CODE STARTS HERE !
// ==============================================

void MENU_init_ui(status_t *current_status)
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
  current_status->menu_first = current_status->menu_act;
  current_status->item_first = NULL;
  current_status->menu_last = NULL;
  current_status->item_last = NULL;

  // we may do some initialization here...
  _MENU_action(NULL,current_status,255);

  // clean up both OSD pages
  OSD_SetPage(0);
  OSD_Clear();
  OSD_SetPage(1);
  OSD_Clear();
  OSD_SetDisplay(0);
  OSD_SetPage(0);

  // clean up display flags
  current_status->update=0;
  current_status->show_status=0;
  current_status->show_menu=0;
  current_status->popup_menu=0;
  current_status->file_browser=0;
}

void _MENU_back_dir(char *pPath)
{
  uint32_t len = strlen(pPath);
  uint32_t i = 0;
  if (len<2) {
    _strlcpy(pPath, "\\", FF_MAX_FILENAME);
    return;
  }
  for (i=len-1;i>0;--i) {
    if (pPath[i-1]=='\\') {
       pPath[i] = '\0';
       break;
    }
  }
}

// TODO: colours from INI file instead of hardcoding...?
void _MENU_update_ui(status_t *current_status)
{
  // reset scroll line position initially
  if (current_status->scroll_pos) {
    OSD_SetHOffset(current_status->scroll_pos, 0, 0);
    current_status->scroll_pos=0;
  }

  // clear OSD area and set the minimum header/footer lines we always need
  OSD_Clear();
  OSD_Write  (0,    "      ***              ***      ", 0);
  OSD_WriteRC(1, 0, "                                ", 0, 0, 0x01);
  OSD_WriteRC(14, 0, "                                ", 0, 0, 0x01);

  if (current_status->show_status) {
    // STATUS SCREEN
    int i,j;

    // print status header
    OSD_WriteRC(0, 10,          "REPLAY INFO", 0, 0xE, 0);

    // print fixed status lines and a separator line
    for (i=0; i<3; i++) {
      OSD_WriteRC(2+i, 0, current_status->status[i], 0, 0x07, 0);
    }
    OSD_WriteRC(2+i, 0,   "                                ", 0, 0, 0x01);

    // finally print the "scrolling" info lines, we start with the oldest
    // entry which is always the next entry after the latest (given by idx)
    j=current_status->info_start_idx+1;
    for (i=0; i<8; i++) {
      if (j>7) j=0;
      // do colouring of different messages
      switch (current_status->info[j][0]) {
        case 'W':
          //WARNING: dark yellow text (0x0E would be yellow)
          OSD_WriteRC(6+i, 0, current_status->info[j++], 0, 0x06, 0);
          break;
        case 'E':
          //ERROR: red text (0x04 would be dark red)
          OSD_WriteRC(6+i, 0, current_status->info[j++], 0, 0x0C, 0);
          break;
        case 'I':
          //INFO: gray text (0x0F would be white)
          OSD_WriteRC(6+i, 0, current_status->info[j++], 0, 0x07, 0);
          break;
        default:
          //DEBUG: blue text (0x01 would be dark blue)
          OSD_WriteRC(6+i, 0, current_status->info[j++], 0, 0x09, 0);
      }
    }

    // print status line
    OSD_WriteRC(MENU_STATUS, 0, " LEFT/RIGHT - F12/replaybtn/ESC", 0, 0x0f, 0);
  }
  else if (current_status->popup_menu) {
    // POP UP MESSAGE HANDLER

    uint8_t startline=(16-MENU_POPUP_HEIGHT)>>1;
    uint8_t startcol=(32-MENU_POPUP_WIDTH)>>1;
    uint8_t row;

    // create pop-up area
    for(row=startline;row<startline+MENU_POPUP_HEIGHT;row++) {
      char line[]="                                ";
      line[MENU_POPUP_WIDTH]=0;
      OSD_WriteRC(row, startcol, line, 0, 0, 0x04);
    }

    // show_menu pop-up message
    OSD_WriteRC(startline+1, startcol+1, current_status->popup_msg ,
                0, 0x0f, 0x04);

    // show_menu selectors
    if (current_status->selections==MENU_POPUP_YESNO) {
      OSD_WriteRC(row-2, 13, "yes no", 0, 0x0f, 0x04);
      if (current_status->selected)
        OSD_WriteRC(row-2, 13, "yes", 0, 0x04, 0x0f);
      else
        OSD_WriteRC(row-2, 17, "no", 0, 0x04, 0x0f);
    }
    if (current_status->selections==MENU_POPUP_SAVEIGNORE) {
      OSD_WriteRC(row-2, 11, "save ignore", 0, 0x0f, 0x04);
      if (current_status->selected)
        OSD_WriteRC(row-2, 11, "save", 0, 0x04, 0x0f);
      else
        OSD_WriteRC(row-2, 16, "ignore", 0, 0x04, 0x0f);
    }

  }
  else {
    // SETUP and FILEBROWSER HANDLER
    menuitem_t *pItem=current_status->item_first;
    uint8_t line=0, row=2;

    // clear OSD area and set some generic stuff
    OSD_Clear();
    OSD_Write  (0,                "      ***              ***      ",0);
    OSD_WriteRC(1, 0,             "                                ",0,0,0x01);
    OSD_WriteRC(MENU_STATUS-1, 0, "                                ",0,0,0x01);

    if (current_status->file_browser) {
      // handling file browser part
      OSD_WriteRC(0, 10, "FILE BROWSER", 0, 0xE, 0);

      uint32_t len;
      uint32_t i;
      char *filename;
      FF_DIRENT entry;

      // show_menu file/directory list
      uint8_t sel = Filesel_GetSel(current_status->dir_scan);
      for (i = 0; i < MENU_HEIGHT; i++) {
        char s[OSDMAXLEN+1];
        s[OSDMAXLEN] = 0; // set temporary string length to OSD line length
        memset(s, ' ', OSDMAXLEN); // clear line buffer
        entry = Filesel_GetLine(current_status->dir_scan, i);
        filename = entry.FileName;

        len = strlen(filename);
        if (i==sel) {
          // FIX ME for really long names
          if (len > OSDLINELEN) { // enable scroll if long name
            OSD_SetHOffset(i+2, 0, 0);
            current_status->scroll_pos=i+2;
          }
        }
        if (entry.Attrib & FF_FAT_ATTR_DIR) {
          // a directory
          strncpy(s + 1, filename, len);
          OSD_WriteRC(i+2, 0, s, i==sel, 0xA, 0);
        } else {
          // a file
          strncpy(s + 1, filename, len);
          OSD_WriteRC(i+2, 0, s, i==sel, 0xB, 0);
        }

        if (i == 1 && current_status->dir_scan->total_entries == 0) {
          strcpy(s, "            No files!");
          OSD_WriteRC(i+2, 0, s, i==sel, 0xC, 0);
        }
      }
      // print status line
      OSD_WriteRC(MENU_STATUS, 0, " UP/DOWN/RET/ESC - F12/replaybtn",0,0x0f,0);
    } else {
      // handling replay setup part
      OSD_WriteRC(0, 10, "REPLAY SETUP", 0, 0xC, 0);
      if (!pItem) {
        // print  "selected" menu title (we pan through menus)
        OSD_WriteRC(row++, MENU_INDENT, current_status->menu_act->menu_title,
                                         1, 0xE, 0);
        pItem=current_status->menu_act->item_list;
        current_status->menu_item_act=NULL;
      } else {
        // print  "not selected" menu title  (we select and modify items)
        OSD_WriteRC(row++, MENU_INDENT, current_status->menu_act->menu_title,
                                         0, 0xE, 0);
      }
      // show the item_name list to browse
      while (pItem && ((line++)<(MENU_HEIGHT-1))) {
        OSD_WriteRC(row++, MENU_ITEM_INDENT, pItem->item_name, 0, 0xB, 0);
        // no selection yet set, set to first option in list as default
        if (!pItem->selected_option) pItem->selected_option=pItem->option_list;
        // check for action before displaying
        _MENU_action(pItem,current_status,0);
        // check if we really have an option_name, otherwise show placeholder
        if (pItem->selected_option->option_name[0])
          OSD_WriteRC(row-1, MENU_OPTION_INDENT,
                      pItem->selected_option->option_name,
                      pItem==current_status->menu_item_act?1:0, 0xA, 0);
        else
          OSD_WriteRC(row-1, MENU_OPTION_INDENT, " ",
                      pItem==current_status->menu_item_act?1:0, 0xA, 0);
        // keep last item_name shown
        current_status->item_last = pItem;
        pItem = pItem->next;
      }
      // print status line
      OSD_WriteRC(MENU_STATUS, 0, "UP/DWN/LE/RI - F12/replaybtn/ESC",0,0x0f,0);
    }
  }
}

uint8_t MENU_handle_ui(uint16_t key, status_t *current_status)
{ // flag OSD update may be set already externally
  uint8_t update=current_status->update;
  current_status->update=0;
  static uint32_t osd_timeout;
  static uint8_t osd_timeout_cnt=0;

  static uint32_t scroll_timer;
  static uint16_t  scroll_offset=0;

  // timeout of OSD after noticing last key press (or external forced update)
  if ((current_status->show_menu || current_status->popup_menu ||
       current_status->file_browser || current_status->show_status)) {
    if (key || update) {
      // we set our timeout with any update (1 sec)
      osd_timeout=Timer_Get(1000);
      osd_timeout_cnt=0;
    }
    else if (Timer_Check(osd_timeout)) {
      if (osd_timeout_cnt++ < 30) {
        // we set our timeout again with any update (1 sec)
        osd_timeout=Timer_Get(1000);
        return 0;
      } else {
        // hide menu after ~30 sec
        current_status->show_menu=0;
        current_status->popup_menu=0;
        current_status->show_status=0;
        current_status->file_browser=0;
        OSD_Disable();
        return 0;
      }
    }
  } else {
    if (update) {
      current_status->show_menu=0;
      current_status->popup_menu=0;
      current_status->show_status=1;
      current_status->file_browser=0;
      osd_timeout=Timer_Get(1000);
      // show half the time
      osd_timeout_cnt=15;
    }
  }

  // --------------------------------------------------

  if (key==KEY_MENU) {
    // OSD menu handling, switch it on/off

    if (current_status->show_menu || current_status->popup_menu ||
        current_status->file_browser || current_status->show_status) {
      // hide menu
      current_status->show_menu=0;
      current_status->popup_menu=0;
      current_status->show_status=0;
      current_status->file_browser=0;
      OSD_Disable();
      return 0;
    }
    else {
      // show menu (if there is something to show)
      if (current_status->menu_top) {
        current_status->show_menu=0;
        current_status->popup_menu=0;
        current_status->show_status=1;
        current_status->file_browser=0;
        update=1;
        // set timeout
        osd_timeout=Timer_Get(1000);
        osd_timeout_cnt=0;
      }
    }
  }

  // --------------------------------------------------

  else if (current_status->popup_menu) {
    // special pop-up handler

    if ((key == KEY_LEFT) || (key == KEY_RIGHT)) {
      current_status->selected=current_status->selected==1?0:1;
      update=1;
    }
    if (key == KEY_ENTER) {
        // ok, we don't have anything set yet to handle the result
        current_status->show_menu=0;
        current_status->popup_menu=0;
        current_status->show_status=0;
        OSD_Disable();
        if (current_status->selected) {
          return _MENU_update(current_status);
        }
        return 0;
    }
    if (key == KEY_ESC) {
        current_status->popup_menu=0;
        current_status->show_menu=1;
        current_status->show_status=0;
        update=1;
    }
  }

  // --------------------------------------------------

  else if (current_status->file_browser) {
    // file browser handling

    if ((key  & ~KF_REPEATED) == KEY_UP) {
      Filesel_Update(current_status->dir_scan, SCAN_PREV);
      update=1;
    }
    if ((key  & ~KF_REPEATED) == KEY_DOWN) {
      Filesel_Update(current_status->dir_scan, SCAN_NEXT);
      update=1;
    }
    /* does not really work with filebrowser code (TODO: check why...)
    if ((key  & ~KF_REPEATED) == KEY_PGUP) {
      Filesel_Update(current_status->dir_scan, SCAN_PREV_PAGE);
      update=1;
    }
    if ((key  & ~KF_REPEATED) == KEY_PGDN) {
      Filesel_Update(current_status->dir_scan, SCAN_NEXT_PAGE);
      update=1;
    }
    */
    if (key == KEY_ENTER) {
      FF_DIRENT mydir = Filesel_GetEntry(current_status->dir_scan,
                                         current_status->dir_scan->sel);

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
          update=1;
        }
      }
      else {
        // check for filebrowser action and update display if succesfully
        update = _MENU_action(current_status->menu_item_act,
                              current_status, 2);
      }
    }
    if (key == KEY_ESC) {
      // quit filebrowsing w/o changes
      current_status->file_browser=0;
      current_status->show_menu=1;
      update=1;
    }
  }

  // --------------------------------------------------

  else if ((current_status->show_status) && (current_status->fpga_load_ok!=2)) {
    if ((key  & ~KF_REPEATED) == KEY_RIGHT) {
      // go to last menu entry
      while (current_status->menu_act->last) {
        current_status->menu_act=current_status->menu_act->last;
      }
      current_status->show_menu=1;
      current_status->show_status=0;
      update=1;
    }
    if ((key  & ~KF_REPEATED) == KEY_LEFT) {
      // go to first menu entry
      while (current_status->menu_act->next) {
        current_status->menu_act=current_status->menu_act->next;
      }
      current_status->show_menu=1;
      current_status->show_status=0;
      update=1;
    }
  }
  else if (current_status->show_menu) {
    // further processing if menu is visible

    if ((key  & ~KF_REPEATED) == KEY_RIGHT) {
      // option_name select or enter item_name list
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
          current_status->menu_act=current_status->menu_act->next;
        } else {
          // we had the last entry, so switch to status page
          current_status->show_menu=0;
          current_status->show_status=1;
        }
      }
      update=1;
    }
    if ((key  & ~KF_REPEATED) == KEY_LEFT) {
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
          current_status->show_menu=0;
          current_status->show_status=1;
        }
      }
      update=1;
    }
    if ((key  & ~KF_REPEATED) == KEY_UP) {
      // scoll menu/item_name list
      if (current_status->item_first) {
        // on item_name level
        if (current_status->menu_item_act==current_status->item_first) {
          // we are on top
          if (current_status->menu_item_act->last) {
            current_status->menu_item_act =
                                          current_status->menu_item_act->last;
            current_status->item_first = current_status->menu_item_act;
          } else {
            // "exit" from item_name list
            current_status->item_first=NULL;
            current_status->menu_item_act=NULL;
          }
        }
        else {
          // we are in the middle
          if (current_status->menu_item_act->last) {
            current_status->menu_item_act =
                                          current_status->menu_item_act->last;
          }
        }
      }
      update=1;
    }
    if ((key  & ~KF_REPEATED) == KEY_DOWN) {
      // scoll menu/item_name list
      if (current_status->item_first) {
        // on item_name level
        if (current_status->menu_item_act==current_status->item_last) {
          // we are at the end
          if (current_status->menu_item_act->next) {
            current_status->item_first=current_status->item_first->next;
            current_status->menu_item_act=current_status->menu_item_act->next;
          }
        }
        else {
          // we are in the middle
          if (current_status->menu_item_act->next) {
            current_status->menu_item_act=current_status->menu_item_act->next;
          }
        }
      }
      else {
        // on menu level
        if (current_status->menu_act->item_list->item_name[0]) {
          current_status->item_first=current_status->menu_act->item_list;
          current_status->menu_item_act = current_status->item_first;
        }
      }
      update=1;
    }
    // step into a item_name list from menu list (see also KEY_RIGHT)
    if (key == KEY_ENTER) {
        if (current_status->menu_item_act->action_name[0]) {
          // check for menu action and update display if succesfully
          update = _MENU_action(current_status->menu_item_act,
                                current_status, 1);
        }
    }
    // step out of item_name list to menu list
    if (key == KEY_ESC) {
      if (current_status->item_first) {
        // exit from item_name list
        current_status->item_first=NULL;
        current_status->menu_item_act=NULL;
        update=1;
      }
      else {
        // exit from menu, but ask what to do
        current_status->popup_menu=1;
        current_status->show_menu=0;
        current_status->show_status=0;
        strcpy(current_status->popup_msg," Reset Target? ");
        current_status->selections = MENU_POPUP_YESNO;
        current_status->selected = 0;
        current_status->do_reboot=0;
        update=1;
      }
    }
  }
  // update menu if needed
  if (update) {
    _MENU_update_ui(current_status);
    // for testing pg 0 is enabled, in pg 1 scroll seems to be broken (in FPGA)
    // --> can check this by setting next two parameters directly to 1
    OSD_SetDisplay(0); //  OSD_GetPage());
    OSD_SetPage(0); //     OSD_NextPage());
    scroll_timer = Timer_Get(20); // restart scroll timer
    scroll_offset = 0; // reset scroll position
    if ((current_status->show_menu || current_status->popup_menu ||
         current_status->file_browser || current_status->show_status)) {
      OSD_Enable(DISABLE_KEYBOARD); // just in case, enable OSD and disable KEYBOARD for core
    }
  } else {
    // scroll if needed
    if (current_status->scroll_pos) {
      if (Timer_Check(scroll_timer)) { // scroll if timer elapsed
          scroll_timer = Timer_Get(20); // restart scroll timer
          scroll_offset= (scroll_offset+1) & ((OSDMAXLEN<<4)-1); // scrolling
          OSD_WaitVBL();
          OSD_SetHOffset(current_status->scroll_pos,
                         (uint8_t) (scroll_offset >> 4),
                         (uint8_t) (scroll_offset & 0xF));
          DEBUG(3, "Offset: %d",scroll_offset);
      }
    }
  }
  return update;
}

void _MENU_update_bits(status_t *current_status)
{
  menu_t *pMenu=current_status->menu_top;
  uint32_t staticbits = current_status->config_s;
  uint32_t dynamicbits = current_status->config_d;

  DEBUG(2,"OLD config - S:%08lx D:%08lx",staticbits,dynamicbits);

  // run through config structure and update configuration settings
  while (pMenu) {
    menuitem_t *pItem=pMenu->item_list;
    while (pItem) {
      DEBUG(3,"%s: ",pItem->item_name);
      // we add only items which are no action item and got selected once
      // only this entries will (must) have an item mask and an option value
      if ((!pItem->action_name[0]) && pItem->selected_option) {
        DEBUG(3,"%08lx ",pItem->conf_mask);
        if (pItem->conf_dynamic) {
          DEBUG(3,"%08lx dyn",pItem->selected_option->conf_value);
          dynamicbits &= ~pItem->conf_mask; //(pItem->conf_mask ^ 0xffffffff);
          dynamicbits |= pItem->selected_option->conf_value;
        }
        else {
          DEBUG(3,"%08lx stat",pItem->selected_option->conf_value);
          staticbits &= ~pItem->conf_mask; //(pItem->conf_mask ^ 0xffffffff);
          staticbits |= pItem->selected_option->conf_value;
        }
      }
      DEBUG(3,"");
      pItem = pItem->next;
    }
    pMenu = pMenu->next;
  }
  DEBUG(2,"NEW config - S:%08lx D:%08lx",staticbits,dynamicbits);
  current_status->config_s = staticbits;
  current_status->config_d = dynamicbits;

  // send bits to FPGA
  OSD_ConfigSendUserD(dynamicbits);
  OSD_ConfigSendUserS(staticbits);
}

uint8_t _MENU_update(status_t *current_status) {
  DEBUG(1,"Config accepted: S:%08lx D:%08lx / %d",current_status->config_s,
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
  } else {
    // perform soft-reset
    OSD_Reset(OSDCMD_CTRL_RES);
    Timer_Wait(1);
    // we should not need this, but just in case...
    OSD_Reset(OSDCMD_CTRL);
    Timer_Wait(100);
    // fall back to status screen
    current_status->update=1;
    current_status->show_status=1;
    current_status->show_menu=0;
    current_status->popup_menu=0;
    current_status->file_browser=0;
  }
  return 1;
}

