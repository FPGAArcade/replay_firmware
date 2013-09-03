        /*{0x08,0x08,0x1C,0x1C,0x3E,0x3E,0x7F,0x7F},         // 16   [0x10] left arrow*/
        /*{0x1C,0x1C,0x1C,0x1C,0x1C,0x1C,0x00,0x00},         // 17   [0x11]*/
        /*{0x00,0x00,0x1C,0x1C,0x1C,0x1C,0x1C,0x1C},         // 18   [0x12] right arrow*/
        /*{0x7F,0x7F,0x3E,0x3E,0x1C,0x1C,0x08,0x08},         // 19   [0x13]*/
        /*{0x00,0x08,0x08,0x0C,0x0C,0x0E,0x7E,0x7F},         // 20   [0x14] up arrow*/
        /*{0x7F,0x7E,0x0E,0x0C,0x0C,0x08,0x08,0x00},         // 21   [0x15]*/
        /*{0x00,0x08,0x08,0x18,0x18,0x38,0x3F,0x7F},         // 22   [0x16] down arrow*/
        /*{0x7F,0x3F,0x38,0x18,0x18,0x08,0x08,0x00},         // 23   [0x17]*/

#include "amiga_fdd.h"
#include "amiga_hdd.h"
#include "config.h"
#include "osd.h"
#include "hardware.h"
//#include "fileio.h"
#include "amiga_menu.h"
#include "filesel.h"

// OSD is 32 by 8 chars
// time delay after which file/dir name starts to scroll
#define SCROLL_DELAY 500

unsigned long scroll_offset; // file/dir name scrolling position
unsigned long scroll_timer;  // file/dir name scrolling timer

unsigned char menustate = MENU_NONE1;
unsigned char menusub = 0;
unsigned long menu_timer;

extern unsigned char drives; // in fdd.c
extern adfTYPE df[4];  // in fdd.c

extern configTYPE amiga_config; // config.c
char s[40];

#define ERROR_NONE 0
#define ERROR_FILE_NOT_FOUND 1
#define ERROR_INVALID_DATA 2
#define ERROR_UPDATE_FAILED 3

unsigned char Error;

extern FF_IOMAN *pIoman;
extern uint8_t *pFileBuf;
char dir_path[FF_MAX_PATH] = "\\";
char file_path[FF_MAX_PATH] = "\\";

tDirScan dir;

extern const char version[];

unsigned char fs_Options;
unsigned char fs_MenuSelect;
unsigned char fs_MenuCancel;

extern const char *config_filter_msg[];
extern const char *config_memory_chip_msg[] ;
extern const char *config_memory_slow_msg[] ;
extern const char *config_memory_fast_msg[] ;
extern const char *config_memory_xram_msg[] ;
extern const char *config_scanlines_msg[] ;
extern const char *config_cpu_msg[];
extern const char *config_chipset_msg[];

//{{
configTYPE amiga_config;

const char *config_filter_msg[] =  {"none", "HOR ", "VER ", "H+V "};
const char *config_memory_chip_msg[] = {"0.5 MB", "1.0 MB", "1.5 MB", "2.0 MB"};
const char *config_memory_slow_msg[] = {"none", "0.5 MB", "1.0 MB", "1.5 MB"};
const char *config_memory_fast_msg[] = {"none", "2 MB", "4 MB", "8 MB"};
const char *config_memory_xram_msg[] = {"none", "48 MB FAST", "48 MB CHIP", "----"};
const char *config_scanlines_msg[] = {"off", "dim", "blk"};
const char *config_cpu_msg[] = {"68000", "68010", "68EC020", "68020"};
const char *config_chipset_msg[] = {"OCS-A500", "OCS-A1000", "ECS", "AGA"};



 //     0 : video mode [0: PAL  1: NTSC]
 //     1 : HDC enable
 //     2 : Master HDD enable
 //     3 : Slave HDD enable
 // 5.. 4 : floppy drive number
 //     6 : Floppy speed mode [0: normal  1: fast]
 //     7 :
 //     8 : CPU speed mode [0: normal  1: turbo] (also enables fast blitter mode)
 //     9 : CPU cache enable
 //    10 : CPU cache prefetch enable
 //    11 :
 //13..12 : CPU type [0: 000 1: 010 2: EC020 3: 020]
 //    14 :
 //    15 :
 //17..16 : chipset type [0: OCS 1: OCSA1K 2: ECS 3: AGA]
 //19..18 : chip memory size [0: 0.5M 1: 1.0M 2: 1.5M 3: 2.0M]
 //21..20 : slow memory size [0: ---- 1: 0.5M 2: 1.0M 3: 1.5M]
 //23..22 : fast memory size [0: ---- 1: 2.0M 2: 4.0M 3: 8.0M]
 //25..24 : xram memory type [0: ---- 1: FAST 2: CHIP 3: ----]
 //    26 :
 //    27 :
 //    28 :
 //    29 :
 //    30 :
 //    31 :
 //--
 //33..32 : lores filter type
 //35..34 : hires filter type
 //37..36 : scanlines type [0: OFF 1: DIM 2: BLK 3: ---]
 //39..38 : Display mode 0 = 15KHz 1 = 30KHz

void ConfigUpdate()
{
  // pack into generic FPGA config word

  uint32_t configD = 0;
  uint32_t configS = 0;

  configS =
    ((amiga_config.chipset.mode        & 0x1) <<  0) |
    ((amiga_config.hdc.enabled         & 0x1) <<  1) |
    ((amiga_config.hardfile[0].present &
      amiga_config.hardfile[0].enabled & 0x1) <<  2) |
    ((amiga_config.hardfile[1].present &
      amiga_config.hardfile[1].enabled & 0x1) <<  3) |
    ((amiga_config.floppy.drives       & 0x3) <<  4) |
    ((amiga_config.floppy.speed        & 0x1) <<  6) |
    ((amiga_config.cpu.speed           & 0x1) <<  8) |
    ((amiga_config.cpu.cache           & 0x1) <<  9) |
    ((amiga_config.cpu.prefetch        & 0x1) << 10) |
    ((amiga_config.cpu.type            & 0x3) << 12) |
    ((amiga_config.chipset.type        & 0x3) << 16) |
    ((amiga_config.memory.chip         & 0x3) << 18) |
    ((amiga_config.memory.slow         & 0x3) << 20) |
    ((amiga_config.memory.fast         & 0x3) << 22) |
    ((amiga_config.memory.xram         & 0x3) << 24);

  configD =
    ((amiga_config.filter.lores        & 0x3) <<  0) |
    ((amiga_config.filter.hires        & 0x3) <<  2) |
    ((amiga_config.scanlines           & 0x3) <<  4) |
    ((amiga_config.scandouble          & 0x3) <<  6);

  OSD_ConfigSendUser(configD, configS);
}

//}}

void InitUI(void)
{
  // set up initial state
  menustate = MENU_NONE1;
  menusub = 0;
}

