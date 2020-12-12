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

#ifndef HARDWARE_SPI_H_INCLUDED
#define HARDWARE_SPI_H_INCLUDED

#if defined(AT91SAM7S256)
#include "board.h"
#include "messaging.h"  // ugly.. 
#endif

void SPI_Init(void);
unsigned char rSPI(unsigned char outByte) __fastrun;
void SPI_WriteBufferSingle(void* pBuffer, uint32_t length);
void SPI_ReadBufferSingle(void* pBuffer, uint32_t length);

void SPI_Wait4XferEnd(void);
void SPI_EnableCard(void);
void SPI_DisableCard(void) __fastrun;
void SPI_EnableFileIO(void);
void SPI_DisableFileIO(void) __fastrun;
void SPI_EnableOsd(void);
void SPI_DisableOsd(void);
void SPI_EnableDirect(void);
void SPI_DisableDirect(void);

unsigned char SPI_IsActive(void);

static inline void _SPI_EnableFileIO()
{
#if defined(AT91SAM7S256)
    AT91C_BASE_PIOA->PIO_CODR = PIN_FPGA_CTRL0;
#else
    SPI_EnableFileIO();
#endif
}
static inline void _SPI_DisableFileIO()
{
#if defined(AT91SAM7S256)

    while (!(AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TXEMPTY));

    AT91C_BASE_PIOA->PIO_SODR = PIN_FPGA_CTRL0;
#else
    SPI_DisableFileIO();
#endif
}

static inline uint8_t _rSPI(uint8_t outByte)
{
#if defined(AT91SAM7S256)
    volatile __attribute__((unused)) uint32_t t = AT91C_BASE_SPI->SPI_RDR;  // unused, but is a must!

    while (!(AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TDRE));

    AT91C_BASE_SPI->SPI_TDR = outByte;

    while (!(AT91C_BASE_SPI->SPI_SR & AT91C_SPI_RDRF));

    return ((uint8_t)AT91C_BASE_SPI->SPI_RDR);
#else
    return rSPI(outByte);
#endif
}

static inline void _SPI_ReadBufferSingle(void* pBuffer, uint32_t length)
{
#if defined(AT91SAM7S256)
    Assert((AT91C_BASE_SPI->SPI_PTSR & (AT91C_PDC_TXTEN | AT91C_PDC_RXTEN)) == 0);

    AT91C_BASE_SPI->SPI_TPR  = (uint32_t) pBuffer;
    AT91C_BASE_SPI->SPI_TCR  = length;
    AT91C_BASE_SPI->SPI_RPR  = (uint32_t) pBuffer;
    AT91C_BASE_SPI->SPI_RCR  = length;
    AT91C_BASE_SPI->SPI_PTCR = AT91C_PDC_TXTEN | AT91C_PDC_RXTEN;

    while ((AT91C_BASE_SPI->SPI_SR & (AT91C_SPI_ENDTX | AT91C_SPI_ENDRX)) != (AT91C_SPI_ENDTX | AT91C_SPI_ENDRX) ) {};

    AT91C_BASE_SPI->SPI_PTCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS; // disable transmitter and receiver*/

#else
    SPI_ReadBufferSingle(pBuffer, length);

#endif
}

#if defined(ARDUINO_SAMD_MKRVIDOR4000)

void SPI_SetFreq400kHz();
void SPI_SetFreq20MHz();
void SPI_SetFreq25MHz();
void SPI_SetFreqDivide(uint32_t freqDivide);
uint32_t SPI_GetFreq();

#else

static inline void SPI_SetFreq400kHz()
{
#if defined(AT91SAM7S256)
    AT91C_BASE_SPI->SPI_CSR[0] = AT91C_SPI_CPOL | (120 << 8) | (2 << 24); //init clock 100-400 kHz
#endif
}
static inline void SPI_SetFreq20MHz()
{
#if defined(AT91SAM7S256)
    AT91C_BASE_SPI->SPI_CSR[0] = AT91C_SPI_CPOL | (3 << 8); // 16 MHz SPI clock (max 20 MHz for MMC card);
#endif
}
static inline void SPI_SetFreq25MHz()
{
#if defined(AT91SAM7S256)
    AT91C_BASE_SPI->SPI_CSR[0] = AT91C_SPI_CPOL | (2 << 8); // 24 MHz SPI clock (max 25 MHz for SD/SDHC card)
#endif
}
static inline void SPI_SetFreqDivide(uint32_t freqDivide)
{
#if defined(AT91SAM7S256)
    AT91C_BASE_SPI->SPI_CSR[0] = AT91C_SPI_CPOL | (freqDivide << 8);
#endif
}
static inline uint32_t SPI_GetFreq()
{
#if defined(AT91SAM7S256)
    uint32_t spiFreq = BOARD_MCK /
                       ((AT91C_BASE_SPI->SPI_CSR[0] & AT91C_SPI_SCBR) >> 8) /
                       1000000;
#else
    uint32_t spiFreq = 10;
#endif
    return spiFreq;
}

#endif

#endif
