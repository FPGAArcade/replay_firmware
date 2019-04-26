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

#ifndef BOARD_H
#define BOARD_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include <stdint.h>
#if !defined(WIN32) && !defined(__APPLE__) && !defined(__linux__)
#if defined(AT91SAM7S256)
#include "common/AT91SAM7S256.h"
#elif defined(__SAMD21G18A__)
#include <Arduino.h>
#else
#error "Unknown board"
#endif
#endif

#define FALSE 0
#define TRUE 1
#define FS_FATBUF_SIZE 2048  // must be >= 2x block size
#define MAX_DISPLAY_FILENAME 24+1 // stores file name for display.

//#define OSD_DEBUG

#if defined(AT91SAM7S256)
#define __ramfunc __attribute__ ((long_call, section (".fastrun")))
#define __fastrun __attribute__ ((section (".fastrun")))
#else
#define __ramfunc
#define __fastrun
#endif


/*-------------------------------*/
/* SAM7Board Memories Definition */
/*-------------------------------*/
// The AT91SAM7S256 embeds a 64 kByte SRAM bank and 256 kByte Flash

#if defined(AT91SAM7S256)
#define  INT_SARM           0x00200000
#define  INT_SARM_REMAP     0x00000000

#define  INT_FLASH          0x00000000
#define  INT_FLASH_REMAP    0x01000000

#define  FLASH_PAGE_NB      AT91C_IFLASH_NB_OF_PAGES
#define  FLASH_PAGE_SIZE    AT91C_IFLASH_PAGE_SIZE
#elif defined(__SAMD21G18A__)
// ..
#else
#define  FLASH_PAGE_NB      0
#define  FLASH_PAGE_SIZE    0
#endif

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Clocks
//------------------------------------------------------------------------------
#define BOARD_MAINOSC           18432000
#define BOARD_MCK               48054857
//------------------------------------------------------------------------------

#define TB(data, bit) (data & (1 << bit)) // test bit
#define SB(data, bit) (data |= (1 << bit))  // set bit
#define CB(data, bit) (data &= ~(1 << bit)) // clear bit

// note bit 0 is most right
#define EBITS(data, pos, len) ((data >> pos) & ((1 << len) - 1))    // extract bits


//------------------------------------------------------------------------------
// Pins
//------------------------------------------------------------------------------
// PA31 Card Access Led         output
// PA30 CODER_NTSC_H            output
// PA29 CODER_CE                output
// PA28 SPI_WRITEPROTECT        input   (pullup)
// PA27 SPI_CARDDETECT          input   (pullup)
// PA26 SPI_DAT2                input   (pullup)
// PA25 PROGRAM                 output  (open drain)
// PA24 DONE                    input
// PA23 SPI_DAT1                input   (pullup)
// PA22 SPI_CS                  output
// PA21 CODER_FITTED_L          input   (pullup)
// PA20 RF - FPGA_CTRL1         output
// PA19 RK - CONF_CCLK
// PA18 RD - CONF_DOUT          input   (vblank)
// PA17 TD - CONF_DIN
// PA16 TK - USB_PULLUP         output
// PA15 TF - CONF_INIT
// PA14 SPI_CLK                 output  (SPI)
// PA13 SPI_MOSI                output  (SPI)
// PA12 SPI_MISO                input   (SPI)
// PA11 FPGA_CTRL0              output
// PA10 DTXD_Y1                 output  (RS232) output (Y trap)
// PA9  DRXD_Y2                 input   (RS232) output (Y trap)
// PA8  FPGA_RST_L              output  (open drain)
// PA7  USB VBUS Monitor        input
// PA6  TXD                     output  (RS232)
// PA5  RXD                     input   (RS232)
// PA4  SCL                     output  (TWI)
// PA3  SDA                     output  (TWI)
// PA2  not used
// PA1  not used
// PA0  Menu                    input   (pullup)

/// DBGU pins definition. Contains DRXD (PA9) and DTXD (PA10).
#define AT91C_DBGU_BAUD         115200   // Baud rate

#if defined(AT91SAM7S256)

#define PIN_ACT_LED             AT91C_PIO_PA31

#define PIN_CODER_NTSC_H        AT91C_PIO_PA30
#define PIN_CODER_CE            AT91C_PIO_PA29

#define PIN_WRITE_PROTECT       AT91C_PIO_PA28
#define PIN_CARD_DETECT         AT91C_PIO_PA27
#define PIN_CARD_DAT2           AT91C_PIO_PA26
#define PIN_FPGA_PROG_L         AT91C_PIO_PA25
#define PIN_FPGA_DONE           AT91C_PIO_PA24
#define PIN_CARD_DAT1           AT91C_PIO_PA23
#define PIN_CARD_CS_L           AT91C_PIO_PA22

#define PIN_CODER_FITTED_L      AT91C_PIO_PA21

#define PIN_FPGA_CTRL1          AT91C_PIO_PA20
#define PIN_CONF_CCLK           AT91C_PIO_PA19
#define PIN_CONF_DOUT           AT91C_PIO_PA18
#define PIN_CONF_DIN            AT91C_PIO_PA17
#define PIN_USB_PULLUP_L        AT91C_PIO_PA16 //0=enable 1=disable