void InitUIFile(void)
{
  sprintf(dir_path, "\\"); // root
  Filesel_Init(&dir, dir_path, "*");
}


static void SelectFile(char* pExt, unsigned char MenuSelect, unsigned char MenuCancel)
{
    // this function displays file selection menu
    uint32_t len = strlen(pExt);
    if (len>3) len=3;

    if (strncmp(pExt, dir.file_ext, len) != 0) { // check desired file extension
      Filesel_Init(&dir, dir_path, pExt); // sets path
      Filesel_ScanFirst(&dir); // scan first
    }

    fs_MenuSelect = MenuSelect;
    fs_MenuCancel = MenuCancel;

    menustate = MENU_FILE_SELECT1;

}

static void BackDir(char* pPath)
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

uint8_t HandleUI(void)
{
    unsigned short c;
    unsigned char i, up, down, select, menu, right, left;
    unsigned long len;
    static hardfileTYPE t_hardfile[2]; // temporary copy of former hardfile configuration

    // get user control codes
    c = OSD_GetKeyCode();
    //OsdWaitVBL();

    // decode and set events
    up = 0;
    down = 0;
    select = 0;
    menu = 0;
    right = 0;
    left = 0;

    if ((c & ~KF_REPEATED) == KEY_UP)
        up = 1;
    if ((c & ~KF_REPEATED) == KEY_DOWN)
        down = 1;
    if (c == KEY_ENTER || c == KEY_SPACE)
        select = 1;
    if (c == KEY_MENU) {
        menu = 1;
//        return 1;
    }
    if (c == KEY_RIGHT)
        right = 1;
    if (c == KEY_LEFT)
        left = 1;
    if (c == KEY_ESC && menustate != MENU_NONE2)
        menu = 1;
    switch (menustate)
    {
        /******************************************************************/
        /* no menu selected                                               */
        /******************************************************************/
    case MENU_NONE1 :

        OSD_Disable();
        menustate = MENU_NONE2;
        break;

    case MENU_NONE2 :

        if (menu)
        {
            menustate = MENU_MAIN1;
            menusub = 0;
            OSD_Clear();
            OSD_Enable(DISABLE_KEYBOARD);
        }
        break;

        /******************************************************************/
        /* main menu                                                      */
        /******************************************************************/
    case MENU_MAIN1 :
        /*OSD_Write(0, "      *** MINIMIG MENU ***      ", 0);*/
        /*OSD_Write(0, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx", 0);*/
        OSD_Write  (0, "      ***              ***", 0);
        OSD_WriteRC(0, 10, "REPLAY  MENU", 0, 0xC, 0);
        OSD_Write(1, "", 0);

        // floppy drive info
        for (i = 0; i < 4; i++)
        {
            if (i <= drives)
            {
                strcpy(s, " dfx: ");
                s[3] = i + '0';
                if (df[i].status & DSK_INSERTED) // floppy disk is inserted
                {
                    strncpy(&s[6], df[i].name, sizeof(df[0].name));
                    _strlcpy(&s[6 + sizeof(df[i].name)], df[i].status & DSK_WRITABLE ? " RW" : " RO", 32); // floppy disk is writable or read-only
                }
                else // no floppy disk
                    strcat(s, " * no disk *");

                OSD_Write(2 + i, s, menusub == i);
            }
            else
                OSD_Write(2 + i, "", 0);
        }

        OSD_Write(6, "", 0);
        OSD_Write(7, "              exit", menusub == 4);

        OSD_WriteRC(9,2,"A",0,0,0);
        OSD_WriteRC(9,3,"B",0,1,0);
        OSD_WriteRC(9,4,"C",0,2,0);
        OSD_WriteRC(9,5,"D",0,3,0);
        OSD_WriteRC(9,6,"E",0,4,0);
        OSD_WriteRC(9,7,"F",0,5,0);
        OSD_WriteRC(9,8,"G",0,6,0);
        OSD_WriteRC(9,9,"H",0,7,0);
        OSD_WriteRC(9,10,"I",0,8,0);
        OSD_WriteRC(9,11,"J",0,9,0);
        OSD_WriteRC(9,12,"K",0,10,0);
        OSD_WriteRC(9,13,"L",0,11,0);
        OSD_WriteRC(9,14,"M",0,12,0);
        OSD_WriteRC(9,15,"N",0,13,0);
        OSD_WriteRC(9,16,"O",0,14,0);
        OSD_WriteRC(9,17,"P",0,15,0);
        OSD_WriteRC(10,2,"A",0,15,0);
        OSD_WriteRC(10,3,"B",0,15,1);
        OSD_WriteRC(10,4,"C",0,15,2);
        OSD_WriteRC(10,5,"D",0,15,3);
        OSD_WriteRC(10,6,"E",0,15,4);
        OSD_WriteRC(10,7,"F",0,15,5);
        OSD_WriteRC(10,8,"G",0,15,6);
        OSD_WriteRC(10,9,"H",0,15,7);
        OSD_WriteRC(10,10,"I",0,7,8);
        OSD_WriteRC(10,11,"J",0,7,9);
        OSD_WriteRC(10,12,"K",0,7,10);
        OSD_WriteRC(10,13,"L",0,7,11);
        OSD_WriteRC(10,14,"M",0,7,12);
        OSD_WriteRC(10,15,"N",0,7,13);
        OSD_WriteRC(10,16,"O",0,7,14);
        OSD_WriteRC(10,17,"P",0,7,15);

        menustate = MENU_MAIN2;
        break;

    case MENU_MAIN2 :

        if (menu)
            menustate = MENU_NONE1;
        else if (up)
        {
            if (menusub > 0)
                menusub--;
            if (menusub > drives)
                menusub = drives;
            menustate = MENU_MAIN1;
        }
        else if (down)
        {
            if (menusub < 4)
                menusub++;
            if (menusub > drives)
                menusub = 4;
            menustate = MENU_MAIN1;
        }
        else if (select)
        {
            if (menusub < 4)
            {
                if (df[menusub].status & DSK_INSERTED) // eject selected floppy
                {
                    df[menusub].status = 0;
                    menustate = MENU_MAIN1;
                    FF_Close(df[menusub].fSource);
                }
                else
                {
                    df[menusub].status = 0;
                    SelectFile("ADF", MENU_FILE_SELECTED, MENU_MAIN1);
                }
            }
            else if (menusub == 4)
                menustate = MENU_NONE1;
        }
        else if (c == KEY_BACK) // eject all floppies
        {
            for (i = 0; i <= drives; i++)
                df[i].status = 0;

            menustate = MENU_MAIN1;
        }
        else if (c == KEY_F10)
        {
            SelectFile("BIN", MENU_FPGAFILE_SELECTED, MENU_MAIN1);
        }
        else if (right)
        {
            menustate = MENU_MAIN2_1;
            menusub = 0;
        }
        break;

    case MENU_FILE_SELECTED : // file successfully selected

         InsertFloppy(file_path, &df[menusub]);
         menustate = MENU_MAIN1;
         menusub++;
         if (menusub > drives)
             menusub = 4;

         break;

        /******************************************************************/
        /* second part of the main menu                                   */
        /******************************************************************/
    case MENU_MAIN2_1 :

        /*OSD_Write(0, " \x10\x11   *** MINIMIG MENU ***", 0);*/
        OSD_Write  (0, "      ***              ***", 0);
        OSD_WriteRC(0, 10, "REPLAY  MENU", 0, 0xC, 0);

        OSD_Write(1, "", 0);
        OSD_Write(2, "            reset",    menusub == 0);
        OSD_Write(3, "            settings", menusub == 1);
        OSD_Write(4, "            firmware", menusub == 2);
        OSD_Write(5, "", 0);
        OSD_Write(6, "", 0);
        OSD_Write(7, "              exit", menusub == 3);

        menustate = MENU_MAIN2_2;
        break;

    case MENU_MAIN2_2 :

        if (menu)
            menustate = MENU_NONE1;
        else if (up)
        {
            if (menusub > 0)
                menusub--;
            menustate = MENU_MAIN2_1;
        }
        else if (down)
        {
            if (menusub < 3)
                menusub++;
            menustate = MENU_MAIN2_1;
        }
        else if (select)
        {
            if (menusub == 0)
            {
                menusub = 1;
                menustate = MENU_RESET1;
            }
            else if (menusub == 1)
            {
                menusub = 0;
                menustate = MENU_SETTINGS1;
            }
            else if (menusub == 2)
            {
                menusub = 2;
                menustate = MENU_FIRMWARE1;
            }
            else if (menusub == 3)
                menustate = MENU_NONE1;
        }
        else if (left)
        {
            menustate = MENU_MAIN1;
            menusub = 0;
        }
        break;

        /******************************************************************/
        /* file selection menu                                            */
        /******************************************************************/
    case MENU_FILE_SELECT1 :

        PrintDirectory();
        menustate = MENU_FILE_SELECT2;
        break;

    case MENU_FILE_SELECT2 :

        ScrollLongName(); // scrolls file name if longer than display line

        if (c == KEY_HOME) {
          Filesel_ScanFirst(&dir); // scan first
          menustate = MENU_FILE_SELECT1;
        }

        if (c == KEY_BACK) {
          BackDir(dir_path);
          Filesel_ChangeDir(&dir, dir_path); // sets path
          Filesel_ScanFirst(&dir); // scan first
          menustate = MENU_FILE_SELECT1;
        }

        if ((c & ~KF_REPEATED) == KEY_PGUP)
        {
          Filesel_Update(&dir, SCAN_PREV_PAGE);
          menustate = MENU_FILE_SELECT1;
        }

        if ((c & ~KF_REPEATED) == KEY_PGDN)
        {
          Filesel_Update(&dir, SCAN_NEXT_PAGE);
          menustate = MENU_FILE_SELECT1;
        }

        if (down) // scroll down one entry
        {
          Filesel_Update(&dir, SCAN_NEXT);
          menustate = MENU_FILE_SELECT1;
        }

        if (up) // scroll up one entry
        {
          Filesel_Update(&dir, SCAN_PREV);
          menustate = MENU_FILE_SELECT1;
        }

        if (!(c&KF_RELEASED) && (i = OSD_ConvASCII(c)))
        { // find an entry beginning with given character
            //printf("key pressed %d \r\n",i);
            Filesel_ScanFind(&dir, i);
            menustate = MENU_FILE_SELECT1;
        }

        if (select) {
            FF_DIRENT mydir = Filesel_GetEntry(&dir, dir.sel);

            if (mydir.Attrib & FF_FAT_ATTR_DIR) {
              //printf("dir selected %s \r\n",mydir.FileName);

              if (_strnicmp(mydir.FileName, ".", FF_MAX_FILENAME) != 0) {
                //
                if (_strnicmp(mydir.FileName, "..", FF_MAX_FILENAME) == 0)
                  BackDir(dir_path);
                else
                  sprintf(dir_path, "%s%s\\", dir_path, mydir.FileName);
                  // should use safe copy

                Filesel_ChangeDir(&dir, dir_path); // sets path
                Filesel_ScanFirst(&dir); // scan first
              }

              menustate = MENU_FILE_SELECT1;
            } else {
              //file_sel
              sprintf(file_path, "%s%s", dir_path, mydir.FileName);
              // should use safe copy
              menustate = fs_MenuSelect;
           }
        }

        if (menu)
        {
            menustate = fs_MenuCancel;
        }

        break;

        /******************************************************************/
        /* reset menu                                                     */
        /******************************************************************/
    case MENU_RESET1 :

        OSD_Write(0, "         Reset Minimig?", 0);
        OSD_Write(1, "", 0);
        OSD_Write(2, "               yes", menusub == 0);
        OSD_Write(3, "               no", menusub == 1);
        OSD_Write(4, "", 0);
        OSD_Write(5, "", 0);
        OSD_Write(6, "", 0);
        OSD_Write(7, "", 0);

        menustate = MENU_RESET2;
        break;

    case MENU_RESET2 :

        if (down && menusub < 1)
        {
            menusub++;
            menustate = MENU_RESET1;
        }

        if (up && menusub > 0)
        {
            menusub--;
            menustate = MENU_RESET1;
        }

        if (select && menusub == 0)
        {
            menustate = MENU_NONE1;
            OSD_Reset(0);
        }

        if (menu || (select && menusub == 1)) // exit menu
        {
            menustate = MENU_MAIN2_1;
            menusub = 0;
        }
        break;

        /******************************************************************/
        /* settings menu                                                  */
        /******************************************************************/
    case MENU_SETTINGS1 :

        OSD_Write(0, "        *** SETTINGS ***", 0);
        OSD_Write(1, "", 0);
        OSD_Write(2, "             chipset", menusub == 0);
        OSD_Write(3, "             memory", menusub == 1);
        OSD_Write(4, "             drives", menusub == 2);
        OSD_Write(5, "             video", menusub == 3);
        OSD_Write(6, "", 0);

        if (menusub == 5)
            OSD_Write(7, "  \x14\x15          save          \x14\x15", 1);
        else if (menusub == 4)
            OSD_Write(7, "  \x16\x17          exit          \x16\x17", 1);
        else
            OSD_Write(7, "              exit", 0);

        menustate = MENU_SETTINGS2;
        break;

    case MENU_SETTINGS2 :

        if (down && menusub < 5)
        {
            menusub++;
            menustate = MENU_SETTINGS1;
        }

        if (up && menusub > 0)
        {
            menusub--;
            menustate = MENU_SETTINGS1;
        }

        if (select)
        {
            if (menusub == 0)
            {
                menustate = MENU_SETTINGS_CHIPSET1;
                menusub = 0;
            }
            else if (menusub == 1)
            {
                menustate = MENU_SETTINGS_MEMORY1;
                menusub = 0;
            }
            else if (menusub == 2)
            {
                menustate = MENU_SETTINGS_DRIVES1;
                menusub = 0;
            }
            else if (menusub == 3)
            {
                menustate = MENU_SETTINGS_VIDEO1;
                menusub = 0;
            }
            else if (menusub == 4)
            {
                menustate = MENU_MAIN2_1;
                menusub = 1;
            }
            else if (menusub == 5)
            {
                /*SaveConfiguration("MINIMIG2CFG");*/
                menustate = MENU_MAIN2_1;
                menusub = 1;
            }
        }

        if (menu)
        {
            menustate = MENU_MAIN2_1;
            menusub = 1;
        }
        break;

        /******************************************************************/
        /* chipset settings menu                                          */
        /******************************************************************/
    case MENU_SETTINGS_CHIPSET1 :

        OSD_Write(0, " \x10\x11         CHIPSET          \x12\x13", 0);
        OSD_Write(1, "", 0);
        strcpy(s, "       CPU Type : ");
        strcat(s, config_cpu_msg[amiga_config.cpu.type & CONFIG_CPU_TYPE]);
        OSD_Write(2, s, menusub == 0);
        strcpy(s, "     Video Mode : ");
        strcat(s, amiga_config.chipset.mode ? "NTSC" : "PAL");
        OSD_Write(3, s, menusub == 1);
        strcpy(s, "        Chipset : ");
        strcat(s, config_chipset_msg[amiga_config.chipset.type & CONFIG_CHIPSET_TYPE]);
        OSD_Write(4, s, menusub == 2);
        OSD_Write(5, "", 0);
        OSD_Write(6, "", 0);
        OSD_Write(7, "              exit", menusub == 3);

        menustate = MENU_SETTINGS_CHIPSET2;
        break;

    case MENU_SETTINGS_CHIPSET2 :

        if (down && menusub < 3)
        {
            menusub++;
            menustate = MENU_SETTINGS_CHIPSET1;
        }

        if (up && menusub > 0)
        {
            menusub--;
            menustate = MENU_SETTINGS_CHIPSET1;
        }

        if (select)
        {
            if (menusub == 0)
            {
                menustate = MENU_SETTINGS_CPU1;
            }
            else if (menusub == 1)
            {
                amiga_config.chipset.mode ^= 1;
                menustate = MENU_SETTINGS_CHIPSET1;
                ConfigUpdate();
            }
            else if (menusub == 2)
            {
                if (amiga_config.chipset.type == 3)
                    amiga_config.chipset.type = 0;
                else
                    amiga_config.chipset.type++;

                menustate = MENU_SETTINGS_CHIPSET1;
                ConfigUpdate();
            }
            else if (menusub == 3)
            {
                menustate = MENU_SETTINGS1;
                menusub = 0;
            }
        }

        if (menu)
        {
            menustate = MENU_SETTINGS1;
            menusub = 0;
        }
        else if (right)
        {
            menustate = MENU_SETTINGS_MEMORY1;
            menusub = 0;
        }
        else if (left)
        {
            menustate = MENU_SETTINGS_VIDEO1;
            menusub = 0;
        }
        break;

        /******************************************************************/
        /* cpu settings menu                                              */
        /******************************************************************/
    case MENU_SETTINGS_CPU1 :

        OSD_Write(0, "              CPU", 0);
        OSD_Write(1, "", 0);
        strcpy(s, "           TYPE : ");
        strcat(s, config_cpu_msg[amiga_config.cpu.type & CONFIG_CPU_TYPE]);
        OSD_Write(2, s, menusub == 0);
        strcpy(s, "          SPEED : ");
        strcat(s, amiga_config.cpu.speed ? "TURBO" : "NORMAL");
        OSD_Write(3, s, menusub == 1);
        strcpy(s, "          CACHE : ");
        strcat(s, amiga_config.cpu.cache ? "ON" : "OFF");
        OSD_Write(4, s, menusub == 2);
        strcpy(s, "       PREFETCH : ");
        strcat(s, amiga_config.cpu.prefetch ? "ON" : "OFF");
        OSD_Write(5, s, menusub == 3);
        OSD_Write(6, "", 0);
        OSD_Write(7, "              exit", menusub == 4);

        menustate = MENU_SETTINGS_CPU2;
        break;

    case MENU_SETTINGS_CPU2 :

        if (down && menusub < 4)
        {
            menusub++;
            menustate = MENU_SETTINGS_CPU1;
        }

        if (up && menusub > 0)
        {
            menusub--;
            menustate = MENU_SETTINGS_CPU1;
        }

        if (select)
        {
            if (menusub == 0)
            {
                if (amiga_config.cpu.type == 3)
                   amiga_config.cpu.type = 0;
                else
                   amiga_config.cpu.type++;

                menustate = MENU_SETTINGS_CPU1;
                ConfigUpdate();
            }
            else if (menusub == 1)
            {
                amiga_config.cpu.speed ^= 1;
                menustate = MENU_SETTINGS_CPU1;
                ConfigUpdate();
            }
            else if (menusub == 2)
            {
                amiga_config.cpu.cache ^= 1;
                menustate = MENU_SETTINGS_CPU1;
                ConfigUpdate();
            }
            else if (menusub == 3)
            {
                amiga_config.cpu.prefetch ^= 1;
                menustate = MENU_SETTINGS_CPU1;
                ConfigUpdate();
            }
            else if (menusub == 4)
            {
                menustate = MENU_SETTINGS_CHIPSET1;
                menusub = 0;
            }
        }

        if (menu)
        {
            menustate = MENU_SETTINGS_CHIPSET1;
            menusub = 0;
        }
        break;

        /******************************************************************/
        /* memory settings menu                                           */
        /******************************************************************/
    case MENU_SETTINGS_MEMORY1 :
        OSD_Write(0, " \x10\x11          MEMORY          \x12\x13", 0);
        OSD_Write(1, "", 0);
        i = 0;
        if (menusub < 4)
        {
            strcpy(s, "         ROM : ");
            if (amiga_config.kickstart.long_name[0])
                strncat(s, amiga_config.kickstart.long_name, sizeof(amiga_config.kickstart.long_name));
            else
                strncat(s, amiga_config.kickstart.name, sizeof(amiga_config.kickstart.name));
            OSD_Write(2, s, menusub == 0);
        }
        else if (menusub < 6)
            i = menusub - 3;
        else
            i = 2;
        if (menusub < 5)
        {
        strcpy(s, "        CHIP : ");
        strcat(s, config_memory_chip_msg[amiga_config.memory.chip & 0x03]);
        OSD_Write(3-i, s, menusub == 1);
        }
        strcpy(s, "        SLOW : ");
        strcat(s, config_memory_slow_msg[amiga_config.memory.slow & 0x03]);
        OSD_Write(4-i, s, menusub == 2);
        strcpy(s, "        FAST : ");
        strcat(s, config_memory_fast_msg[amiga_config.memory.fast & 0x03]);
        OSD_Write(5-i, s, menusub == 3);
        if (menusub > 3)
        {
        strcpy(s, "        XRAM : ");
        strcat(s, config_memory_xram_msg[amiga_config.memory.xram & 0x03]);
        OSD_Write(6-i, s, menusub == 4);
        }
        if (menusub > 4)
        {
        strcpy(s, "        CART : ");
        strcat(s, amiga_config.disable_cart ? "disabled" : "enabled ");
        OSD_Write(7-i, s, menusub == 5);
        }
        OSD_Write(6, "", 0);
        OSD_Write(7, "              exit", menusub == 6);

        menustate = MENU_SETTINGS_MEMORY2;
        break;

    case MENU_SETTINGS_MEMORY2 :

        if (down && menusub < 6)
        {
            menusub++;
            menustate = MENU_SETTINGS_MEMORY1;
        }

        if (up && menusub > 0)
        {
            menusub--;
            menustate = MENU_SETTINGS_MEMORY1;
        }

        if (select)
        {
            if (menusub == 0)
            {
                //SelectFile("ROM", MENU_ROMFILE_SELECTED, MENU_SETTINGS_MEMORY1);
            }
            else if (menusub == 1)
            {
                amiga_config.memory.chip++;
                amiga_config.memory.chip &= 0x03;
                menustate = MENU_SETTINGS_MEMORY1;
                ConfigUpdate();
            }
            else if (menusub == 2)
            {
                amiga_config.memory.slow++;
                amiga_config.memory.slow &= 0x03;
                menustate = MENU_SETTINGS_MEMORY1;
                ConfigUpdate();
            }
            else if (menusub == 3)
            {
                amiga_config.memory.fast++;
                amiga_config.memory.fast &= 0x03;
                menustate = MENU_SETTINGS_MEMORY1;
                ConfigUpdate();
            }
            else if (menusub == 4)
            {
                amiga_config.memory.xram++;
                amiga_config.memory.xram &= 0x03;
                menustate = MENU_SETTINGS_MEMORY1;
                ConfigUpdate();
            }
            else if (menusub == 5)
            {
                amiga_config.disable_cart ^= 0x01;
                amiga_config.disable_cart &= 0x01;
                menustate = MENU_SETTINGS_MEMORY1;
                // no update to FPGA
            }
            else if (menusub == 6)
            {
                menustate = MENU_SETTINGS1;
                menusub = 1;
            }
        }

        if (menu)
        {
            menustate = MENU_SETTINGS1;
            menusub = 1;
        }
        else if (right)
        {
            menustate = MENU_SETTINGS_DRIVES1;
            menusub = 0;
        }
        else if (left)
        {
            menustate = MENU_SETTINGS_CHIPSET1;
            menusub = 0;
        }
        break;

        /******************************************************************/
        /* drive settings menu                                            */
        /******************************************************************/
    case MENU_SETTINGS_DRIVES1 :

        OSD_Write(0, " \x10\x11          DRIVES          \x12\x13", 0);
        OSD_Write(1, "", 0);
        sprintf(s, "         drives : %d", amiga_config.floppy.drives + 1);
        OSD_Write(2, s, menusub == 0);
        strcpy(s, "          speed : ");
        strcat(s, amiga_config.floppy.speed ? "fast " : "normal");
        OSD_Write(3, s, menusub == 1);
        strcpy(s, " A600/A1200 IDE : ");
        strcat(s, amiga_config.hdc.enabled ? "on " : "off");
        OSD_Write(4, s, menusub == 2);
        sprintf(s, "      hardfiles : %d", (amiga_config.hardfile[0].present & amiga_config.hardfile[0].enabled) + (amiga_config.hardfile[1].present & amiga_config.hardfile[1].enabled));
        OSD_Write(5,s, menusub == 3);
        OSD_Write(6, "", 0);
        OSD_Write(7, "              exit", menusub == 4);

        menustate = MENU_SETTINGS_DRIVES2;
        break;

    case MENU_SETTINGS_DRIVES2 :

        if (down && menusub < 4)
        {
            menusub++;
            menustate = MENU_SETTINGS_DRIVES1;
        }

        if (up && menusub > 0)
        {
            menusub--;
            menustate = MENU_SETTINGS_DRIVES1;
        }

        if (select)
        {
            if (menusub == 0)
            {
                amiga_config.floppy.drives++;
                amiga_config.floppy.drives &= 3;
                menustate = MENU_SETTINGS_DRIVES1;
                ConfigUpdate();
            }
            else if (menusub == 1)
            {
                amiga_config.floppy.speed ^= 1;
                menustate = MENU_SETTINGS_DRIVES1;
                ConfigUpdate();
            }
            else if (menusub == 2)
            {
                amiga_config.hdc.enabled ^= 1;
                menustate = MENU_SETTINGS_DRIVES1;
                ConfigUpdate();
            }
            else if (menusub == 3)
            {
                //t_hardfile[0] = amiga_config.hardfile[0];
                //t_hardfile[1] = amiga_config.hardfile[1];
                menustate = MENU_SETTINGS_HARDFILE1;
                menusub = 4;
            }
            else if (menusub == 4)
            {
                menustate = MENU_SETTINGS1;
                menusub = 2;
            }
        }

        if (menu)
        {
            menustate = MENU_SETTINGS1;
            menusub = 2;
        }
        else if (right)
        {
            menustate = MENU_SETTINGS_VIDEO1;
            menusub = 0;
        }
        else if (left)
        {
            menustate = MENU_SETTINGS_MEMORY1;
            menusub = 0;
        }
        break;

        /******************************************************************/
        /* hardfile settings menu                                         */
        /******************************************************************/

    case MENU_SETTINGS_HARDFILE1 :

        OSD_Write(0, "            HARDFILES", 0);
        OSD_Write(1, "", 0);
        strcpy(s, "     Master : ");
        strcat(s, amiga_config.hardfile[0].present ? amiga_config.hardfile[0].enabled ? "enabled" : "disabled" : "n/a");
        OSD_Write(2, s, menusub == 0);
        if (amiga_config.hardfile[0].present)
        {
            strcpy(s, "                                ");
            if (amiga_config.hardfile[0].long_name[0])
                strncpy(&s[14], amiga_config.hardfile[0].long_name, sizeof(amiga_config.hardfile[0].long_name));
            else
                strncpy(&s[14], amiga_config.hardfile[0].name, sizeof(amiga_config.hardfile[0].name));
            OSD_Write(3, s, menusub == 1);
        }
        else
            OSD_Write(3, "       ** file not found **", menusub == 1);

        strcpy(s, "      Slave : ");
        strcat(s, amiga_config.hardfile[1].present ? amiga_config.hardfile[1].enabled ? "enabled" : "disabled" : "n/a");
        OSD_Write(4, s, menusub == 2);
        if (amiga_config.hardfile[1].present)
        {
            strcpy(s, "                                ");
            if (amiga_config.hardfile[1].long_name[0])
                strncpy(&s[14], amiga_config.hardfile[1].long_name, sizeof(amiga_config.hardfile[0].long_name));
            else
                strncpy(&s[14], amiga_config.hardfile[1].name, sizeof(amiga_config.hardfile[0].name));
            OSD_Write(5, s, menusub == 3);
        }
        else
            OSD_Write(5, "       ** file not found **", menusub == 3);

        OSD_Write(6, "", 0);
        OSD_Write(7, "              exit", menusub == 4);

        menustate = MENU_SETTINGS_HARDFILE2;
        break;

    case MENU_SETTINGS_HARDFILE2 :

        if (down && menusub < 4)
        {
            menusub++;
            menustate = MENU_SETTINGS_HARDFILE1;
        }

        if (up && menusub > 0)
        {
            menusub--;
            menustate = MENU_SETTINGS_HARDFILE1;
        }

        if (select)
        {
            if (menusub == 0)
            {
                if (amiga_config.hardfile[0].present)
                {
                   amiga_config.hardfile[0].enabled ^= 1;
                   menustate = MENU_SETTINGS_HARDFILE1;
                }
            }
            else if (menusub == 1)
            {
                SelectFile("HDF", MENU_HARDFILE_SELECTED, MENU_SETTINGS_HARDFILE1);
            }
            else if (menusub == 2)
            {
                if (amiga_config.hardfile[1].present)
                {
                   amiga_config.hardfile[1].enabled ^= 1;
                   menustate = MENU_SETTINGS_HARDFILE1;
                }
            }
            else if (menusub == 3)
            {
                SelectFile("HDF", MENU_HARDFILE_SELECTED, MENU_SETTINGS_HARDFILE1);
            }
            else if (menusub == 4) // return to previous menu
            {
                menustate = MENU_HARDFILE_EXIT;
            }
        }

        if (menu) // return to previous menu
        {
            menustate = MENU_HARDFILE_EXIT;
        }
        break;

        /******************************************************************/
        /* hardfile selected menu                                         */
        /******************************************************************/
    case MENU_HARDFILE_SELECTED :

        if (menusub == 1) // master drive selected
        {
            //memcpy((void*)amiga_config.hardfile[0].name, (void*)file.name, sizeof(amiga_config.hardfile[0].name));
            //memcpy((void*)amiga_config.hardfile[0].long_name, (void*)file.long_name, sizeof(amiga_config.hardfile[0].long_name));
            amiga_config.hardfile[0].present = 1;
        }

        if (menusub == 3) // slave drive selected
        {
            //memcpy((void*)amiga_config.hardfile[1].name, (void*)file.name, sizeof(amiga_config.hardfile[1].name));
            //memcpy((void*)amiga_config.hardfile[1].long_name, (void*)file.long_name, sizeof(amiga_config.hardfile[1].long_name));
            amiga_config.hardfile[1].present = 1;
        }

        menustate = MENU_SETTINGS_HARDFILE1;
        break;

     // check if hardfile configuration has changed
    case MENU_HARDFILE_EXIT :

         if (memcmp(amiga_config.hardfile, t_hardfile, sizeof(t_hardfile)) != 0)
         {
             menustate = MENU_HARDFILE_CHANGED1;
             menusub = 1;
         }
         else
         {
             menustate = MENU_SETTINGS_DRIVES1;
             menusub = 3;
         }

         break;

    // hardfile configuration has changed, ask user if he wants to use the new settings
    case MENU_HARDFILE_CHANGED1 :

        OSD_Write(0, "", 0);
        OSD_Write(1, "      Changing configuration", 0);
        OSD_Write(2, "        requires reset.", 0);
        OSD_Write(3, "", 0);
        OSD_Write(4, "         Reset Minimig?", 0);
        OSD_Write(5, "", 0);
        OSD_Write(6, "               yes", menusub == 0);
        OSD_Write(7, "               no", menusub == 1);

        menustate = MENU_HARDFILE_CHANGED2;
        break;

    case MENU_HARDFILE_CHANGED2 :

        if (down && menusub < 1)
        {
            menusub++;
            menustate = MENU_HARDFILE_CHANGED1;
        }

        if (up && menusub > 0)
        {
            menusub--;
            menustate = MENU_HARDFILE_CHANGED1;
        }

        if (select)
        {
            if (menusub == 0) // yes
            {
                /*
                if (strncmp(amiga_config.hardfile[0].name, t_hardfile[0].name, sizeof(t_hardfile[0].name)) != 0)
                    OpenHardfile(0);

                if (strncmp(amiga_config.hardfile[1].name, t_hardfile[1].name, sizeof(t_hardfile[1].name)) != 0)
                    OpenHardfile(1);
                */

                /*ConfigMasterHDD(amiga_config.hardfile[0].present & amiga_config.hardfile[0].enabled);*/
                /*ConfigSlaveHDD(amiga_config.hardfile[1].present & amiga_config.hardfile[1].enabled);*/
                ConfigUpdate();
                OSD_Reset(0);

                menustate = MENU_NONE1;
            }
            else if (menusub == 1) // no
            {
                memcpy(amiga_config.hardfile, t_hardfile, sizeof(t_hardfile)); // restore configuration
                menustate = MENU_SETTINGS_DRIVES1;
                menusub = 3;
            }
        }

        if (menu)
        {
            memcpy(amiga_config.hardfile, t_hardfile, sizeof(t_hardfile)); // restore configuration
            menustate = MENU_SETTINGS_DRIVES1;
            menusub = 3;
        }
        break;

        /******************************************************************/
        /* video settings menu                                            */
        /******************************************************************/
    case MENU_SETTINGS_VIDEO1 :

        OSD_Write(0, " \x10\x11          VIDEO           \x12\x13", 0);
        OSD_Write(1, "", 0);
        strcpy(s, "   Lores Filter : ");
        strcat(s, config_filter_msg[amiga_config.filter.lores]);
        OSD_Write(2, s, menusub == 0);
        strcpy(s, "   Hires Filter : ");
        strcat(s, config_filter_msg[amiga_config.filter.hires]);
        OSD_Write(3, s, menusub == 1);
        strcpy(s, "   Scanlines    : ");
        strcat(s, config_scanlines_msg[amiga_config.scanlines]);
        OSD_Write(4, s, menusub == 2);
        OSD_Write(5, "", 0);
        OSD_Write(6, "", 0);
        OSD_Write(7, "              exit", menusub == 3);

        menustate = MENU_SETTINGS_VIDEO2;
        break;

    case MENU_SETTINGS_VIDEO2 :

        if (down && menusub < 3)
        {
            menusub++;
            menustate = MENU_SETTINGS_VIDEO1;
        }

        if (up && menusub > 0)
        {
            menusub--;
            menustate = MENU_SETTINGS_VIDEO1;
        }

        if (select)
        {
            if (menusub == 0)
            {
                amiga_config.filter.lores++;
                amiga_config.filter.lores &= 3;
                menustate = MENU_SETTINGS_VIDEO1;
                /*ConfigFilterLores(amiga_config.filter.lores);*/
                ConfigUpdate();
            }
            else if (menusub == 1)
            {
                amiga_config.filter.hires++;
                amiga_config.filter.hires &= 3;
                menustate = MENU_SETTINGS_VIDEO1;
                /*ConfigFilterHires(amiga_config.filter.hires);*/
                ConfigUpdate();
            }
            else if (menusub == 2)
            {
                amiga_config.scanlines++;
                if (amiga_config.scanlines > 2)
                    amiga_config.scanlines = 0;
                menustate = MENU_SETTINGS_VIDEO1;
                /*ConfigScanlines(amiga_config.scanlines);*/
                ConfigUpdate();
            }
            else if (menusub == 3)
            {
                menustate = MENU_SETTINGS1;
                menusub = 3;
            }
        }

        if (menu)
        {
            menustate = MENU_SETTINGS1;
            menusub = 3;
        }
        else if (right)
        {
            menustate = MENU_SETTINGS_CHIPSET1;
            menusub = 0;
        }
        else if (left)
        {
            menustate = MENU_SETTINGS_DRIVES1;
            menusub = 0;
        }
        break;

        /******************************************************************/
        /* rom file selected menu                                         */
        /******************************************************************/
    case MENU_ROMFILE_SELECTED :

         menusub = 1;
         // no break intended

    case MENU_ROMFILE_SELECTED1 :

        OSD_Write(0, "", 0);
        OSD_Write(1, "        Reload Kickstart?", 0);
        OSD_Write(2, "", 0);
        OSD_Write(3, "               yes", menusub == 0);
        OSD_Write(4, "               no", menusub == 1);
        OSD_Write(5, "", 0);
        OSD_Write(6, "", 0);
        OSD_Write(7, "", 0);

        menustate = MENU_ROMFILE_SELECTED2;
        break;

    case MENU_ROMFILE_SELECTED2 :

        if (down && menusub < 1)
        {
            menusub++;
            menustate = MENU_ROMFILE_SELECTED1;
        }

        if (up && menusub > 0)
        {
            menusub--;
            menustate = MENU_ROMFILE_SELECTED1;
        }

        if (select)
        {
            if (menusub == 0)
            {
                //memcpy((void*)amiga_config.kickstart.name, (void*)file.name, sizeof(amiga_config.kickstart.name));
                //memcpy((void*)amiga_config.kickstart.long_name, (void*)file.long_name, sizeof(amiga_config.kickstart.long_name));

                OSD_Disable();
                OSD_Reset(0);
                /*ConfigCPUSpeed(CONFIG_CPU_TURBO);*/
                /*ConfigFloppySpeed(CONFIG_FLOPPY_FAST);*/
                ConfigUpdate();

                /*if (Amiga_UploadKickstart(amiga_config.kickstart.name))*/
                /*{*/
                    /*FPGA_BootExit();*/
                /*}*/
                /*ConfigCPUSpeed(amiga_config.cpu.speed); // restore CPU speed mode*/
                /*ConfigFloppySpeed(amiga_config.floppy.speed); // restore floppy speed mode*/
                ConfigUpdate();

                menustate = MENU_NONE1;
            }
            else if (menusub == 1)
            {
                menustate = MENU_SETTINGS_MEMORY1;
                menusub = 0;
            }
        }

        if (menu)
        {
            menustate = MENU_SETTINGS_MEMORY1;
            menusub = 0;
        }
        break;

        /******************************************************************/
        /* foga file selected menu                                         */
        /******************************************************************/
    case MENU_FPGAFILE_SELECTED :

        //InitFpga(file.name);
        menustate = MENU_NONE1;
        break;

        /******************************************************************/
        /* firmware menu */
        /******************************************************************/
    case MENU_FIRMWARE1 :

        OSD_Write(0, "        *** Firmware ***", 0);
        OSD_Write(1, "", 0);
        sprintf(s, "   ARM s/w ver.: %s", version + 5);
        OSD_Write(2, s, 0);
        OSD_Write(3, "", 0);
        OSD_Write(4, "", 0);
        OSD_Write(5, "", 0);
        OSD_Write(6, "", 0);
        OSD_Write(7, "              exit", menusub == 0);

        menusub = 7;
        menustate = MENU_FIRMWARE2;
        break;

    case MENU_FIRMWARE2 :

        if (menu)
        {
            menusub = 1;
            menustate = MENU_MAIN2_1;
        }
        else if (select)
        {
                menustate = MENU_MAIN2_1;
                menusub = 2;
        }
        break;


        /******************************************************************/
        /* error message menu                                             */
        /******************************************************************/
    case MENU_ERROR :

        if (menu)
        {
            menustate = MENU_NONE1;
        }
        break;

        /******************************************************************/
        /* popup info menu                                                */
        /******************************************************************/
    case MENU_INFO :

        if (menu)
            menustate = MENU_MAIN1;
        else if (Timer_Check(menu_timer))
            menustate = MENU_NONE1;

        break;

        /******************************************************************/
        /* we should never come here                                      */
        /******************************************************************/
    default :

        break;
    }
  return 0;
}

