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
#ifndef OSD_H_INCLUDED
#define OSD_H_INCLUDED

#include "board.h"
#include "config.h"

//#define OSD_DEBUG

#define OSDMAXLEN         32          // (64 if both pages are used)

// some constants
#define OSDNLINE          16          // number of lines of OSD
#define OSDLINELEN        32          // single line length in chars (64 actually)
//
#define OSDCMD_READSTAT   0x00
#define OSDCMD_READKBD    0x08

#define OSDCMD_CTRL       0x10

#define OSDCMD_CTRL_RES     0x01        // OSD reset command
#define OSDCMD_CTRL_HALT    0x02        // OSD reset command
#define OSDCMD_CTRL_RES_VID 0x04        // OSD reset command

#define OSDCMD_CONFIG     0x20
#define OSDCMD_SENDPS2    0x30
#define OSDCMD_DISABLE    0x40
#define OSDCMD_ENABLE     0x41
#define OSDCMD_SETHOFF    0x50
#define OSDCMD_SETVOFF    0x51
#define OSDCMD_WRITE      0xC0

#define REPEATDELAY     500         // repeat delay in 1ms units
#define REPEATRATE      50          // repeat rate in 1ms units

#define BUTTONDELAY     100         // replay button delay in 1ms units
#define FLASHDELAY      1000        // replay button delay to start flashing in 1ms units

#define PS2DELAY        10          // ps/2 delay in 1ms units
#define PS2FLAGSDELAY   500         // ps/2 delay in 1ms units

#define SER_ESC_SEQ_DELAY 5         // time to wait for next char in an USART escape-sequence, in 1ms units

//#define OSDCTRLUP       0x01        /*OSD up control*/
//#define OSDCTRLDOWN     0x02        /*OSD down control*/
//#define OSDCTRLSELECT   0x04        /*OSD select control*/
//#define OSDCTRLMENU     0x08        /*OSD menu control*/
//#define OSDCTRLRIGHT    0x10        /*OSD right control*/
//#define OSDCTRLLEFT     0x20        /*OSD left control*/

#define STF_NEWKEY 0x01

#define DISABLE_KEYBOARD 0x02        // disable keyboard while OSD is active

/** amount of space between end and start of repeated name */
#define OSD_SCROLL_BLANKSPACE 10

/* Key Flags/Modifiers */
#define KF_GUI      0x0400
#define KF_CTRL     0x0800
#define KF_SHIFT    0x1000
#define KF_ALT      0x2000
#define KF_REPEATED 0x4000
#define KF_RELEASED 0x8000

/* special keys */
#define KEY_POSx  0x100
#define KEY_HOME  0x101
#define KEY_PGUP  0x102
#define KEY_PGDN  0x103
#define KEY_END   0x104
#define KEY_UP    0x105
#define KEY_DOWN  0x106
#define KEY_LEFT  0x107
#define KEY_RIGHT 0x108
#define KEY_DEL   0x109
#define KEY_INS   0x10A

#define KEY_Fx    0x200
#define KEY_F1    0x201
#define KEY_F2    0x202
#define KEY_F3    0x203
#define KEY_F4    0x204
#define KEY_F5    0x205
#define KEY_F6    0x206
#define KEY_F7    0x207
#define KEY_F8    0x208
#define KEY_F9    0x209
#define KEY_F10   0x20A
#define KEY_F11   0x20B
#define KEY_F12   0x20C
#define KEY_MENU  0x20C

#define KEY_SYSRQ 0x3FC
#define KEY_PAUSE 0x3FD
#define KEY_FLASH 0x3FE
#define KEY_RESET 0x3FF

// other keys conform to ASCII codes
#define KEY_ESC   0x01B
#define KEY_ENTER 0x00D
#define KEY_BACK  0x008
#define KEY_TAB   0x009
#define KEY_SPACE 0x020

#define KF_MASK   0xF800    // mask for flags/modifiers
#define KEY_MASK  0x03FF    // mask for actual keys
#define KEY_MASK_ASCII 0x7f // mask for printable chars

/* OSD colors
bit
7      invert
6..0   char
11..8  fg colour
15..12 bg colour

[8:03 PM]
of these 2..0 RGB and bit 3 is high/low brightness

[8:06 PM]
8 is wasted (light black) and could be reused
*/

typedef enum _tOSDColor {
    BLACK        = 0x0,
    DARK_BLUE    = 0x1,
    DARK_GREEN   = 0x2,
    DARK_CYAN    = 0x3,
    DARK_RED     = 0x4,
    DARK_MAGENTA = 0x5,
    DARK_YELLOW  = 0x6,
    GRAY         = 0x7,
    _UNUSED_COL  = 0x8,
    BLUE         = 0x9,
    GREEN        = 0xa,
    CYAN         = 0xb,
    RED          = 0xc,
    MAGENTA      = 0xd,
    YELLOW       = 0xe,
    WHITE        = 0xf
} tOSDColor;

/*functions*/
void OSD_Write(uint8_t row, const char* s, uint8_t invert);
void OSD_WriteRC(uint8_t row, uint8_t col, const char* s, uint8_t invert, tOSDColor fg_col, tOSDColor bg_col );
void OSD_WriteRCt(uint8_t row, uint8_t col, const char* s, uint8_t maxlen, uint8_t invert, tOSDColor fg_col, tOSDColor bg_col ); // truncate
void OSD_WriteBase(uint8_t row, uint8_t col, const char* s, uint8_t maxlen, uint8_t invert, tOSDColor fg_col, tOSDColor bg_col, uint8_t clear );

void OSD_SetHOffset(uint8_t row, uint8_t col, uint8_t pix);
void OSD_SetVOffset(uint8_t row);

uint8_t OSD_NextPage(void);
uint8_t OSD_GetPage(void);
void OSD_SetPage(uint8_t page);
void OSD_SetDisplay(uint8_t page);
void OSD_SetDisplayScroll(uint8_t page);

void OSD_WriteScroll(uint8_t row, const char* text, uint16_t pos, uint16_t len, uint8_t invert, tOSDColor fg_col, tOSDColor bg_col);

void OSD_Clear(void);
void OSD_Enable(unsigned char mode);
void OSD_Disable(void);
void OSD_Reset(unsigned char option);
void OSD_ConfigSendUserD(uint32_t configD);
void OSD_ConfigSendUserS(uint32_t configS);
void OSD_ConfigSendCtrl(uint32_t config);
void OSD_ConfigSendFileIO_CHA(uint32_t config);
void OSD_ConfigSendFileIO_CHB(uint32_t config);

uint8_t  OSD_ConfigReadSysconVer(void);
uint32_t OSD_ConfigReadVer(void);
uint32_t OSD_ConfigReadStatus(void);
uint32_t OSD_ConfigReadFileIO_Ena(void);
uint32_t OSD_ConfigReadFileIO_Drv(void);

void OSD_WaitVBL(void);

uint8_t OSD_ConvASCII(uint8_t keycode);
uint8_t OSD_ConvFx(uint8_t keycode);
uint8_t OSD_ConvPosx(uint8_t keycode);
uint16_t OSD_ConvFlags(uint8_t keycode1, uint8_t keycode2, uint8_t keycode3);

uint16_t OSD_GetKeyCode(status_t* current_status);

uint16_t OSD_GetKeyCodeFromString(const char* string);
const char* OSD_GetStringFromKeyCode(uint16_t keycode);

void OSD_SendPS2(uint16_t keycode);

#endif
