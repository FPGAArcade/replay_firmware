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

#include "osd.h"
#include "hardware.h"
#include "hardware/io.h"
#include "hardware/spi.h"
#include "hardware/usart.h"
#include "hardware/timer.h"
#include "messaging.h"
#include "config.h"

////
// Escape sequence definitions
//
// If you find a missing mapping, add it where it's length fits
// in a sorted list, as the code relies on the list to be sorted.
//

// Escape sequence struct, used for mapping sequence to key code
typedef struct _tEscSequence {
    const uint16_t code; // The translated keycode
    const uint8_t  slen; // Length of 'seq'
    const uint8_t* seq;  // Sequence following 'pfx', 'slen' bytes long
} tEscSequence;

// VT100 TAB, length 1
const static uint8_t TAB_KEYSEQ[1]   = {0x48};
const static uint8_t SHIFTTAB_KEYSEQ[2]  = {0x5b, 0x5a};
// - Cursor keys, length 2
const static uint8_t UP_KEYSEQ[2]    = {0x5b, 0x41};
const static uint8_t DOWN_KEYSEQ[2]  = {0x5b, 0x42};
const static uint8_t RIGHT_KEYSEQ[2] = {0x5b, 0x43};
const static uint8_t LEFT_KEYSEQ[2]  = {0x5b, 0x44};
// - Shifted cursor keys, length 2
const static uint8_t SHIFTUP_KEYSEQ[2]    = {0x4f, 0x41};
const static uint8_t SHIFTDOWN_KEYSEQ[2]  = {0x4f, 0x42};
const static uint8_t SHIFTRIGHT_KEYSEQ[2] = {0x4f, 0x43};
const static uint8_t SHIFTLEFT_KEYSEQ[2]  = {0x4f, 0x44};
// VT100 PF1-PF4
const static uint8_t PF1_KEYSEQ[2]   = {0x4f, 0x50};
const static uint8_t PF2_KEYSEQ[2]   = {0x4f, 0x51};
const static uint8_t PF3_KEYSEQ[2]   = {0x4f, 0x52};
const static uint8_t PF4_KEYSEQ[2]   = {0x4f, 0x53};
// - Special keys, length 3
const static uint8_t HOME_KEYSEQ[3]  = {0x5b, 0x31, 0x7e};
const static uint8_t INS_KEYSEQ[3]   = {0x5b, 0x32, 0x7e};
const static uint8_t DEL_KEYSEQ[3]   = {0x5b, 0x33, 0x7e};
const static uint8_t END_KEYSEQ[3]   = {0x5b, 0x34, 0x7e};
const static uint8_t PGUP_KEYSEQ[3]  = {0x5b, 0x35, 0x7e};
const static uint8_t PGDN_KEYSEQ[3]  = {0x5b, 0x36, 0x7e};
// - Function keys, length 4
const static uint8_t F1_KEYSEQ[4]    = {0x5b, 0x31, 0x31, 0x7e};
const static uint8_t F2_KEYSEQ[4]    = {0x5b, 0x31, 0x32, 0x7e};
const static uint8_t F3_KEYSEQ[4]    = {0x5b, 0x31, 0x33, 0x7e};
const static uint8_t F4_KEYSEQ[4]    = {0x5b, 0x31, 0x34, 0x7e};
const static uint8_t F5_KEYSEQ[4]    = {0x5b, 0x31, 0x35, 0x7e};
const static uint8_t F6_KEYSEQ[4]    = {0x5b, 0x31, 0x37, 0x7e};
const static uint8_t F7_KEYSEQ[4]    = {0x5b, 0x31, 0x38, 0x7e};
const static uint8_t F8_KEYSEQ[4]    = {0x5b, 0x31, 0x39, 0x7e};
const static uint8_t F9_KEYSEQ[4]    = {0x5b, 0x32, 0x30, 0x7e};
const static uint8_t F10_KEYSEQ[4]   = {0x5b, 0x32, 0x31, 0x7e};
const static uint8_t F11_KEYSEQ[4]   = {0x5b, 0x32, 0x33, 0x7e};
const static uint8_t F12_KEYSEQ[4]   = {0x5b, 0x32, 0x34, 0x7e};