void ScrollLongName(void)
{
// this function is called periodically when file selection window is displayed
// it checks if predefined period of time has elapsed and scrolls the name if necessary
    #define BLANKSPACE 10 // number of spaces between the end and start of repeated name

    char *filename;
    long len;
    long max_len;
    long offset;
    uint8_t sel = Filesel_GetSel(&dir);
    FF_DIRENT entry = Filesel_GetEntry(&dir, dir.sel);
    if (Timer_Check(scroll_timer)) { // scroll if long name and timer delay elapsed*/
        scroll_timer = Timer_Get(20); // reset scroll timer to repeat delay
        scroll_offset++; // increase scroll position

        /*
        memset(s, ' ', 32); // clear line buffer
        s[32] = 0; // set temporary string length to OSD line length

        filename = entry.FileName;
        len      = strlen(filename);
        // leave extension for now
        if (len > 30) { // scroll name if longer than display size


          if (scroll_offset >= (len + BLANKSPACE) << 4) // reset scroll position if it exceeds predefined maximum
            scroll_offset = 0;

          if ((scroll_offset & 0xF) == 0) {
            offset = scroll_offset >> 4; // get new starting character of the name
            len -= offset; // remaing number of characters in the name
            // NOTE, len goes -ve

            max_len = 31; // number of file name characters to display (one more required for srolling)
            if (entry.Attrib & FF_FAT_ATTR_DIR)
              max_len = 25; // number of directory name characters to display

            if (len > max_len)
              len = max_len; // trim length to the maximum value

            if (len > 0)
              strncpy(s, &filename[offset], len); // copy name substring
            if (len < max_len - BLANKSPACE) // file name substring and blank space is shorter than display line size
              strncpy(s + len + BLANKSPACE, &filename[0], max_len - len - BLANKSPACE); // repeat the name after its end and predefined number of blank space

            printf("scroll <%s>\r\n",s);
            OSD_Write(sel, s, 1);
          }
          */
          OSD_WaitVBL();
          /*OSD_WriteRC(sel, 31, "A", 1, 0x7, 0x0); //*/
          /*OSD_WriteRC(sel, 32, "X", 1, 0x7, 0x0); //*/
          OSD_SetHOffset(sel, (uint8_t) (scroll_offset >> 4), (uint8_t) (scroll_offset & 0xF));

          //OSD_PrintText((unsigned char)sel, s, 8, (max_len - 1) << 3, (scroll_offset & 0x3) << 1, 1); // OSD print function with pixel precision
        //}

    }

}

