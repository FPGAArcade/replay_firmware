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
// hardware abstraction layer for Sam7S

#include "board.h"
#include "messaging.h"

#define USB_USART 0

#if USB_USART==1
#include"cdc_enumerate.h"
extern struct _AT91S_CDC pCDC;
#endif

//
// General
//
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

}


