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

#include "../board.h"
#include "SPI.h"

static SPISettings settings;
static uint8_t chip_select_asserted = 0;

static void vidor_SPI_beginTransaction(uint8_t cs)
{
    if (!chip_select_asserted) {
        chip_select_asserted = 1;
        SPI.beginTransaction(settings);
    }

    digitalWrite(cs, LOW);
}

static void vidor_SPI_endTransaction(uint8_t cs)
{
    digitalWrite(cs, HIGH);

    if (chip_select_asserted) {
        chip_select_asserted = 0;
        SPI.endTransaction();
    }
}

static void vidor_SPI_setFreq(uint32_t freq)
{
    settings = SPISettings(freq, MSBFIRST, SPI_MODE0);
}

static uint32_t vidor_SPI_getFreq()
{
    return settings.getClockFreq();
}


//
// SPI
//
extern "C" {

#include "../messaging.h"
#include "../hardware/spi.h"
#include "../hardware/timer.h"
#include "../osd.h"
#include "../fileio.h"

void SPI_Init(void)
{
    pinMode(PIN_CARD_CS_L,  OUTPUT);
    pinMode(PIN_CONF_DIN,   OUTPUT);
    pinMode(PIN_FPGA_CTRL1, OUTPUT);
    pinMode(PIN_FPGA_CTRL0, OUTPUT);
    digitalWrite(PIN_CARD_CS_L,  HIGH);
    digitalWrite(PIN_CONF_DIN,   HIGH);
    digitalWrite(PIN_FPGA_CTRL1, HIGH);
    digitalWrite(PIN_FPGA_CTRL0, HIGH);

    SPI.begin();
    settings = SPISettings(250000, MSBFIRST, SPI_MODE0);
}


unsigned char rSPI(unsigned char outByte)
{
    return SPI.transfer(outByte);
}

void SPI_WriteBufferSingle(void* pBuffer, uint32_t length)
{
    uint8_t* p = (uint8_t*)pBuffer;

    for (uint32_t i = 0; i < length; ++i) {
        rSPI(*p++);
    }
}

void SPI_ReadBufferSingle(void* pBuffer, uint32_t length)
{
    uint8_t* p = (uint8_t*)pBuffer;

    for (uint32_t i = 0; i < length; ++i) {
        *p++ = rSPI(0x00);
    }
}

void SPI_Wait4XferEnd(void)
{
    DEBUG(0, "%s", __FUNCTION__);
}

void SPI_EnableCard(void)
{
    vidor_SPI_beginTransaction(PIN_CARD_CS_L);
}

void SPI_DisableCard(void)
{
    vidor_SPI_endTransaction(PIN_CARD_CS_L);
}

void SPI_EnableFileIO(void)
{
    vidor_SPI_beginTransaction(PIN_FPGA_CTRL0);
}

void SPI_DisableFileIO(void)
{
    vidor_SPI_endTransaction(PIN_FPGA_CTRL0);
}

void SPI_EnableOsd(void)
{
    vidor_SPI_beginTransaction(PIN_FPGA_CTRL1);
}

void SPI_DisableOsd(void)
{
    vidor_SPI_endTransaction(PIN_FPGA_CTRL1);
}

void SPI_EnableDirect(void)
{
    digitalWrite(PIN_CONF_DIN, LOW);
}

void SPI_DisableDirect(void)
{
    digitalWrite(PIN_CONF_DIN, HIGH);
}

unsigned char SPI_IsActive(void)
{
    return chip_select_asserted != 0;
}

void SPI_SetFreq400kHz()
{
    vidor_SPI_setFreq(400 * 1000);
    DEBUG(0, "%s -> %d", __FUNCTION__, vidor_SPI_getFreq());
}
void SPI_SetFreq20MHz()
{
    vidor_SPI_setFreq(20 * 1000 * 1000);
    DEBUG(0, "%s -> %d", __FUNCTION__, vidor_SPI_getFreq());
}
void SPI_SetFreq25MHz()
{
    vidor_SPI_setFreq(25 * 1000 * 1000);
    DEBUG(0, "%s -> %d", __FUNCTION__, vidor_SPI_getFreq());
}
void SPI_SetFreqDivide(uint32_t freqDivide)
{
    DEBUG(0, "%s %d -> %d", __FUNCTION__, freqDivide, 48054857 / freqDivide);
    vidor_SPI_setFreq(48054857 / freqDivide);
}
uint32_t SPI_GetFreq()
{
    // DEBUG(0, "%s -> %d", __FUNCTION__, vidor_SPI_getFreq());
    return vidor_SPI_getFreq() / (1000 * 1000);
}
} // extern "C"