char* GetDiskInfo(char* filename, uint32_t len)
{
// extracts disk number substring form file name
// if file name contains "X of Y" substring where X and Y are one or two digit number
// then the number substrings are extracted and put into the temporary buffer for further processing
// comparision is case sensitive

    uint16_t i, k;
    static char info[] = "XX/XX"; // temporary buffer
    static char template[4] = " of "; // template substring to search for
    char *ptr1, *ptr2, c;
    unsigned char cmp;

    if (len > 20) // scan only names which can't be fully displayed
    {
        for (i = (uint16_t) len - 1 - sizeof(template); i > 0; i--) // scan through the file name starting from its end
        {
            ptr1 = &filename[i]; // current start position
            ptr2 = template;
            cmp = 0;
            for (k = 0; k < sizeof(template); k++) // scan through template
            {
                cmp |= *ptr1++ ^ *ptr2++; // compare substrings' characters one by one
                if (cmp)
                   break; // stop further comparing if difference already found
            }

            if (!cmp) // match found
            {
                k = i - 1; // no need to check if k is valid since i is greater than zero
                c = filename[k]; // get the first character to the left of the matched template substring
                if (c >= '0' && c <= '9') // check if a digit
                {
                    info[1] = c; // copy to buffer
                    info[0] = ' '; // clear previous character
                    k--; // go to the preceding character
                    if (k >= 0) // check if index is valid
                    {
                        c = filename[k];
                        if (c >= '0' && c <= '9') // check if a digit
                            info[0] = c; // copy to buffer
                    }

                    k = i + sizeof(template); // get first character to the right of the mached template substring
                    c = filename[k]; // no need to check if index is valid
                    if (c >= '0' && c <= '9') // check if a digit
                    {
                        info[3] = c; // copy to buffer
                        info[4] = ' '; // clear next char
                        k++; // go to the followwing character
                        if (k < len) // check if index is valid
                        {
                            c = filename[k];
                            if (c >= '0' && c <= '9') // check if a digit
                                info[4] = c; // copy to buffer
                        }
                        return info;
                    }
                }
            }
        }
    }
    return NULL;
}

