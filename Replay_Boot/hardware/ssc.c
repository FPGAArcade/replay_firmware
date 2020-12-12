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
    // MMCK / ( 2 * SSC_CMR). Setting to 3 gives us ~8MHz.
    AT91C_BASE_SSC->SSC_CMR = 3;

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