// List of all the sequence mappings.
// @IMPORTANT: Keep sorted on slen!
const static tEscSequence usart_esc_sequences[] = {
    // Length 1, VT100 tab
    {.code = KEY_TAB,   .slen = sizeof(TAB_KEYSEQ),  .seq = TAB_KEYSEQ},
    {.code = KF_SHIFT | KEY_TAB,  .slen = sizeof(SHIFTTAB_KEYSEQ), .seq = SHIFTTAB_KEYSEQ},

    // Length 2, cursor keys
    {.code = KEY_UP,    .slen = sizeof(UP_KEYSEQ),   .seq = UP_KEYSEQ},
    {.code = KEY_DOWN,  .slen = sizeof(DOWN_KEYSEQ), .seq = DOWN_KEYSEQ},
    {.code = KEY_RIGHT, .slen = sizeof(RIGHT_KEYSEQ), .seq = RIGHT_KEYSEQ},
    {.code = KEY_LEFT,  .slen = sizeof(LEFT_KEYSEQ), .seq = LEFT_KEYSEQ},

    // Length 2, shifted cursor keys
    {.code = KF_SHIFT | KEY_UP,    .slen = sizeof(SHIFTUP_KEYSEQ),   .seq = SHIFTUP_KEYSEQ},
    {.code = KF_SHIFT | KEY_DOWN,  .slen = sizeof(SHIFTDOWN_KEYSEQ), .seq = SHIFTDOWN_KEYSEQ},
    {.code = KF_SHIFT | KEY_RIGHT, .slen = sizeof(SHIFTRIGHT_KEYSEQ), .seq = SHIFTRIGHT_KEYSEQ},
    {.code = KF_SHIFT | KEY_LEFT,  .slen = sizeof(SHIFTLEFT_KEYSEQ), .seq = SHIFTLEFT_KEYSEQ},

    // Length 3, VT100 PF1-PF4, here mapped to F1-F4
    {.code = KEY_F1,  .slen = sizeof(PF1_KEYSEQ), .seq = PF1_KEYSEQ},
    {.code = KEY_F2,  .slen = sizeof(PF2_KEYSEQ), .seq = PF2_KEYSEQ},
    {.code = KEY_F3,  .slen = sizeof(PF3_KEYSEQ), .seq = PF3_KEYSEQ},
    {.code = KEY_F4,  .slen = sizeof(PF4_KEYSEQ), .seq = PF4_KEYSEQ},

    // Length 3 special keys
    {.code = KEY_HOME, .slen = sizeof(HOME_KEYSEQ), .seq = HOME_KEYSEQ},
    {.code = KEY_INS,  .slen = sizeof(INS_KEYSEQ),  .seq = INS_KEYSEQ},
    {.code = KEY_DEL,  .slen = sizeof(DEL_KEYSEQ),  .seq = DEL_KEYSEQ},
    {.code = KEY_END,  .slen = sizeof(END_KEYSEQ),  .seq = END_KEYSEQ},
    {.code = KEY_PGUP, .slen = sizeof(PGUP_KEYSEQ), .seq = PGUP_KEYSEQ},
    {.code = KEY_PGDN, .slen = sizeof(PGDN_KEYSEQ), .seq = PGDN_KEYSEQ},

    // Length 4 function keys. Note that F1-F4 may be sent as PF1-PF4 if VT100
    {.code = KEY_F1,  .slen = sizeof(F1_KEYSEQ), .seq = F1_KEYSEQ},
    {.code = KEY_F2,  .slen = sizeof(F2_KEYSEQ), .seq = F2_KEYSEQ},
    {.code = KEY_F3,  .slen = sizeof(F3_KEYSEQ), .seq = F3_KEYSEQ},
    {.code = KEY_F4,  .slen = sizeof(F4_KEYSEQ), .seq = F4_KEYSEQ},
    {.code = KEY_F5,  .slen = sizeof(F5_KEYSEQ), .seq = F5_KEYSEQ},
    {.code = KEY_F6,  .slen = sizeof(F6_KEYSEQ), .seq = F6_KEYSEQ},
    {.code = KEY_F7,  .slen = sizeof(F7_KEYSEQ), .seq = F7_KEYSEQ},
    {.code = KEY_F8,  .slen = sizeof(F8_KEYSEQ), .seq = F8_KEYSEQ},
    {.code = KEY_F9,  .slen = sizeof(F9_KEYSEQ), .seq = F9_KEYSEQ},
    {.code = KEY_F10, .slen = sizeof(F10_KEYSEQ), .seq = F10_KEYSEQ},
    {.code = KEY_F11, .slen = sizeof(F11_KEYSEQ), .seq = F11_KEYSEQ},
    {.code = KEY_F12, .slen = sizeof(F12_KEYSEQ), .seq = F12_KEYSEQ}
};


// nasty globals ...
uint8_t osd_vscroll = 0;
uint8_t osd_page = 0;

// a ? e1:e2 e1 a/=0, e2 a==0
void OSD_Write(uint8_t row, const char* s, uint8_t invert)
{
    // clears until end of line
    OSD_WriteBase(row, 0 , s, 0, invert, 0xF, 0, 1);
}

void OSD_WriteRC(uint8_t row, uint8_t col, const char* s, uint8_t invert, tOSDColor fg_col, tOSDColor bg_col )
{
    //
    OSD_WriteBase(row, col, s, 0, invert, fg_col, bg_col, 0);
}

void OSD_WriteRCt(uint8_t row, uint8_t col, const char* s, uint8_t maxlen, uint8_t invert, tOSDColor fg_col, tOSDColor bg_col ) // truncate
{
    //
    OSD_WriteBase(row, col, s, maxlen, invert, fg_col, bg_col, 0);
}


// write a null-terminated string <s> to the OSD buffer starting at line row, col. If maxlen non zero, truncate string to maxlen chars
void OSD_WriteBase(uint8_t row, uint8_t col, const char* s, uint8_t maxlen, uint8_t invert, tOSDColor fg_col, tOSDColor bg_col, uint8_t clear )
{
    uint16_t i;
    uint8_t b;
    uint8_t attrib = (fg_col & 0xF) | ((bg_col & 0xF) << 4);
    /*uint8_t col_track;*/

    if (invert) {
        attrib = (fg_col & 0xF) | (0x4 << 4);
    }

#ifdef OSD_DEBUG
    DEBUG(1, "OsdWrite %s ", s);

    if (invert) {
        DEBUG(1, "<< ");

    } else {
        DEBUG(1, "   ");
    }

#endif

    // select OSD SPI device
    SPI_EnableOsd();
    rSPI(OSDCMD_WRITE | (row & 0x3F));
    rSPI(col + osd_page * OSDLINELEN);

    i = 0;

    /*col_track = col;*/
    // send all characters in string to OSD
    while (1) {
        b = *s++;

        if (b == 0) { // end of string
            break;
        }

        else if (b == 0x0D || b == 0x0A) { // cariage return / linefeed, go to next line
            // increment line counter
            if (++row >= OSDNLINE) {
                row = 0;
            }

            SPI_DisableOsd();
            // send new line number to OSD
            SPI_EnableOsd();
            rSPI(OSDCMD_WRITE | (row & 0x3F)); // col 0
            rSPI(col + osd_page * OSDLINELEN);

        } else { // normal character
            /*if (col_track >= OSDLINELEN) {*/
            /*DEBUG(1,"OSD WRAP row %d", row);*/
            /*}*/
            rSPI(b);
            rSPI(attrib); // attrib
            i++;

            if (i == maxlen) {
                break;
            }

            /*col_track++;*/
        }
    }

    if (clear) {
        for (; i < OSDLINELEN; i++) { // clear end of line
            rSPI(0x20);
            rSPI(attrib);
        }
    }

    // deselect OSD SPI device
    SPI_DisableOsd();
}