// print directory contents
void PrintDirectory(void)
{
  uint32_t len;
  uint32_t i;
  char *filename;
  char *info;
  FF_DIRENT entry;


  s[32] = 0; // set temporary string length to OSD line length

  scroll_timer = Timer_Get(SCROLL_DELAY); // set timer to start name scrolling after predefined time delay
  scroll_offset = 0; // start scrolling from the start
  uint8_t sel = Filesel_GetSel(&dir);

  for (i = 0; i < 16; i++) {
    memset(s, ' ', 32); // clear line buffer
    entry = Filesel_GetLine(&dir, i);
    filename = entry.FileName;
    len = strlen(filename);

    info = GetDiskInfo(filename, len); // extract disk number info
    if (len > 30)
      len = 30; // trim display length if longer than 30 characters

    if (entry.Attrib & FF_FAT_ATTR_DIR) {
      // a dir
      strncpy(s + 1, filename, len); // display only name
      strcpy(&s[25], " <DIR>");

    } else {
      if (len > 4)
        if (filename[len-4] == '.')
          len -= 4; // remove extension

      if (i != sel && info != NULL) {
        // display disk number info for not selected items
        strncpy(s + 1, filename, 30-6); // trimmed name
        strncpy(s + 1+30-5, info, 5); // disk number
       } else
        strncpy(s + 1, filename, len); // display only name
    }

    if (i == 1 && dir.total_entries == 0) {
      strcpy(s, "            No files!");
    }

    OSD_SetHOffset(i, 0, 0);
    OSD_Write(i, s, i == sel);
  }
  #ifdef OSD_DEBUG
    printf("\r\n");
  #endif
}

