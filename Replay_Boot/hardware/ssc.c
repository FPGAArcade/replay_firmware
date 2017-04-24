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
#include "board.h"
#include "hardware/ssc.h"

// SSC
void SSC_Configure_Boot(void)
{
    // Enables CCLK and DIN as outputs. Note CCLK is wired to RK and we need to set
    // loop mode to get this to work.
    //
    // Enable SSC peripheral clock
    AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_SSC;

    // Reset, disable receiver & transmitter
    AT91C_BASE_SSC->SSC_CR   = AT91C_SSC_RXDIS | AT91C_SSC_TXDIS | AT91C_SSC_SWRST;
    AT91C_BASE_SSC->SSC_PTCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS;

    // Configure clock frequency
    //AT91C_BASE_SSC->SSC_CMR = BOARD_MCK / (2 * 10000000); // 10MHz
    AT91C_BASE_SSC->SSC_CMR = BOARD_MCK / (2 * 12500000); // 12.5MHz max for ARM pingroup 1
    // Configure TX

    // Enable pull ups
    AT91C_BASE_PIOA->PIO_PPUER = PIN_CONF_CCLK | PIN_CONF_DIN;

    // Configure TX
    AT91C_BASE_SSC->SSC_TCMR = AT91C_SSC_START_CONTINOUS | AT91C_SSC_CKO_DATA_TX | AT91C_SSC_CKS_DIV; // transmit clock mode
    AT91C_BASE_SSC->SSC_TFMR = 0x7 << 16 | AT91C_SSC_MSBF | 0x7; // transmit frame mode

    // Configure RX
    AT91C_BASE_SSC->SSC_RCMR = AT91C_SSC_START_TX | AT91C_SSC_CKO_DATA_TX | AT91C_SSC_CKS_TK;   // receive clock mode*/
    AT91C_BASE_SSC->SSC_RFMR = 0x7 << 16 | 1 << 5 |  AT91C_SSC_MSBF | 0x7 ; // receive frame mode*/

    // Assign clock and data to SSC (peripheral A)
    AT91C_BASE_PIOA->PIO_ASR = PIN_CONF_CCLK | PIN_CONF_DIN;
    AT91C_BASE_PIOA->PIO_PDR = PIN_CONF_CCLK | PIN_CONF_DIN;

    // Disable PIO drive (we need to float later)
    AT91C_BASE_PIOA->PIO_ODR = PIN_CONF_CCLK | PIN_CONF_DIN;

    // IO_Init is part of the SSC group, external pullup
    AT91C_BASE_PIOA->PIO_ODR  = PIN_FPGA_INIT_L;
    AT91C_BASE_PIOA->PIO_CODR = PIN_FPGA_INIT_L;
    AT91C_BASE_PIOA->PIO_PER  = PIN_FPGA_INIT_L;

    // DMA static
    AT91C_BASE_SSC->SSC_TNCR = 0;
    AT91C_BASE_SSC->SSC_RNCR = 0;
}

void SSC_EnableTxRx(void)
{
    AT91C_BASE_SSC->SSC_CR = AT91C_SSC_TXEN |  AT91C_SSC_RXEN;

    // Assign clock and data to SSC (peripheral A)
    AT91C_BASE_PIOA->PIO_ASR = PIN_CONF_DIN;
    AT91C_BASE_PIOA->PIO_PDR = PIN_CONF_DIN;
}

void SSC_DisableTxRx(void)
{
    AT91C_BASE_SSC->SSC_CR = AT91C_SSC_TXDIS | AT91C_SSC_RXDIS;

    // drive DIN high
    AT91C_BASE_PIOA->PIO_SODR = PIN_CONF_DIN;
    AT91C_BASE_PIOA->PIO_PER  = PIN_CONF_DIN;
    AT91C_BASE_PIOA->PIO_OER  = PIN_CONF_DIN;
}

void SSC_Write(uint32_t frame)
{
    while ((AT91C_BASE_SSC->SSC_SR & AT91C_SSC_TXRDY) == 0);

    AT91C_BASE_SSC->SSC_THR = frame;
}

void SSC_WaitDMA(void)
{
    // wait for buffer to be sent
    while ((AT91C_BASE_SSC->SSC_SR & AT91C_SSC_ENDTX ) != AT91C_SSC_ENDTX) {}; // no timeout
}

void SSC_WriteBufferSingle(void* pBuffer, uint32_t length, uint32_t wait)
{
    // single bank only, assume idle on entry
    AT91C_BASE_SSC->SSC_TPR = (unsigned int) pBuffer;
    AT91C_BASE_SSC->SSC_TCR = length;
    AT91C_BASE_SSC->SSC_PTCR = AT91C_PDC_TXTEN;

    // wait for transmission only if requested
    if (wait) {
        SSC_WaitDMA();
    }
}
