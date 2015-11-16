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
#include "hardware.h"
#include "filesel.h"
#include "fileio.h"
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

    // cha select,0...3 ----------------------------------
    if MATCH(item->action_name,"cha_select") {
      if ((current_status->fileio_cha_ena >> item->action_value) & 1) { // protects agains illegal values also
        if (!FileIO_FCh_GetInserted(0,item->action_value)) {
          if (current_status->fileio_cha_mode == REMOVABLE)
            strcpy(item->selected_option->option_name,"<RETURN> to set");
          else
            strcpy(item->selected_option->option_name,"Not mounted");
        } else {
          strncpy(item->selected_option->option_name, FileIO_FCh_GetName(0,item->action_value), MAX_OPTION_STRING-1);
        }
      } else {
        item->selected_option->option_name[0]=0;
      }
      return 1;
    }
    // chb select,0...3 ----------------------------------
    if MATCH(item->action_name,"chb_select") {
      if ((current_status->fileio_chb_ena >> item->action_value) & 1) { // protects agains illegal values also
        if (!FileIO_FCh_GetInserted(1,item->action_value)) {
          if (current_status->fileio_chb_mode == REMOVABLE)
            strcpy(item->selected_option->option_name,"<RETURN> to set");
          else
            strcpy(item->selected_option->option_name,"Not mounted");
        } else {
          strncpy(item->selected_option->option_name, FileIO_FCh_GetName(1,item->action_value), MAX_OPTION_STRING-1);
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
    // fileio cha select
    if MATCH(item->action_name,"cha_select") {
      if ((current_status->fileio_cha_ena >> item->action_value) & 1) {
        if (FileIO_FCh_GetInserted(0,item->action_value)) {
          if (current_status->fileio_cha_mode == REMOVABLE) {
            // deselect file
            strcpy(item->selected_option->option_name,"<RETURN> to set");
            item->selected_option=NULL;
            FileIO_FCh_Eject(0,item->action_value);
            return 1;
          } else {
            // tried to eject fixed drive... nothing for now.
          }
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

    // fileio chb select
    if MATCH(item->action_name,"chb_select") {
      if ((current_status->fileio_chb_ena >> item->action_value) & 1) {
        if (FileIO_FCh_GetInserted(1,item->action_value)) {
          if (current_status->fileio_chb_mode == REMOVABLE) {
          // deselect file
            strcpy(item->selected_option->option_name,"<RETURN> to set");
            item->selected_option=NULL;
            FileIO_FCh_Eject(1,item->action_value);
            return 1;
          } else {
            // tried to eject fixed drive... nothing for now.
          }
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
    // insert is ok for fixed or removable
    // cha_select
    if MATCH(item->action_name,"cha_select") {
      if ((current_status->fileio_cha_ena >> item->action_value) & 1) {
        item->selected_option=item->option_list;

        char file_path[FF_MAX_PATH] = "";
        sprintf(file_path, "%s%s", current_status->act_dir, mydir.FileName);
        FileIO_FCh_Insert(0,item->action_value, file_path);

        strncpy(item->option_list->option_name, FileIO_FCh_GetName(0,item->action_value), MAX_OPTION_STRING-1);
        item->option_list->option_name[MAX_OPTION_STRING-1]=0;

        current_status->show_menu=1;
        current_status->file_browser=0;
        return 1;
      }
    }
    // chb_select
    if MATCH(item->action_name,"chb_select") {
      if ((current_status->fileio_chb_ena >> item->action_value) & 1) {
        item->selected_option=item->option_list;

        char file_path[FF_MAX_PATH] = "";
        sprintf(file_path, "%s%s", current_status->act_dir, mydir.FileName);
        FileIO_FCh_Insert(1,item->action_value, file_path);

        strncpy(item->option_list->option_name, FileIO_FCh_GetName(1,item->action_value), MAX_OPTION_STRING-1);
        item->option_list->option_name[MAX_OPTION_STRING-1]=0;

        current_status->show_menu=1;
        current_status->file_browser=0;
        return 1;
      }
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

        uint32_t staticbits = current_status->config_s;
        uint32_t dynamicbits = current_status->config_d;
        DEBUG(1,"OLD config - S:%08lx D:%08lx",staticbits,dynamicbits);
        CFG_upload_rom(full_filename,item->action_value,0,item->option_list->conf_value&1,(item->option_list->conf_value>>3)&255,&staticbits,&dynamicbits);
        DEBUG(1,"NEW config - S:%08lx D:%08lx",staticbits,dynamicbits);
        current_status->config_s = staticbits;
        //not used yet: current_status->config_d = dynamicbits;
        // send bits to FPGA
        OSD_ConfigSendUserS(staticbits);
        //not used yet: OSD_ConfigSendUserD(dynamicbits);

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
  OSD_SetPage(1);

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
  uint8_t col = 0;

  current_status->scroll_pos=0;

  // clear OSD area and set the minimum header/footer lines we always need
  OSD_Clear();
  OSD_Write  ( 0,    "      ***              ***      ", 0);
  OSD_WriteRC( 1, 0, "                                ", 0, 0, 0x01);
  OSD_WriteRC(14, 0, "                                ", 0, 0, 0x01);

  if (current_status->show_status) {
    //
    // STATUS SCREEN
    //
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
          //DEBUG: cyan text
          OSD_WriteRC(6+i, 0, current_status->info[j++], 0, 0x03, 0);
      }
    }

    // print status line
    OSD_WriteRC(MENU_STATUS, 0, " LEFT/RIGHT - F12/replaybtn/ESC", 0, 0x0f, 0);
  }
  else if (current_status->popup_menu) {
    //
    // POP UP MESSAGE HANDLER
    //
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
    //
    // SETUP and FILEBROWSER HANDLER
    //
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

      uint32_t i;
      char *filename;
      FF_DIRENT entry;

      // show filter in header
      uint8_t sel = Filesel_GetSel(current_status->dir_scan);
      if (current_status->dir_scan->file_filter_len) {
        char s[OSDMAXLEN+1];
        strncpy (s, current_status->dir_scan->file_filter, OSDMAXLEN);
        s[current_status->dir_scan->file_filter_len]='*';
        s[current_status->dir_scan->file_filter_len+1]=0;
        OSD_WriteRC(1, 0, s ,0,0xF,1);
      }

      // show file/directory list
      if (current_status->dir_scan->total_entries == 0) {
        // nothing there
        char s[OSDMAXLEN+1];
        strcpy(s, "            No files                   ");
        OSD_WriteRC(1+2, 0, s, 1, 0xC, 0);
      } else {
        for (i = 0; i < MENU_HEIGHT; i++) {
          char s[OSDMAXLEN+1];
          memset(s, ' ', OSDMAXLEN); // clear line buffer
          s[OSDMAXLEN] = 0; // set temporary string length to OSD line length

          entry = Filesel_GetLine(current_status->dir_scan, i);
          filename = entry.FileName;

          uint16_t len = strlen(filename);
          if (i==sel) {
            if (len > OSDLINELEN) { // enable scroll if long name
              strncpy (current_status->scroll_txt, filename, FF_MAX_FILENAME);
              current_status->scroll_pos=i+2;
              current_status->scroll_len=len;
            }
          }

          if (entry.Attrib & FF_FAT_ATTR_DIR) {
            // a directory
            strncpy (s, filename, OSDMAXLEN);
            OSD_WriteRC(i+2, 0, s, i==sel, 0xA, 0);
          } else {
            // a file
            strncpy (s, filename, OSDMAXLEN);
            OSD_WriteRC(i+2, 0, s, i==sel, 0xB, 0);
          }

        }
      }
      // print status line
      OSD_WriteRC(MENU_STATUS, 0, " UP/DOWN/RET/ESC - F12/replaybtn",0,0x0f,0);
    } else {
      //
      // handling replay setup part (general menus)
      //
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

        OSD_WriteRCt(row++, MENU_ITEM_INDENT, pItem->item_name, MENU_OPTION_INDENT-MENU_ITEM_INDENT, 0, 0xB, 0);

        // temp bodge to show read only flags
        if MATCH(pItem->action_name,"cha_select") {
          if ((current_status->fileio_cha_ena >> pItem->action_value) & 1) {
            if (FileIO_FCh_GetReadOnly(0,pItem->action_value)) {
              OSD_WriteRC(row-1, 0, "R", 0, 0xA, 0); // Green>
            } else if (FileIO_FCh_GetProtect(0,pItem->action_value)) {
              OSD_WriteRC(row-1, 0, "P", 0, 0xA, 0); // Green>
            }
          }
        }

        if MATCH(pItem->action_name,"chb_select") {
          if ((current_status->fileio_chb_ena >> pItem->action_value) & 1) {
            if (FileIO_FCh_GetReadOnly(1,pItem->action_value)) {
              OSD_WriteRC(row-1, 0, "R", 0, 0xA, 0); // Green>
            } else if (FileIO_FCh_GetProtect(1,pItem->action_value)) {
              OSD_WriteRC(row-1, 0, "P", 0, 0xA, 0); // Green>
            }
          }
        }

        // no selection yet set, set to first option in list as default
        if (!pItem->selected_option) pItem->selected_option=pItem->option_list;
        // check for action before displaying
        _MENU_action(pItem,current_status,0);

        col = 0xA; // green
        if (pItem->conf_dynamic) col = 0xF; // White

        // check if we really have an option_name, otherwise show placeholder
        if (pItem->selected_option->option_name[0])
          OSD_WriteRCt(row-1, MENU_OPTION_INDENT,
                      pItem->selected_option->option_name,
                      MENU_WIDTH-MENU_OPTION_INDENT, // len limit
                      pItem==current_status->menu_item_act?1:0, col, 0);
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
  static uint8_t  osd_timeout_cnt=0;

  static uint32_t scroll_timer;
  static uint16_t scroll_text_offset=0;
  static uint8_t  scroll_started=0;

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
    if ((key  & ~KF_REPEATED) == KEY_PGUP) {
      Filesel_Update(current_status->dir_scan, SCAN_PREV_PAGE);
      update=1;
    }
    if ((key  & ~KF_REPEATED) == KEY_PGDN) {
      Filesel_Update(current_status->dir_scan, SCAN_NEXT_PAGE);
      update=1;
    }
    if ( ((key >= '0') && (key<='9')) || ((key >= 'A') && (key<='Z')) ) {
      Filesel_AddFilterChar(current_status->dir_scan, key&0x7F);
      Filesel_ScanFirst(current_status->dir_scan); // scan first
      update=1;
    }
    if ( key==KEY_BACK ) { // backspace removes a letter from the filter string
      Filesel_DelFilterChar(current_status->dir_scan);
      Filesel_ScanFirst(current_status->dir_scan); // scan first
      update=1;
    }
    // quickly remove filter and rescan
    if (key == KEY_HOME) {
      Filesel_ChangeDir(current_status->dir_scan, current_status->act_dir);
      Filesel_ScanFirst(current_status->dir_scan); // scan first
      update=1;
    }
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
        // check for filebrowser action and update display if successful
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
    // change file attributes
    if (key == 'P' ) {
        if MATCH(current_status->menu_item_act->action_name,"cha_select") {
          if ((current_status->fileio_cha_ena >> current_status->menu_item_act->action_value) & 1) {
            FileIO_FCh_TogProtect(0, current_status->menu_item_act->action_value);
            update=1;
          }
        }

        if MATCH(current_status->menu_item_act->action_name,"chb_select") {
          if ((current_status->fileio_chb_ena >> current_status->menu_item_act->action_value) & 1) {
            FileIO_FCh_TogProtect(1, current_status->menu_item_act->action_value);
            update=1;
          }
        }

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

    OSD_SetDisplay(OSD_GetPage()); //  OSD_GetPage());
    OSD_SetPage(OSD_NextPage());   //  OSD_NextPage());

    scroll_timer = Timer_Get(1000); // restart scroll timer
    scroll_started = 0;

    if ((current_status->show_menu || current_status->popup_menu ||
         current_status->file_browser || current_status->show_status)) {
      OSD_Enable(DISABLE_KEYBOARD); // just in case, enable OSD and disable KEYBOARD for core
    }
  } else {
    // scroll if needed
    if (current_status->file_browser && current_status->scroll_pos) {
      uint16_t len    = current_status->scroll_len;

      if (!scroll_started) {
        if (Timer_Check(scroll_timer)) { // scroll if timer elapsed
          scroll_started = 1;
          scroll_text_offset = 0;
          /*scroll_pix_offset  = 0;*/

          OSD_WriteScroll(current_status->scroll_pos, current_status->scroll_txt, 0, len, 1, 0xB, 0);
          scroll_timer = Timer_Get(20); // restart scroll timer
        }
      } else if (Timer_Check(scroll_timer)) { // scroll if timer elapsed

          scroll_timer = Timer_Get(20); // restart scroll timer

          scroll_text_offset = scroll_text_offset + 2;

          if (scroll_text_offset >= (len + OSD_SCROLL_BLANKSPACE) << 4)
            scroll_text_offset = 0;

          if ((scroll_text_offset & 0xF) == 0)
            OSD_WriteScroll(current_status->scroll_pos, current_status->scroll_txt, scroll_text_offset>>4, len, 1, 0xB, 0);

          OSD_SetHOffset(current_status->scroll_pos,0, (uint8_t) (scroll_text_offset & 0xF));
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