void OSD_SetHOffset(uint8_t row, uint8_t col, uint8_t pix)
{
    SPI_EnableOsd();
    rSPI(OSDCMD_WRITE | (row & 0x3F));
    // col not set for address
    SPI_DisableOsd();

    SPI_EnableOsd();
    rSPI(OSDCMD_SETHOFF);
    rSPI( ((col & 0x3F) << 2) | ((pix & 0xC) >> 2) );
    rSPI(pix & 0x3);
    SPI_DisableOsd();
}

void OSD_SetVOffset(uint8_t row)
{
    SPI_EnableOsd();
    rSPI(OSDCMD_SETVOFF);
    rSPI(row);
    SPI_DisableOsd();
}

uint8_t OSD_NextPage(void)
{
    return (osd_page + 1) % 2;
};

uint8_t OSD_GetPage(void)
{
    return osd_page;
};

void OSD_SetPage(uint8_t page)
{
    osd_page = page;
}

void OSD_SetDisplay(uint8_t page)
{
    uint8_t row = 0;
    OSD_WaitVBL();

    for (row = 0; row < OSDNLINE; row ++) {
        OSD_SetHOffset(row, page * OSDLINELEN, 0);
    }
}

// text points at long name, pos is the start position, len is the string len
void OSD_WriteScroll(uint8_t row, const char* text, uint16_t pos, uint16_t len, uint8_t invert, tOSDColor fg_col, tOSDColor bg_col)
{
    uint16_t i;
    uint8_t attrib = (fg_col & 0xF) | ((bg_col & 0xF) << 4);

    if (invert) {
        attrib = (fg_col & 0xF) | (0x4 << 4);
    }

    char s[OSDMAXLEN + 1];
    memset(s, ' ', OSDMAXLEN + 1); // clear line buffer

    int remaining = len - pos;

    if (remaining > OSDMAXLEN + 1) {
        remaining = OSDMAXLEN + 1;
    }

    if (remaining > 0) {
        strncpy(s, &text[pos], remaining);
    }

    if (remaining < OSDMAXLEN + 1 - OSD_SCROLL_BLANKSPACE) {
        strncpy(s + remaining + OSD_SCROLL_BLANKSPACE, text, OSDMAXLEN + 1 - remaining - OSD_SCROLL_BLANKSPACE);
    }

    SPI_EnableOsd();
    rSPI(OSDCMD_WRITE | (row & 0x3F));
    rSPI(0); // col

    // need to write to len + 1 for the scroll...
    for (i = 0; i < OSDMAXLEN + 1; i++) {
        rSPI(s[i]);
        rSPI(attrib);
    }

    SPI_DisableOsd();
}

// clear OSD frame buffer
void OSD_Clear(void)
{
    uint8_t  row;
    uint16_t n;

#ifdef OSD_DEBUG
    DEBUG(1, "OsdClear");
#endif

    for (row = 0; row < OSDNLINE; row++) {
        SPI_EnableOsd();
        rSPI(OSDCMD_WRITE | (row & 0x3F));
        rSPI(osd_page * OSDLINELEN);

        // clear buffer
        for (n = 0; n < OSDLINELEN; n++) {
            rSPI(0x20);
            rSPI(0x0F);
        }

        SPI_DisableOsd();
    }

    OSD_SetVOffset(0);
    osd_vscroll = 0;
}

void OSD_WaitVBL(void)
{
    uint32_t pioa_old = 0;
    uint32_t pioa = 0;
    uint32_t timeout = Timer_Get(100);

    while ((~pioa ^ pioa_old) & PIN_CONF_DOUT) {
        pioa_old = pioa;
        pioa     = AT91C_BASE_PIOA->PIO_PDSR;

        if (Timer_Check(timeout)) {
            WARNING("OSDWaitVBL timeout");
            break;
        }
    }
}

// enable displaying of OSD
void OSD_Enable(unsigned char mode)
{
    SPI_EnableOsd();
    rSPI(OSDCMD_ENABLE | (mode & 0xF));
    SPI_DisableOsd();
}

// disable displaying of OSD
void OSD_Disable(void)
{
    SPI_EnableOsd();
    rSPI(OSDCMD_DISABLE);
    SPI_DisableOsd();
}

void OSD_Reset(unsigned char option)
{
    // soft reset or halt
    SPI_EnableOsd();
    rSPI(OSDCMD_CTRL | option);
    SPI_DisableOsd();
}

void OSD_ConfigSendUserD(uint32_t configD)
{
    // Dynamic config
    SPI_EnableOsd();
    rSPI(OSDCMD_CONFIG | 0x01); // dynamic
    rSPI((uint8_t)(configD));
    rSPI((uint8_t)(configD >> 8));
    rSPI((uint8_t)(configD >> 16));
    rSPI((uint8_t)(configD >> 24));
    SPI_DisableOsd();
}

void OSD_ConfigSendUserS(uint32_t configS)
{
    // Static config
    SPI_EnableOsd();
    rSPI(OSDCMD_CONFIG | 0x00); // static
    rSPI((uint8_t)(configS));
    rSPI((uint8_t)(configS >> 8));
    rSPI((uint8_t)(configS >> 16));
    rSPI((uint8_t)(configS >> 24));
    SPI_DisableOsd();
}


void OSD_ConfigSendCtrl(uint32_t config)
{
    SPI_EnableOsd();
    rSPI(OSDCMD_CONFIG | 0x03); // ctrl
    rSPI((uint8_t)(config));
    rSPI((uint8_t)(config >> 8));
    SPI_DisableOsd();
}

