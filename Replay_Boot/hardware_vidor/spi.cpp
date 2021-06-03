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
    const uint32_t max_freq = F_CPU / SPI_MIN_CLOCK_DIVIDER;
    if (freq > max_freq)
        freq = max_freq;

    settings = SPISettings(freq, MSBFIRST, SPI_MODE3);
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
    settings = SPISettings(250000, MSBFIRST, SPI_MODE3);
}

static DmacDescriptor dmaDesc[2] __attribute__ ((aligned (16))) = { 0 };
static DmacDescriptor wbDesc[2]  __attribute__ ((aligned (16))) = { 0 };
static uint32_t dmaDummy __attribute__ ((aligned (16))) = 0;

static uint32_t StartDMA(void* mem, uint16_t size, uint8_t dir /* high is write/tx/mosi */)
{
    // MISO has higher prio than MOSI; thus channel 0 is for RX
    const int channel = dir == 1 ? 0x01 : 0x00;

    const volatile void* spi = &SERCOM1->SPI.DATA.reg;
    DmacDescriptor* desc = &dmaDesc[channel];

    // "When address incrementation is configured, SRCADDR/DSTADDR must be set to
    //  the address of the last beat transfer in the block transfer."
    // ( 19.6.2.7 'Addressing' / Atmel-42181G–SAM-D21_Datasheet–09/2015 )
    uint32_t addr = (uint32_t)mem + size;
    uint32_t incr = 1;

    // if no buffer was provided, let's just dummy source/sink the transfer
    if (!mem) {
        addr = (uint32_t)&dmaDummy;
        incr = 0;
    }

    DMAC->CHID.bit.ID = channel;
    DMAC->CHCTRLA.bit.SWRST = 1;

    desc->BTCTRL.bit.VALID = 1;
    desc->BTCNT.bit.BTCNT = size;

    DMAC->CHCTRLB.bit.TRIGACT = DMAC_CHCTRLB_TRIGACT_BEAT_Val;

    if (dir) { // write
        // source is memory; destination is peripheral
        desc->BTCTRL.bit.SRCINC = incr;
        desc->SRCADDR.bit.SRCADDR = addr;
        desc->DSTADDR.bit.DSTADDR = (uint32_t)spi;

        // trigger on TX ready
        DMAC->CHCTRLB.bit.TRIGSRC = SERCOM1_DMAC_ID_TX;

    } else {
        // source is peripheral; destination is memory
        desc->BTCTRL.bit.DSTINC = incr;
        desc->SRCADDR.bit.SRCADDR = (uint32_t)spi;
        desc->DSTADDR.bit.DSTADDR = addr;

        // trigger on RX done
        DMAC->CHCTRLB.bit.TRIGSRC = SERCOM1_DMAC_ID_RX;

    }

    // enable IRQ signaling and DMA channel
    DMAC->CHINTENSET.bit.TCMPL = 1;
    DMAC->CHCTRLA.bit.ENABLE = 1;

    return (1 << channel);
}

void SPI_DMA(const void* out, void* in, uint16_t length)
{
    PM->AHBMASK.bit.DMAC_ = 1;
    PM->APBBMASK.bit.DMAC_ = 1;

    DMAC->CTRL.bit.SWRST = 1;
    DMAC->CTRL.reg = DMAC_CTRL_LVLEN(0xF);

    DMAC->BASEADDR.bit.BASEADDR = (uint32_t)dmaDesc;
    DMAC->WRBADDR.bit.WRBADDR   = (uint32_t)wbDesc;

    uint32_t irqMask = 0;

    irqMask |= StartDMA(in, length, 0);
    irqMask |= StartDMA((void*)out, length, 1);

    DMAC->CTRL.bit.DMAENABLE = 1;

    HARDWARE_TICK timeout = Timer_Get(100);      // 100 ms timeout

    while ((DMAC->INTSTATUS.reg & irqMask) != irqMask) {
        if (Timer_Check(timeout)) {
            DEBUG(1, "SPI:DMA Timeout!");

            break;
        }
    }

    DMAC->CTRL.bit.DMAENABLE = 0;
    PM->APBBMASK.bit.DMAC_ = 0;
    PM->AHBMASK.bit.DMAC_ = 0;
}

void SPI_WriteBufferSingle(void* pBuffer, uint32_t length)
{
    uint8_t* p = (uint8_t*)pBuffer;

    // for (uint32_t i = 0; i < length; ++i) {
    //     rSPI(*p++);
    // }

    // Send buffer, and ignore incoming
    SPI_DMA(pBuffer, 0, length);
}

void SPI_ReadBufferSingle(void* pBuffer, uint32_t length)
{
    uint8_t* p = (uint8_t*)pBuffer;

    // for (uint32_t i = 0; i < length; ++i) {
    //     *p++ = rSPI(0x00);
    // }

    // Send bogus contents, and store incoming stream
    SPI_DMA(0, pBuffer, length);
}

void SPI_Wait4XferEnd(void)
{
    DEBUG(0, "%s NOT IMPLEMENTED!", __FUNCTION__);
}

void SPI_EnableCard(void)
{
    ACTLED_ON;
    vidor_SPI_beginTransaction(PIN_CARD_CS_L);
}

void SPI_DisableCard(void)
{
    vidor_SPI_endTransaction(PIN_CARD_CS_L);
    ACTLED_OFF;
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
    DEBUG(1, "%s -> %d", __FUNCTION__, vidor_SPI_getFreq());
}
void SPI_SetFreq20MHz()
{
    vidor_SPI_setFreq(20 * 1000 * 1000);
    DEBUG(1, "%s -> %d", __FUNCTION__, vidor_SPI_getFreq());
}
void SPI_SetFreq25MHz()
{
    vidor_SPI_setFreq(25 * 1000 * 1000);
    DEBUG(1, "%s -> %d", __FUNCTION__, vidor_SPI_getFreq());
}
void SPI_SetFreqDivide(uint32_t freqDivide)
{
    DEBUG(1, "%s %d -> %d", __FUNCTION__, freqDivide, 48054857 / freqDivide);
    vidor_SPI_setFreq(48054857 / freqDivide);
}
uint32_t SPI_GetFreq()
{
    // DEBUG(0, "%s -> %d", __FUNCTION__, vidor_SPI_getFreq());
    return vidor_SPI_getFreq() / (1000 * 1000);
}
} // extern "C"