#define PIN_FPGA_INIT_L         AT91C_PIO_PA15
#define PIN_CARD_CLK            AT91C_PIO_PA14
#define PIN_CARD_MOSI           AT91C_PIO_PA13
#define PIN_CARD_MISO           AT91C_PIO_PA12
#define PIN_FPGA_CTRL0          AT91C_PIO_PA11

#define PIN_DTXD_Y1             AT91C_PIO_PA10
#define PIN_DRXD_Y2             AT91C_PIO_PA9
#define PIN_FPGA_RST_L          AT91C_PIO_PA8
#define PIN_USB_VBUS_MON        AT91C_PIO_PA7

#define PIN_TXD                 AT91C_PIO_PA6
#define PIN_RXD                 AT91C_PIO_PA5
#define PIN_SCL                 AT91C_PIO_PA4
#define PIN_SDA                 AT91C_PIO_PA3
#define PIN_NOT_USED_PA2        AT91C_PIO_PA2
#define PIN_NOT_USED_PA1        AT91C_PIO_PA1
#define PIN_MENU_BUTTON         AT91C_PIO_PA0

#define DBGU_TXD                AT91C_PA10_DTXD
#define DBGU_RXD                AT91C_PA9_DRXD

#define BITSTREAM_LENGTH		746212	// bytes

#elif defined(ARDUINO_SAMD_MKRVIDOR4000)

#define PIN_ACT_LED             LED_BUILTIN
#define PIN_MENU_BUTTON         (1<<0)   // dont we have a button?

// use JTAG instead
#define PIN_FPGA_INIT_L         (1<<1)
#define PIN_FPGA_RST_L          (1<<2)
#define PIN_FPGA_PROG_L         (1<<3)
#define PIN_FPGA_DONE           (1<<4)

// unsupported
#define PIN_CODER_FITTED_L      (1<<5)
#define PIN_CARD_DETECT         (1<<6)
#define PIN_WRITE_PROTECT       (1<<7)

// coder
#define PIN_CODER_NTSC_H        0x1234
#define PIN_CODER_CE            0x1234
#define PIN_DTXD_Y1             0x1234
#define PIN_DRXD_Y2             0x1234

#define FPGA_DISABLE_EMBEDDED_CORE 1
#define BITSTREAM_LENGTH		510882	// bytes

#else // HOSTED

#define PIN_ACT_LED             "PIN_ACT_LED"
#define PIN_CODER_NTSC_H        "PIN_CODER_NTSC_H"
#define PIN_CODER_CE            "PIN_CODER_CE"
#define PIN_WRITE_PROTECT       "PIN_WRITE_PROTECT"
#define PIN_CARD_DETECT         "PIN_CARD_DETECT"
#define PIN_CARD_DAT2           "PIN_CARD_DAT2"
#define PIN_FPGA_PROG_L         "PIN_FPGA_PROG_L"
#define PIN_FPGA_DONE           "PIN_FPGA_DONE"
#define PIN_CARD_DAT1           "PIN_CARD_DAT1"
#define PIN_CARD_CS_L           "PIN_CARD_CS_L"
#define PIN_CODER_FITTED_L      "PIN_CODER_FITTED_L"
#define PIN_FPGA_CTRL1          "PIN_FPGA_CTRL1"
#define PIN_CONF_CCLK           "PIN_CONF_CCLK"
#define PIN_CONF_DOUT           "PIN_CONF_DOUT"
#define PIN_CONF_DIN            "PIN_CONF_DIN"
#define PIN_USB_PULLUP_L        "PIN_USB_PULLUP_L"
#define PIN_FPGA_INIT_L         "PIN_FPGA_INIT_L"
#define PIN_CARD_CLK            "PIN_CARD_CLK"
#define PIN_CARD_MOSI           "PIN_CARD_MOSI"
#define PIN_CARD_MISO           "PIN_CARD_MISO"
#define PIN_FPGA_CTRL0          "PIN_FPGA_CTRL0"
#define PIN_DTXD_Y1             "PIN_DTXD_Y1"
#define PIN_DRXD_Y2             "PIN_DRXD_Y2"
#define PIN_FPGA_RST_L          "PIN_FPGA_RST_L"
#define PIN_USB_VBUS_MON        "PIN_USB_VBUS_MON"
#define PIN_TXD                 "PIN_TXD"
#define PIN_RXD                 "PIN_RXD"
#define PIN_SCL                 "PIN_SCL"
#define PIN_SDA                 "PIN_SDA"
#define PIN_NOT_USED_PA2        "PIN_NOT_USED_PA2"
#define PIN_NOT_USED_PA1        "PIN_NOT_USED_PA1"
#define PIN_MENU_BUTTON         "PIN_MENU_BUTTON"
#define DBGU_TXD                "DBGU_TXD"
#define DBGU_RXD                "DBGU_RXD"

#define BITSTREAM_LENGTH		746212	// bytes

#endif // defined(AT91SAM7S256)

// Macros to toggle state of activity LED on board
#define ACTLED_OFF do { IO_DriveLow_OD(PIN_ACT_LED); } while (0)
#define ACTLED_ON  do { IO_DriveHigh_OD(PIN_ACT_LED); } while (0)

//---------------------------------------------------------------------------

#endif //#ifndef BOARD_H

