/*--------------------------------------------------------------------
 *                       Replay Firmware
 *                      www.fpgaarcade.com
 *                     All rights reserved.
 *
 *                     admin@fpgaarcade.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *--------------------------------------------------------------------
 *
 * Copyright (c) 2020, The FPGAArcade community (see AUTHORS.txt)
 *
 */

//
// Replay board configuration file
//
// This is for Replay-apps only. As they will be called from a configured
// Replay board, they do not include any INIT and SDCARD stuff.
//

#ifndef BOARD_H
#define BOARD_H

//------------------------------------------------------------------------------
//         Headers
//------------------------------------------------------------------------------

#include <stdint.h>
#include "AT91SAM7S256.h"

#define FALSE 0
#define TRUE 1

#define __ramfunc __attribute__ ((long_call, section (".fastrun")))

/*-------------------------------*/
/* SAM7Board Memories Definition */
/*-------------------------------*/
// The AT91SAM7S256 embeds a 64 kByte SRAM bank and 256 kByte Flash

#define  INT_SARM           0x00200000
#define  INT_SARM_REMAP     0x00000000

#define  INT_FLASH          0x00000000
#define  INT_FLASH_REMAP    0x01000000

#define  FLASH_PAGE_NB      AT91C_IFLASH_NB_OF_PAGES
#define  FLASH_PAGE_SIZE    AT91C_IFLASH_PAGE_SIZE

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


//---------------------------------------------------------------------------

#endif //#ifndef BOARD_H