//0x28 write fileio hd             W0 7..4 hd_wen, 3..0 hd_ins DYNAMIC
void OSD_ConfigSendFileIO_CHA(uint32_t config)
{
    SPI_EnableOsd();
    rSPI(OSDCMD_CONFIG | 0x08); // fileio
    rSPI((uint8_t)(config));
    SPI_DisableOsd();
}

//0x29 write fileio fd             W0 7..4 fd_wen, 3..0 fd_ins DYNAMIC
void OSD_ConfigSendFileIO_CHB(uint32_t config)
{
    SPI_EnableOsd();
    rSPI(OSDCMD_CONFIG | 0x09); // fileio
    rSPI((uint8_t)(config));
    SPI_DisableOsd();
}

uint8_t OSD_ConfigReadSysconVer(void)
{
    uint8_t config;

    SPI_EnableOsd();
    rSPI(OSDCMD_READSTAT | 0x01);
    config = rSPI(0);
    SPI_DisableOsd();
    return config;
}

uint32_t OSD_ConfigReadVer(void)
{
    uint32_t config;

    SPI_EnableOsd();
    rSPI(OSDCMD_READSTAT | 0x03); // high word
    config  = (rSPI(0) & 0xFF) << 8;
    SPI_DisableOsd();

    SPI_EnableOsd();
    rSPI(OSDCMD_READSTAT | 0x02); // low word
    config |= (rSPI(0) & 0xFF);
    SPI_DisableOsd();

    return config;
}

uint32_t OSD_ConfigReadStatus(void)
{
    uint32_t config;

    SPI_EnableOsd();
    rSPI(OSDCMD_READSTAT | 0x07); // high word
    config  = (rSPI(0) & 0xFF) << 8;
    SPI_DisableOsd();

    SPI_EnableOsd();
    rSPI(OSDCMD_READSTAT | 0x06); // low word
    config |= (rSPI(0) & 0xFF);
    SPI_DisableOsd();

    return config;
}

uint32_t OSD_ConfigReadFileIO_Ena(void) // num disks supported
{
    uint32_t config;

    SPI_EnableOsd();
    rSPI(OSDCMD_READSTAT | 0x04);
    config  = (rSPI(0) & 0xFF);
    SPI_DisableOsd();
    return config; // HD mask & FD mask
}


uint32_t OSD_ConfigReadFileIO_Drv(void) // driver
{
    uint32_t config;

    SPI_EnableOsd();
    rSPI(OSDCMD_READSTAT | 0x05);
    config  = (rSPI(0) & 0xFF);
    SPI_DisableOsd();
    return config; // HD mask & FD mask
}

// conversion table of ps/2 scan codes to ASCII codes
static const char keycode_table[128] =
    // in: keycode as given by matrix (in HEX); out: ASCII code (as char)
{
    // x0  x1  x2  x3  x4  x5  x6  x7   x8  x9  xA  xB  xC  xD  xE  xF
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  9,  '`',  0,   // 0x
    0,  0,  0,  0,  0, 'Q', '1', 0,   0,  0, 'Z', 'S', 'A', 'W', '2', 0, // 1x
    0, 'C', 'X', 'D', 'E', '4', '3', 0,   0, ' ', 'V', 'F', 'T', 'R', '5', 0, // 2x
    0, 'N', 'B', 'H', 'G', 'Y', '6', 0,   0,  0, 'M', 'J', 'U', '7', '8', 0, // 3x
    0, ',', 'K', 'I', 'O', '0', '9', 0,   0, '.',  '/', 'L', ';', 'P', '-',  0, // 4x
    0,  0,  '\'',  0,  '[',  '=',  0,  0,   0,  0, 13,  ']',  0,  '\\',  0,  0,   // 5x
    0,  '\\',  0,  0,  0,  0,  8,  0,   0, '1', 0, '4', '7', 0,  0,  0,  // 6x
    '0', '.', '2', '5', '6', '8', 27,  0,   0,  '+', '3', '-',  '*', '9', 0,  0 // 7x
};
uint8_t OSD_ConvASCII(uint8_t keycode)
{
    if (keycode & 0x80) {
        return 0;
    }

    // note, this converts from ps2 keycode to ASCII
    return keycode_table[keycode & 0x7F];
}

// function keys
static const uint8_t Fx_KEY[] = {0x05, 0x06, 0x04, 0x0C, 0x03, 0x0B, 0x83, 0x0A, 0x01, 0x09, 0x78, 0x07};

uint8_t OSD_ConvFx(uint8_t keycode)
{
    uint8_t i;

    for (i = 0; i < 12; ++i) {
        if (Fx_KEY[i] == keycode) {
            return i + 1;
        }
    }

    return 0;
}

// position keys
static const uint8_t Pos_KEY[] = {0x6C, 0x7D, 0x7A, 0x69, 0x75, 0x72, 0x6B, 0x74, 0x71, 0x70};
uint8_t OSD_ConvPosx(uint8_t keycode)
{
    uint8_t i;

    for (i = 0; i < 10; ++i) {
        if (Pos_KEY[i] == keycode) {
            return i + 1;
        }
    }

    return 0;
}

uint16_t OSD_ConvFlags(uint8_t keycode1, uint8_t keycode2, uint8_t keycode3)
{
    // special keys
    const uint8_t KEY_SHIFTL  = 0x12;
    const uint8_t KEY_SHIFTR  = 0x59;
    const uint8_t KEY_ALT     = 0x11;
    const uint8_t KEY_CTRL    = 0x14;
    const uint8_t KEY_GUIL    = 0x1f;
    const uint8_t KEY_GUIR    = 0x27;

    if ((keycode1 == KEY_SHIFTL) || (keycode1 == KEY_SHIFTR)) {
        return KF_SHIFT;
    }

    if ((keycode1 == KEY_ALT)    || (keycode2 == KEY_ALT)) {
        return KF_ALT;
    }

    if ((keycode1 == KEY_CTRL)   || (keycode2 == KEY_CTRL)) {
        return KF_CTRL;
    }

    if ((keycode2 == KEY_GUIL)   || (keycode3 == KEY_GUIL)) {
        return KF_GUI;
    }

    if ((keycode2 == KEY_GUIR)   || (keycode3 == KEY_GUIR)) {
        return KF_GUI;
    }

    return 0;
}

