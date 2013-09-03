#ifndef AMIGA_MENU_H_INCLUDED
#define AMIGA_MENU_H_INCLUDED

#include "board.h"
/*menu states*/
enum MENU
{
    MENU_NONE1,
    MENU_NONE2,
    MENU_MAIN1,
    MENU_MAIN2,
    MENU_FILE_SELECT1,
    MENU_FILE_SELECT2,
    MENU_FILE_SELECTED,
    MENU_RESET1,
    MENU_RESET2,
    MENU_SETTINGS1,
    MENU_SETTINGS2,
    MENU_ROMFILE_SELECTED,
    MENU_ROMFILE_SELECTED1,
    MENU_ROMFILE_SELECTED2,
    MENU_SETTINGS_VIDEO1,
    MENU_SETTINGS_VIDEO2,
    MENU_SETTINGS_MEMORY1,
    MENU_SETTINGS_MEMORY2,
    MENU_SETTINGS_CHIPSET1,
    MENU_SETTINGS_CHIPSET2,
    MENU_SETTINGS_DRIVES1,
    MENU_SETTINGS_DRIVES2,
    MENU_SETTINGS_HARDFILE1,
    MENU_SETTINGS_HARDFILE2,
    MENU_HARDFILE_SELECT1,
    MENU_HARDFILE_SELECT2,
    MENU_HARDFILE_SELECTED,
    MENU_HARDFILE_EXIT,
    MENU_HARDFILE_CHANGED1,
    MENU_HARDFILE_CHANGED2,
    MENU_MAIN2_1,
    MENU_MAIN2_2,
    MENU_FIRMWARE1,
    MENU_FIRMWARE2,
    MENU_FIRMWARE_OPTIONS1,
    MENU_FIRMWARE_OPTIONS2,
    MENU_FIRMWARE_OPTIONS_ENABLE1,
    MENU_FIRMWARE_OPTIONS_ENABLE2,
    MENU_FIRMWARE_OPTIONS_ENABLED1,
    MENU_FIRMWARE_OPTIONS_ENABLED2,
    MENU_ERROR,
    MENU_INFO,
    MENU_FPGAFILE_SELECTED,
    MENU_SETTINGS_CPU1,
    MENU_SETTINGS_CPU2,
};

//{{{
typedef struct
{
    char name[8];
    char long_name[16];
} kickstartTYPE;

typedef struct
{
    unsigned char lores;
    unsigned char hires;
} filterTYPE;

typedef struct
{
    unsigned char chip;
    unsigned char slow;
    unsigned char fast;
    unsigned char xram;
} memoryTYPE;

typedef struct
{
    unsigned char speed;
    unsigned char drives;
} floppyTYPE;

typedef struct
{
    unsigned char enabled;
    unsigned char present;
    char name[8];
    char long_name[16];
} hardfileTYPE;

typedef struct
{
    unsigned char type;
    unsigned char speed;
    unsigned char cache;
    unsigned char prefetch;
} cpuTYPE;

typedef struct
{
    unsigned char type;
    unsigned char mode;
} chipsetTYPE;

typedef struct
{
    unsigned char enabled;
} hdcTYPE;

typedef struct
{
    char          id[8];
    unsigned long version;
    kickstartTYPE kickstart;
    filterTYPE    filter;
    memoryTYPE    memory;
    chipsetTYPE   chipset;
    cpuTYPE       cpu;
    floppyTYPE    floppy;
    unsigned char disable_cart;
    hdcTYPE       hdc;
    unsigned char scanlines;
    hardfileTYPE  hardfile[2];
    unsigned char scandouble;
} configTYPE;

#define CONFIG_CPU_TURBO 1
#define CONFIG_CPU_NORMAL 0
#define CONFIG_VIDEO_NTSC 1
#define CONFIG_VIDEO_PAL 0
#define CONFIG_CHIPSET_TYPE 3

#define CONFIG_CPU_TYPE 3
#define CONFIG_CPU_CACHE 1
#define CONFIG_CPU_PREFETCH 1
#define CONFIG_CPU_SPEED 1

#define CONFIG_FLOPPY_NORMAL 0
#define CONFIG_FLOPPY_FAST 1

//}}}
//uint8_t LoadConfiguration(char *filename);
//uint8_t SaveConfiguration(char *filename);

//uint8_t Cfg_UploadRom(char *name);
/** @brief AMIGA CONFIGURATION

    TODO: better documentation
*/
void ConfigUpdate(void);

void InitUI(void);
void InitUIFile(void);
void InsertFloppy(char *path, adfTYPE *drive);
uint8_t HandleUI(void);
void PrintDirectory(void);
void ScrollLongName(void);
void ErrorMessage(const char *message, unsigned char code);
void InfoMessage(char *message);

#endif
