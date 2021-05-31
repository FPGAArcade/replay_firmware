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
#include "messaging.h"
#include "hardware/io.h"
#include "hardware/spi.h"
#include "hardware/timer.h"

//
// SPI
//
void SPI_Init(void)
{
    // Enable the peripheral clock in the PMC
    AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_SPI;

    // Disable SPI interface
    AT91C_BASE_SPI->SPI_CR = AT91C_SPI_SPIDIS;

    // Execute a software reset of the SPI twice
    AT91C_BASE_SPI->SPI_CR = AT91C_SPI_SWRST;
    AT91C_BASE_SPI->SPI_CR = AT91C_SPI_SWRST;

    // SPI Mode Register
    AT91C_BASE_SPI->SPI_MR = AT91C_SPI_MSTR | AT91C_SPI_MODFDIS  | (0x0E << 16);

    // SPI cs register
    AT91C_BASE_SPI->SPI_CSR[0] = AT91C_SPI_CPOL | (48 << 8) | (0x00 << 16) | (0x01 << 24);

    // Configure pins for SPI use
    AT91C_BASE_PIOA->PIO_PDR = AT91C_PA14_SPCK | AT91C_PA13_MOSI | AT91C_PA12_MISO;

    // Enable SPI interface
    AT91C_BASE_SPI->SPI_CR = AT91C_SPI_SPIEN;

    // DMA static
    AT91C_BASE_SPI->SPI_TNCR = 0;
    AT91C_BASE_SPI->SPI_RNCR = 0;
}

unsigned char rSPI(unsigned char outByte)  // inline?
{
    volatile uint32_t t = AT91C_BASE_SPI->SPI_RDR;
    // for t not to generate a warning, add a dummy asm that 'reference' t
    asm volatile(""
                 : "=r" (t));

    while (!(AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TDRE));

    AT91C_BASE_SPI->SPI_TDR = outByte;

    while (!(AT91C_BASE_SPI->SPI_SR & AT91C_SPI_RDRF));

    return ((unsigned char)AT91C_BASE_SPI->SPI_RDR);
}

void SPI_WriteBufferSingle(void* pBuffer, uint32_t length)
{
    Assert((AT91C_BASE_SPI->SPI_PTSR & (AT91C_PDC_TXTEN | AT91C_PDC_RXTEN)) == 0);

    // single bank only, assume idle on entry
    AT91C_BASE_SPI->SPI_TPR  = (uint32_t) pBuffer;
    AT91C_BASE_SPI->SPI_TCR  = length;
    AT91C_BASE_SPI->SPI_PTCR = AT91C_PDC_TXTEN;

    HARDWARE_TICK timeout = Timer_Get(100);      // 100 ms timeout

    while ((AT91C_BASE_SPI->SPI_SR & AT91C_SPI_ENDTX ) != AT91C_SPI_ENDTX) {
        if (Timer_Check(timeout)) {
            DEBUG(1, "SPI:WriteBufferSingle DMA Timeout!");

            break;
        }
    }

    AT91C_BASE_SPI ->SPI_PTCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS; // disable transmitter and receiver*/
}

void SPI_ReadBufferSingle(void* pBuffer, uint32_t length)
{
    // assume spi rx buffer is flushed on entry (previous SPI command)

    Assert((AT91C_BASE_SPI->SPI_PTSR & (AT91C_PDC_TXTEN | AT91C_PDC_RXTEN)) == 0);

    // we do not care what we send out (current buffer contents), the FPGA will ignore
    AT91C_BASE_SPI->SPI_TPR  = (uint32_t) pBuffer;
    AT91C_BASE_SPI->SPI_TCR  = length;
    AT91C_BASE_SPI->SPI_RPR  = (uint32_t) pBuffer;
    AT91C_BASE_SPI->SPI_RCR  = length;
    AT91C_BASE_SPI->SPI_PTCR = AT91C_PDC_TXTEN | AT91C_PDC_RXTEN;

    HARDWARE_TICK timeout = Timer_Get(100);      // 100 ms timeout

    while ((AT91C_BASE_SPI->SPI_SR & (AT91C_SPI_ENDTX | AT91C_SPI_ENDRX)) != (AT91C_SPI_ENDTX | AT91C_SPI_ENDRX) ) {
        if (Timer_Check(timeout)) {
            WARNING("SPI:ReadBufferSingle DMA Timeout!");
            break;
        }
    }

    AT91C_BASE_SPI ->SPI_PTCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS; // disable transmitter and receiver*/
}

inline void SPI_Wait4XferEnd(void)
{
    while (!(AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TXEMPTY));
}

inline void SPI_EnableCard(void)
{
    ACTLED_ON;
    AT91C_BASE_PIOA->PIO_CODR = PIN_CARD_CS_L;
}

void SPI_DisableCard(void)
{
    SPI_Wait4XferEnd();
    AT91C_BASE_PIOA->PIO_SODR = PIN_CARD_CS_L;
    rSPI(0xFF);
    SPI_Wait4XferEnd();
    ACTLED_OFF;
}

inline void SPI_EnableFileIO(void)
{
    AT91C_BASE_PIOA->PIO_CODR = PIN_FPGA_CTRL0;
}

void SPI_DisableFileIO(void)
{
    SPI_Wait4XferEnd();
    AT91C_BASE_PIOA->PIO_SODR = PIN_FPGA_CTRL0;
}

inline void SPI_EnableOsd(void)
{
    AT91C_BASE_PIOA->PIO_CODR = PIN_FPGA_CTRL1;
}

inline void SPI_DisableOsd(void)
{
    SPI_Wait4XferEnd();
    AT91C_BASE_PIOA->PIO_SODR = PIN_FPGA_CTRL1;
}

inline void SPI_EnableDirect(void)
{
    // used for direct card -> FPGA transfer
    // must be asserted before SPI cs is disabled
    // swaps rx/tx pins on FPGA.
    AT91C_BASE_PIOA->PIO_CODR = PIN_CONF_DIN;
}

inline void SPI_DisableDirect(void)
{
    AT91C_BASE_PIOA->PIO_SODR = PIN_CONF_DIN;
}

unsigned char SPI_IsActive(void)
{
    uint32_t mask = PIN_CARD_CS_L | PIN_FPGA_CTRL0 | PIN_FPGA_CTRL1 | PIN_CONF_DIN;

    if ((AT91C_BASE_PIOA->PIO_ODSR & mask) != mask) {
        return 1;
    }

    if (!(AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TXEMPTY)) {
        return 1;
    }

    return 0;
}
