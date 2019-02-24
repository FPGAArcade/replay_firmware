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
#ifndef HARDWARE_IO_H_INCLUDED
#define HARDWARE_IO_H_INCLUDED

#include "hardware/timer.h"
#include "messaging.h"	// ugly.. 

void IO_DriveLow_OD(uint32_t pin);
void IO_DriveHigh_OD(uint32_t pin);
uint8_t IO_Input_H(uint32_t pin);
uint8_t IO_Input_L(uint32_t pin);
static inline void IO_ClearOutputData(uint32_t pin)
{
    AT91C_BASE_PIOA->PIO_CODR = pin;
}
static inline void IO_SetOutputData(uint32_t pin)
{
    AT91C_BASE_PIOA->PIO_SODR = pin;
}

extern volatile uint32_t vbl_counter;
static inline void IO_WaitVBL(void)
{
    uint32_t pioa_old = 0;
    uint32_t pioa = AT91C_BASE_PIOA->PIO_PDSR;
    uint32_t vbl = vbl_counter;
    HARDWARE_TICK timeout = Timer_Get(100);

    while (!(~pioa & pioa_old & PIN_CONF_DOUT) && vbl != vbl_counter) {
        pioa_old = pioa;
        pioa     = AT91C_BASE_PIOA->PIO_PDSR;

        if (Timer_Check(timeout)) {
            WARNING("IO_WaitVBL timeout");
            break;
        }
    }
}

void IO_Init(void);

#endif