uint16_t OSD_GetKeyCode(uint8_t osd_enabled, uint16_t hotkey)
{
    // front menu button stuff
    static uint32_t button_delay;
    static uint16_t  button_pressed = 0;
    static uint32_t flash_delay;

    // PS/2 stuff
    uint16_t x = 0;
    uint16_t key_break = 0;
    uint16_t key_code = 0;

    static uint32_t ps2_delay = 0;
    static uint32_t ps2_flags_delay = 0;

    static uint8_t keybuf[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // max. amount of codes with one keypress (PAUSE key)
    static uint16_t keypos = 0;

    static uint16_t key_flags = 0;
    static uint16_t old_key_code = 0;

    // RS232 escape sequence char timeout
    static uint32_t esc_delay = 0;
    static uint16_t meta_key = 0;

    // check front menu button
    // ---------------------------------------------------
    if (IO_Input_L(PIN_MENU_BUTTON)) {
        if (!button_pressed) {
            key_code = KEY_MENU;
            button_pressed = 1;
            button_delay = Timer_Get(BUTTONDELAY);
            flash_delay = Timer_Get(FLASHDELAY);

        } else if (Timer_Check(flash_delay)) {
            // a long press will call the flash loader (rApp)
            // or cause a "hard" board reset
            DEBUG(1, "--> TRY TO FLASH");
            return KEY_FLASH;
        }

    } else if (Timer_Check(button_delay)) {
        button_pressed = 0;
    }

    // check and buffer keycodes from FPGA (if available)
    // ---------------------------------------------------
    if (osd_enabled) {
        SPI_EnableOsd();
        rSPI(OSDCMD_READSTAT);
        x = rSPI(0);
        SPI_DisableOsd();

        if (x & STF_NEWKEY) {
            SPI_EnableOsd();
            rSPI(OSDCMD_READKBD);
            x = rSPI(0);
            SPI_DisableOsd();

            keybuf[keypos++] = x;
            ps2_delay = Timer_Get(PS2DELAY);

            if (keypos == 1) {
                DEBUG(3, "ps2: %d: %02x", keypos, keybuf[0]);
            }

            if (keypos == 2) {
                DEBUG(3, "ps2: %d: %02x %02x", keypos, keybuf[0], keybuf[1]);
            }

            if (keypos == 3) {
                DEBUG(3, "ps2: %d: %02x %02x %02x", keypos, keybuf[0], keybuf[1], keybuf[2]);
            }

            if (keypos == 4) {
                DEBUG(3, "ps2: %d: %02x %02x %02x %02x", keypos, keybuf[0], keybuf[1], keybuf[2], keybuf[3]);
            }

        } else if (Timer_Check(ps2_delay)) {
            // we clean up the buffer on a timeout
            keypos = 0;
        }

    } else {
        // clean up if no OSD available
        keypos = 0;
    }

    // time out for flags


    // process key buffer
    // ---------------------------------------------------
    if (keypos && (!key_code)) { // menu push button takes priority

        // break sequence (key release)
        if ((keybuf[1] == 0xF0) && (keypos == 3)) {
            keybuf[1] = keybuf[2];
            keypos = 2;
            key_break = KF_RELEASED;
        }

        if ((keybuf[0] == 0xF0) && (keypos == 2)) {
            keybuf[0] = keybuf[1];
            keypos = 1;
            key_break = KF_RELEASED;
        }

        // check "modifier" keys first
        x = OSD_ConvFlags(keybuf[0], keypos == 2 ? keybuf[1] : 0, keypos == 3 ? keybuf[2] : 0);

        if (x) {
            ps2_flags_delay = Timer_Get(PS2FLAGSDELAY); // reset timer if we get a modifier

            if (key_break) {
                key_flags &= ~x;

            } else {
                key_flags |= x;
            }

            keypos = 0;
            key_code = 0;
            // otherwise check key sequences

        } else {
            x = OSD_ConvASCII(keybuf[0]);

            if (x) {
                key_code = key_break | key_flags | x;
                keypos = 0;

            } else {
                x = OSD_ConvFx(keybuf[0]);

                if (x) {
                    key_code = key_break | key_flags | KEY_Fx | x;
                    keypos = 0;

                } else {
                    x = (keybuf[0] == 0xE0) && (keypos == 2) ? OSD_ConvPosx(keybuf[1]) : 0;

                    if (x) {
                        key_code = key_break | key_flags | KEY_POSx | x;
                        keypos = 0;

                    } else {
                        if ((keybuf[0] == 0xE1) && (keypos == 8)) {
                            key_code = key_flags | KEY_PAUSE;
                            keypos = 0;

                        } else {
                            if (((keybuf[0] == 0xE0) && (keybuf[1] != 0xF0) && (keypos == 2)) || ((keybuf[0] != 0xE0) && (keybuf[0] != 0xF0))) {
                                keypos = 0; // ignore rest of any possible keys
                            }
                        }
                    }
                }
            }
        }

        if (key_code == hotkey) {
            OSD_SendPS2(KF_RELEASED | key_code);		// send key-up / break to the core
            key_code = key_break | KEY_MENU;
            DEBUG(2, "Hotkey (%04X) detected ; Overriding with KEY_MENU (%04X)", hotkey, key_code);
        }

        if (key_code & KEY_MASK) {
            if (key_code == old_key_code) {
                key_code |= KF_REPEATED;

            } else {
                old_key_code = key_code;
            }
        }
    }

    if (key_flags && Timer_Check(ps2_flags_delay)) {
        key_flags = 0;
        old_key_code = 0;
    }

    if (key_code) {
        DEBUG(2, "PS/2 Key: 0x%04X", key_code);
        return key_code;
    }

    // process RS232 inputs
    // ---------------------------------------------------
    if (!USART_GetValid()) { // fixes bug where peekc returns 0 (no char), but is valid by the time key_code=USART_Getc(); happens
        return 0;
    }

    if (USART_Peekc() != 0x1b) {
        // Not in, or starting new, ESC sequence
        key_code = USART_Getc();

        DEBUG(3, "usart: %04x", key_code);

        if ((key_code > 0x60) && (key_code < 0x7B)) {
            key_code &= 0x5F; // set uppercase

        } else if ((key_code > 0x40) && (key_code < 0x5B)) {
            key_code |= KF_SHIFT;

        } else if ((key_code > 0) && (key_code < 0x20)) {
            if ( key_code == 0x1F) {
                key_code = KF_CTRL | 0x2F;

            } else if (key_code != KEY_BACK &&
                       key_code != KEY_TAB &&
                       key_code != KEY_ENTER &&
                       key_code != KEY_ESC) {
                // Control-[A-Z], except special keys (back,tab,enter,esc)
                key_code = KF_CTRL | (key_code + 0x40);
            }
        }

        if (key_code == 0x7f) {  // If backspace sent as 'Control-? (127)' - settings in PuTTY
            key_code = KEY_BACK;
        }

        // attach any previous META/ALT key
        key_code |= meta_key;
        meta_key = 0;

    } else {
        // esc sequence, partial or complete

        // peek the buffer to see how many chars we got
        // atm we need at most 5, but try to fill up the keybuf (which was 8 when writing this)
        uint16_t keybuf_size = USART_PeekBuf((uint8_t*)&keybuf, sizeof(keybuf));
        // @FIXME: Re-uses 'keybuf'. Should also reset 'keypos' ?
        DEBUG(3, "esc: keybuf_size: %d", keybuf_size);

        if (keybuf_size > 1) { // 1+ chars in buffer, might be an esc seq
            uint8_t esc_seq_idx = 0;
            uint8_t n_matches = 0;
            const tEscSequence* mes = NULL;

            do { // Iterate through esc sequences.
                const tEscSequence* eks = &usart_esc_sequences[esc_seq_idx];
                const uint8_t slen = eks->slen;
                const uint8_t* seq = eks->seq;

                // esc_sequences are sorted on slen, so we bail early if slen is larger than keybuf_size
                if (slen > keybuf_size - 1) {
                    break;
                }

                DEBUG(3, "esc: check seq %d len %d> 0x%02x 0x%02x ..", esc_seq_idx, slen, seq[0], seq[1]);

                if (slen <= keybuf_size - 1 &&         // -1 to comp for first char, 0x1B
                        seq[0] == keybuf[1] &&              // optimization, compare first byte
                        memcmp(seq + 1, &keybuf[2], slen - 1) == 0) { // compare rest
                    // Escape sequences are prefix-unique, meaning that if we find a second
                    // match for a prefix, it is not unique, hence we'll need to wait for more
                    // input, or a timeout.
                    n_matches++;

                    if (n_matches > 1) {
                        // second match for this keybuf/prefix, so it's not unique :(
                        mes = NULL;
                        break;

                    } else {
                        mes = eks;
                    }
                }
            } while (++esc_seq_idx < sizeof(usart_esc_sequences) / sizeof(tEscSequence));

            if (mes != NULL) { // if set, there was only 1 match (the first)
                DEBUG(3, "esc: sequence detected: key=0x%02x", mes->code);
                key_code = mes->code;

                // Use slen+1, not keybuf_size, entire seq matched, + two leading bytes
                if (USART_GetBuf(keybuf, mes->slen + 1) == 0) {
                    DEBUG(1, "WARN: esc: USART_GetBuf failed to pick off the esc seq!");
                }

            } else if (n_matches != 0) {
                // There was more than 1 match, we "know" we can expect more chars
                // @CHECKME: Could this run us into a wall of infinite timeout restarts ?
                esc_delay = 0;// results in `new Timer_Get(SER_ESC_SEQ_DELAY)` below
            }
        }

        if (key_code == 0) {
            // ESC was (previously) hit, but did not translate to a key_code yet.
            // either we start waiting for more chars, or we already are.
            if (esc_delay == 0) {
                // new timeout to wait for rest of ESC sequence
                esc_delay = Timer_Get(SER_ESC_SEQ_DELAY);
                DEBUG(3, "esc: start timeout");
                key_code = 0; // or return 0, but we want logging below

            } else if (esc_delay != 0 &&  // (else+check for neater debug logging)
                       Timer_Check(esc_delay)) {
                esc_delay = 0; // Cancel the timeout

                if (keybuf_size > 2 && keybuf[1] != 0x1b) { // don't log double taps..
                    DEBUG(0, "Unknown esc sequence! keybuf size %d => 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x",
                          keybuf_size, keybuf[0], keybuf[1], keybuf[2], keybuf[3], keybuf[4], keybuf[5]);
                }

                // If we get here, there is (most likely) only a single ESC in keybuf,
                // and/or no seq matched. Consider this as if only the ESC key was hit.
                // But if we have an ESC+single char, then this should be treated as a META/ALT key-combo,
                if (keybuf_size == 2) {
                    meta_key = KF_ALT;

                } else {
                    key_code = KEY_ESC;
                }

                USART_Getc(); // skip ESC key
                // If there are more chars in the keybuf (more ESC / unknown sequence),
                // they're emitted later

            } else {
                DEBUG(3, "esc: ..waiting for more chars");
            }

        } else {
            // We got a key_code! cancel any pending timeout
            esc_delay = 0;
        }

    } // } else { // esc sequence, partial or complete

    if (key_code != 0) {
        DEBUG(2, "USART Key: 0x%04X", key_code);
    }

    return key_code;
}

// Conversion from string to keycode ('ctrl-c' => 0x0843), and reverse.
static struct {
    const char* string;
    uint16_t keycode;
} StringToKeycode [] = {
    { "GUI"   , KF_GUI    },
    { "CTRL"  , KF_CTRL   },
    { "SHIFT" , KF_SHIFT  },
    { "ALT"   , KF_ALT    },
    { "HOME"  , KEY_HOME  },
    { "PGUP"  , KEY_PGUP  },
    { "PGDN"  , KEY_PGDN  },
    { "END"   , KEY_END   },
    { "UP"    , KEY_UP    },
    { "DOWN"  , KEY_DOWN  },
    { "LEFT"  , KEY_LEFT  },
    { "RIGHT" , KEY_RIGHT },
    { "DEL"   , KEY_DEL   },
    { "INS"   , KEY_INS   },
    { "F1"    , KEY_F1    },
    { "F2"    , KEY_F2    },
    { "F3"    , KEY_F3    },
    { "F4"    , KEY_F4    },
    { "F5"    , KEY_F5    },
    { "F6"    , KEY_F6    },
    { "F7"    , KEY_F7    },
    { "F8"    , KEY_F8    },
    { "F9"    , KEY_F9    },
    { "F10"   , KEY_F10   },
    { "F11"   , KEY_F11   },
    { "F12"   , KEY_F12   },
    { "MENU"  , KEY_MENU  },
    { "ESC"   , KEY_ESC   },
    { "ENTER" , KEY_ENTER },
    { "BACK"  , KEY_BACK  },
    { "TAB"   , KEY_TAB   },
    { "SPACE" , KEY_SPACE },
    { 0       , 0         } // end-of-table
};

uint16_t OSD_GetKeyCodeFromString(const char* string)
{
    const char* value = string;
    uint16_t keycode = 0;
    uint16_t token_length;
    const size_t num_mappings = sizeof(StringToKeycode) / sizeof(StringToKeycode[0]);

    DEBUG(3, "Parsing keycode from '%s'", value);

    while (value && *value) {
        // First find the substring (token) delimiters
        token_length = 0;

        while (*value && !isalnum((uint8_t)*value)) {
            ++value;
        }

        for (token_length = 0; isalnum((uint8_t) * (value + token_length)); ++token_length)
            ;

        if (!token_length) {
            break;
        }

        DEBUG(3, "Token is %d chars; starting with '%s'", token_length, value);

        // Next find the keycode matching the substring (or, ASCII if the token has length of 1)
        if (token_length > 1) {
            for (int i = 0; i < num_mappings; ++i) {
                const char* strI = StringToKeycode[i].string;
                uint16_t keyI = StringToKeycode[i].keycode;

                if (!strI) {
                    DEBUG(1, "Unknown key substring - not defined by StringToKeycode mapping : %s", value);
                    return 0;
                }

                uint16_t key = keycode & KEY_MASK;
                uint8_t is_modifer = (keyI & KEY_MASK) == 0;

                if (strnicmp(value, strI, token_length) == 0) {
                    // Check if we already have a full keycode defined
                    if (!is_modifer && key != 0) {
                        DEBUG(1, "Illegal keycode combo ; Found %s, but key already defined %04x", strI, keycode);
                        return 0;
                    }

                    DEBUG(3, "Found substring '%s'", strI);
                    keycode |= keyI;
                    break;
                }
            }

        } else {
            char c = toupper((uint8_t) * value);

            if (keycode & KEY_MASK) {
                DEBUG(1, "Illegal keycode combo ; Found %c, but key already defined %04x", c, keycode);
                return 0;
            }

            DEBUG(3, "Found substring '%c'", c);
            keycode |= c;
        }

        value += token_length;
    }

    DEBUG(3, "Decoded '%s' to %04X / '%s'", string, keycode, OSD_GetStringFromKeyCode(keycode));
    return keycode;
}

const char* OSD_GetStringFromKeyCode(uint16_t keycode)
{
    static char buffer[32];
    const size_t num_mappings = sizeof(StringToKeycode) / sizeof(StringToKeycode[0]);
    buffer[0] = 0;

    for (int i = 0; i < num_mappings; ++i) {
        const char* strI = StringToKeycode[i].string;
        uint16_t keyI = StringToKeycode[i].keycode;

        if (keycode == 0 || strI == 0) {
            break;
        }

        uint16_t key = keycode & KEY_MASK;
        uint8_t is_modifer = (keyI & KEY_MASK) == 0;
        uint8_t modifier_matches = (keycode & keyI) == keyI;
        uint8_t key_matches = (key == (keyI & KEY_MASK));

        if ((key_matches && !is_modifer) || (is_modifer && modifier_matches)) {
            if (buffer[0] != 0) {
                strcat(buffer, "-");
            }

            strcat(buffer, strI);
            keycode &= ~keyI;
        }
    }

    if (keycode != 0) {
        char c[2] = {0, 0};

        if (buffer[0] != 0) {
            strcat(buffer, "-");
        }

        c[0] = keycode & KEY_MASK;
        strcat(buffer, c);
    }

    DEBUG(3, "Converted keycode to string '%s'", buffer);
    return buffer;
}

// Conversion from uppercase symbols to  lowercase 'keys'.
static const struct {
    uint8_t shifted, regular;
} unshift_table[] = {
    { '!', '1' },
    { '"', '\'' },
    { '#', '3' },
    { '$', '4' },
    { '%', '5' },
    { '&', '7' },
    { '(', '9' },
    { ')', '0' },
    { '*', '8' },
    { '+', '=' },
    { ':', ';' },
    { '<', ',' },
    { '>', '.' },
    { '?', '/' },
    { '@', '2' },
    { '^', '6' },
    { '_', '-' },
    { '{', '[' },
    { '|', '\\' },
    { '}', ']' },
    { '~', '`' },
};

static __attribute__ ((noinline)) void _OSD_SendKey(uint8_t key)
{
    DEBUG(2, "SPI: sendkey %d / 0x%02x", key, key);
    SPI_EnableOsd();
    rSPI(OSDCMD_SENDPS2);
    rSPI(key);
    Timer_Wait(1);
    SPI_DisableOsd();
}

void OSD_SendPS2(uint16_t keycode)
{
    const uint8_t gui   = (keycode & KF_GUI) ? 1 : 0;
    const uint8_t ctrl  = (keycode & KF_CTRL) ? 1 : 0;
    const uint8_t shift = (keycode & KF_SHIFT) ? 1 : 0;
    const uint8_t alt   = (keycode & KF_ALT) ? 1 : 0;
    const uint8_t make  = (keycode & KF_RELEASED) ? 0 : 1;
    uint8_t shifted     = 0;    // for keys that have been 'unshifted'
    uint8_t extended    = 0;    // E0 type scancode
    uint8_t scancode    = 0;    // PS2 scancode

    DEBUG(2, "OSD_SendPS2(0x%04x)", keycode);

    keycode &= KEY_MASK;

    if (keycode <= KEY_MASK_ASCII) {
        // map lowercase [a-z] to uppercase
        if ('a' <= keycode && keycode <= 'z') {
            keycode &= 0x5F;

        } else {
            // map "uppercase" symbols to their lowercased counterparts (assuming US keyb)
            for (int i = 0; i < sizeof(unshift_table) / sizeof(unshift_table[0]); ++i) {
                if (keycode == unshift_table[i].shifted) {
                    keycode = unshift_table[i].regular;
                    shifted = 1;
                    break;
                }
            }
        }

        DEBUG(2, "Trying to map ascii keycode '0x%02x' to a scancode", keycode);

        for (int i = 0; i < sizeof(keycode_table) / sizeof(keycode_table[0]); ++i) {
            if (keycode == keycode_table[i]) {
                scancode = i;
                break;
            }
        }

    } else if (KEY_F1 <= keycode && keycode <= KEY_F12) {
        keycode &= ~KEY_Fx;
        Assert(0 < keycode && keycode <= sizeof(Fx_KEY) / sizeof(Fx_KEY[0]));
        scancode = Fx_KEY[keycode - 1];

    } else if (KEY_HOME <= keycode && keycode <= KEY_INS) {
        keycode &= ~KEY_POSx;
        Assert(0 < keycode && keycode <= sizeof(Pos_KEY) / sizeof(Pos_KEY[0]));
        scancode = Pos_KEY[keycode - 1];
        extended = 1;
    }

    if (scancode == 0) {
        DEBUG(1, "Unable to map keycode '%04x' to a PS/2 scancode", keycode);
        return;
    }

    const uint8_t KEY_EXTEND  = 0xe0;
    const uint8_t KEY_BREAK   = 0xf0;

    // as we can't distinguish between left and right modifiers, we will send both
    const uint8_t KEY_SHIFTL  = 0x12;
    const uint8_t KEY_SHIFTR  = 0x59;
    const uint8_t KEY_ALT     = 0x11;
    const uint8_t KEY_CTRL    = 0x14;
    const uint8_t KEY_GUIL    = 0x1f;
    const uint8_t KEY_GUIR    = 0x27;

    if (make) {
        // send make for all modifier first, and then the scancode

        if (gui) {
            _OSD_SendKey(KEY_EXTEND);
            _OSD_SendKey(KEY_GUIL);
            _OSD_SendKey(KEY_EXTEND);
            _OSD_SendKey(KEY_GUIR);
        }

        if (ctrl) {
            _OSD_SendKey(KEY_CTRL);
            _OSD_SendKey(KEY_EXTEND);
            _OSD_SendKey(KEY_CTRL);
        }

        if (shift || shifted) {
            _OSD_SendKey(KEY_SHIFTL);
            _OSD_SendKey(KEY_SHIFTR);
        }

        if (alt) {
            _OSD_SendKey(KEY_ALT);
            _OSD_SendKey(KEY_EXTEND);
            _OSD_SendKey(KEY_ALT);
        }

        if (extended) {
            _OSD_SendKey(KEY_EXTEND);
        }

        _OSD_SendKey(scancode);

    } else {
        // send break of the scancode, then break for all modifiers

        _OSD_SendKey(KEY_BREAK);

        if (extended) {
            _OSD_SendKey(KEY_EXTEND);
        }

        _OSD_SendKey(scancode);

        if (alt) {
            _OSD_SendKey(KEY_BREAK);
            _OSD_SendKey(KEY_ALT);
            _OSD_SendKey(KEY_BREAK);
            _OSD_SendKey(KEY_EXTEND);
            _OSD_SendKey(KEY_ALT);
        }

        if (shift || shifted) {
            _OSD_SendKey(KEY_BREAK);
            _OSD_SendKey(KEY_SHIFTL);
            _OSD_SendKey(KEY_BREAK);
            _OSD_SendKey(KEY_SHIFTR);
        }

        if (ctrl) {
            _OSD_SendKey(KEY_BREAK);
            _OSD_SendKey(KEY_CTRL);
            _OSD_SendKey(KEY_BREAK);
            _OSD_SendKey(KEY_EXTEND);
            _OSD_SendKey(KEY_CTRL);
        }

        if (gui) {
            _OSD_SendKey(KEY_BREAK);
            _OSD_SendKey(KEY_EXTEND);
            _OSD_SendKey(KEY_GUIL);
            _OSD_SendKey(KEY_BREAK);
            _OSD_SendKey(KEY_EXTEND);
            _OSD_SendKey(KEY_GUIR);
        }
    }
}
