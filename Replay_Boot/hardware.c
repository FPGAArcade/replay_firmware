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

// hardware abstraction layer for Sam7S

#include "board.h"
#include "messaging.h"
#include "hardware/io.h"
#include "hardware/usart.h"

//
// General
//

#if defined(AT91SAM7S256)

void Hardware_Init(void)
{
    AT91C_BASE_WDTC->WDTC_WDMR = AT91C_WDTC_WDDIS; //disable watchdog
    AT91C_BASE_RSTC->RSTC_RMR = (0xA5 << 24) | AT91C_RSTC_URSTEN; //enable external user reset input

    // set up a safe default
    AT91C_BASE_PIOA->PIO_PER   = 0xFFFFFFFF; //grab all IOs
    AT91C_BASE_PIOA->PIO_PPUER = 0xFFFFFFFF; //enable all pullups (to be sure)
    AT91C_BASE_PIOA->PIO_ODR   = 0xFFFFFFFF; //disable all outputs
    AT91C_BASE_PIOA->PIO_CODR  = 0xFFFFFFFF; //clear all outputs (for open drain drive)

    //set outputs
    AT91C_BASE_PIOA->PIO_SODR = PIN_USB_PULLUP_L;
    AT91C_BASE_PIOA->PIO_SODR = PIN_FPGA_CTRL1 | PIN_FPGA_CTRL0 | PIN_CARD_CS_L;

    //output enable register
    AT91C_BASE_PIOA->PIO_OER = PIN_ACT_LED ;
    AT91C_BASE_PIOA->PIO_OER = PIN_USB_PULLUP_L;
    AT91C_BASE_PIOA->PIO_OER = PIN_FPGA_CTRL1 | PIN_FPGA_CTRL0 | PIN_CARD_CS_L;

    //pull-up disable register
    AT91C_BASE_PIOA->PIO_PPUDR = PIN_ACT_LED ;
    AT91C_BASE_PIOA->PIO_PPUDR = PIN_FPGA_INIT_L | PIN_FPGA_PROG_L; // open drains with external pull
    AT91C_BASE_PIOA->PIO_PPUDR = PIN_USB_PULLUP_L;
    AT91C_BASE_PIOA->PIO_PPUDR = PIN_FPGA_CTRL1 | PIN_FPGA_CTRL0;

    //external pulls, so disable internals
    AT91C_BASE_PIOA->PIO_PPUDR = PIN_CARD_CLK | PIN_CARD_MOSI | PIN_CARD_MISO | PIN_CARD_CS_L ;

    // other pullups disabled when peripherals enabled
    // Enable peripheral clock in the PMC
    AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_PIOA;

    // PIN_CONF_CCLK, PIN_CONF_DIN set up in SSC

    // CODER
    AT91C_BASE_PIOA->PIO_OER   = PIN_CODER_NTSC_H | PIN_CODER_CE;
    AT91C_BASE_PIOA->PIO_PPUDR = PIN_CODER_NTSC_H | PIN_CODER_CE;

    AT91C_BASE_PIOA->PIO_CODR  = PIN_CODER_NTSC_H;
    AT91C_BASE_PIOA->PIO_CODR  = PIN_CODER_CE;

    AT91C_BASE_PIOA->PIO_OER   = PIN_DTXD_Y1 | PIN_DRXD_Y2;
    AT91C_BASE_PIOA->PIO_PPUDR = PIN_DTXD_Y1 | PIN_DRXD_Y2;
    AT91C_BASE_PIOA->PIO_SODR  = PIN_DTXD_Y1 | PIN_DRXD_Y2;

    // FPGA RESET
    AT91C_BASE_PIOA->PIO_CODR  = PIN_FPGA_RST_L;
    AT91C_BASE_PIOA->PIO_OER   = PIN_FPGA_RST_L;
    AT91C_BASE_PIOA->PIO_PPUDR = PIN_FPGA_RST_L;

    IO_Init();
}

#endif // defined(AT91SAM7S256)
