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
#include "../board.h"
#include "../hardware/ssc.h"
#include "jtag.h"

extern uint8_t pin_fpga_done;
static uint32_t written = 0;

// SSC
void SSC_Configure_Boot(void)
{
}

void SSC_EnableTxRx(void)
{
    pin_fpga_done = FALSE;
    written = 0;
    JTAG_Init();
    JTAG_Reset();
    JTAG_StartBitstream();
}

void SSC_DisableTxRx(void)
{
    if (JTAG_EndBitstream()) {
        MSG_error("Failed to configure FPGA (%d bytes written)", written);
        return;
    }

    if (written < BITSTREAM_LENGTH) {
        MSG_warning("Short bitstream (%d of %d bytes)", written, BITSTREAM_LENGTH);
    }

    DEBUG(0, "Successfully configured FPGA over JTAG");
    pin_fpga_done = TRUE;
    enableFpgaClock();
}

void SSC_Write(uint32_t frame)
{
    (void)frame;
}

void SSC_WaitDMA(void)
{
}

void SSC_WriteBufferSingle(void* pBuffer, uint32_t length, uint32_t wait)
{
    (void)wait; // no async support

    if (length == 0) {
        return;
    }

    written += length;
    bool done = written == BITSTREAM_LENGTH;

    FPGA_WriteBitstream((uint8_t*)pBuffer, length, done);
}