void InsertFloppy(char *path, adfTYPE *drive)
{
  uint16_t i;
  uint32_t tracks;

  drive->fSource = FF_Open(pIoman, path, FF_MODE_READ, NULL);

  if (!drive->fSource) {
    printf("InsertFloppy:Could not open file.\n\r");
    return;
  }

  // calculate number of tracks in the ADF image file
  tracks = drive->fSource->Filesize / (512*11);
  if (tracks > MAX_TRACKS) {
    printf("UNSUPPORTED ADF SIZE!!! Too many tracks: %lu\n", tracks);
    tracks = MAX_TRACKS;
  }
  drive->tracks = (uint16_t)tracks;

  // get display name
  i = (uint16_t) strlen(path);

  while(i != 0) {
    if(path[i] == '\\' || path[i] == '/') {
      break;
    }
    i--;
  }
  _strncpySpace(drive->name, (path + i + 1), MAX_DISPLAY_FILENAME);
  drive->name[MAX_DISPLAY_FILENAME-1] = '\0';

  // initialize the rest of drive struct
  drive->status = DSK_INSERTED;
  //if (!(file.attributes & ATTR_READONLY)) // read-only attribute
  //    drive->status |= DSK_WRITABLE;

  drive->sector_offset = 0;
  drive->track = 0;
  drive->track_prev = -1;

  // some debug info
  printf("Inserting floppy: <%s>\n\r", drive->name);

  printf("file size   : %lu (%lu kB)\n\r", drive->fSource->Filesize, drive->fSource->Filesize >> 10);
  printf("drive tracks: %u\n\r", drive->tracks);
  printf("drive status: 0x%02X\n\r", drive->status);

}

/*  Error Message */
void ErrorMessage(const char *message, unsigned char code)
{
    menustate = MENU_ERROR;

    OSD_Write(0, "         *** ERROR ***", 1);
    OSD_Write(1, "", 0);
    strncpy(s, message, 32);
    s[32] = 0;
    OSD_Write(2, s, 0);
    OSD_Write(3, "", 0);

    if (code)
    {
        sprintf(s, "    #%d", code);
        OSD_Write(4, s, 0);
    }
    else
        OSD_Write(4, "", 0);

    OSD_Write(5, "", 0);
    OSD_Write(6, "", 0);
    OSD_Write(7, "", 0);

    OSD_Enable(0); // do not disable KEYBOARD
}

void InfoMessage(char *message)
{
    OSD_WaitVBL();
    if (menustate != MENU_INFO)
    {
        OSD_Clear();
        OSD_Enable(0); // do not disable keyboard
    }
    OSD_Write(1, message, 0);
    menu_timer = Timer_Get(1000);
    menustate = MENU_INFO;
}


