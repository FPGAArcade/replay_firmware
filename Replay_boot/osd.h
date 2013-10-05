#ifndef OSD_H_INCLUDED
#define OSD_H_INCLUDED

#include "board.h"
//#define OSD_DEBUG

#define OSDMAXLEN         64          // for printing

// some constants
#define OSDNLINE          16          // number of lines of OSD
#define OSDLINELEN        32          // single line length in chars (64 actually)
//
#define OSDCMD_READSTAT   0x00
#define OSDCMD_READKBD    0x08

#define OSDCMD_CTRL       0x10
#define OSDCMD_CTRL_RES   0x11        // OSD reset command
#define OSDCMD_CTRL_HALT  0x12        // OSD reset command

#define OSDCMD_CONFIG     0x20
#define OSDCMD_DISABLE    0x40
#define OSDCMD_ENABLE     0x41
#define OSDCMD_SETHOFF    0x50
#define OSDCMD_SETVOFF    0x51
#define OSDCMD_WRITE      0xC0

#define REPEATDELAY     500         // repeat delay in 1ms units
#define REPEATRATE      50          // repeat rate in 1ms units

#define BUTTONDELAY     100         // replay button delay in 1ms units

#define SERIALDELAY     1           // serial timeout delay in 1ms units

#define STF_NEWKEY 0x01

/* AT Keyboard Control Codes */
#define ATKB_EXTEND  0xE0
#define ATKB_RELEASE 0xF0
/* Key Flags */
#define KF_EXTENDED 0x100
#define KF_RELEASED 0x200
#define KF_REPEATED 0x400

#define OSDCTRLUP       0x01        /*OSD up control*/
#define OSDCTRLDOWN     0x02        /*OSD down control*/
#define OSDCTRLSELECT   0x04        /*OSD select control*/
#define OSDCTRLMENU     0x08        /*OSD menu control*/
#define OSDCTRLRIGHT    0x10        /*OSD right control*/
#define OSDCTRLLEFT     0x20        /*OSD left control*/

#define DISABLE_KEYBOARD 0x02        // disable keyboard while OSD is active

#define KEY_MENU  0x007
#define KEY_PGUP  0x17D
#define KEY_PGDN  0x17A
#define KEY_HOME  0x16C
#define KEY_ESC   0x076
#define KEY_ENTER 0x05A
#define KEY_BACK  0x066
#define KEY_SPACE 0x029
#define KEY_UP    0x175
#define KEY_DOWN  0x172
#define KEY_LEFT  0x16B
#define KEY_RIGHT 0x174
#define KEY_F1    0x005
#define KEY_F2    0x006
#define KEY_F3    0x004
#define KEY_F4    0x00C
#define KEY_F5    0x003
#define KEY_F6    0x00B
#define KEY_F7    0x083
#define KEY_F8    0x00A
#define KEY_F9    0x001
#define KEY_F10   0x009

/*functions*/
void OSD_Write(uint8_t row, const char *s, uint8_t invert);
void OSD_WriteRC(uint8_t row, uint8_t col, const char *s, uint8_t invert, uint8_t fg_col, uint8_t bg_col );
void OSD_WriteBase(uint8_t row, uint8_t col, const char *s, uint8_t invert, uint8_t fg_col, uint8_t bg_col, uint8_t clear );

void OSD_SetHOffset(uint8_t row, uint8_t col, uint8_t pix);
void OSD_SetVOffset(uint8_t row);

uint8_t OSD_NextPage(void);
uint8_t OSD_GetPage(void);
void OSD_SetPage(uint8_t page);
void OSD_SetDisplay(uint8_t page);
void OSD_SetDisplayScroll(uint8_t page);

void OSD_WriteScroll(uint8_t row, uint8_t col, const char *s, uint8_t invert, uint8_t fg_col, uint8_t bg_col, uint8_t clear );

void OSD_Clear(void);
void OSD_Enable(unsigned char mode);
void OSD_Disable(void);
void OSD_Reset(unsigned char option);
void OSD_ConfigSendUserD(uint32_t configD);
void OSD_ConfigSendUserS(uint32_t configS);
void OSD_ConfigSendFileIO(uint32_t config);
void OSD_ConfigSendCtrl(uint32_t config);

uint8_t  OSD_ConfigReadSysconVer(void);
uint32_t OSD_ConfigReadVer(void);
uint32_t OSD_ConfigReadStatus(void);
uint32_t OSD_ConfigReadFileIO(void); // num disks etc

void OSD_WaitVBL(void);

//--> Replaced by structure in message.* module
//void OSD_BootPrint(const char *pText);

uint8_t OSD_ConvASCII(uint16_t c);
uint16_t OSD_GetKeyCode(void);

#endif
