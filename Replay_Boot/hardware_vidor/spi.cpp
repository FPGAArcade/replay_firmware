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
/*
void vidor_SPI_init();
void vidor_SPI_beginTransaction();
uint8_t vidor_SPI_transfer(uint8_t data);
void vidor_SPI_endTransaction();

void vidor_SPI_setFreq(uint32_t freq);
uint32_t vidor_SPI_getFreq();
*/

#include "SPI.h"

static SPISettings settings;
static uint8_t chip_select_asserted = 0;
static const uint8_t chipSelectPin = PIN_SPI_SS;

static void vidor_SPI_init()
{
}

static void vidor_SPI_beginTransaction()
{
    if (!chip_select_asserted) {
        chip_select_asserted = 1;
        SPI.beginTransaction(settings);
    }
    digitalWrite(chipSelectPin, LOW);
}

static void vidor_SPI_endTransaction()
{
    digitalWrite(chipSelectPin, HIGH);
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
    pinMode(chipSelectPin, OUTPUT);
    digitalWrite(chipSelectPin, HIGH);

    SPI.begin();
    settings = SPISettings(250000, MSBFIRST, SPI_MODE0);
}


unsigned char rSPI(unsigned char outByte)
{
    return SPI.transfer(outByte);
}

void SPI_WriteBufferSingle(void* pBuffer, uint32_t length)
{
    printf("%s %p %08x -> %08x\r\n", __FUNCTION__, pBuffer, length);
}

void SPI_ReadBufferSingle(void* pBuffer, uint32_t length)
{
    printf("%s %p %08x <- %08x\r\n", __FUNCTION__, pBuffer, length);
}

void SPI_Wait4XferEnd(void)
{
    printf("%s\r\n", __FUNCTION__);
}

void SPI_EnableCard(void)
{
//    printf("%s\r\n", __FUNCTION__);
    vidor_SPI_beginTransaction();
}

void SPI_DisableCard(void)
{
//    printf("%s\r\n", __FUNCTION__);
    vidor_SPI_endTransaction();
}

void SPI_EnableFileIO(void)
{
    printf("%s\r\n", __FUNCTION__);
}

void SPI_DisableFileIO(void)
{
    printf("%s\r\n", __FUNCTION__);
}

void SPI_EnableOsd(void)
{
//    printf("%s\r\n", __FUNCTION__);
}

void SPI_DisableOsd(void)
{
//    printf("%s\r\n", __FUNCTION__);
}

void SPI_EnableDirect(void)
{
    printf("%s\r\n", __FUNCTION__);
}

void SPI_DisableDirect(void)
{
    printf("%s\r\n", __FUNCTION__);
}

unsigned char SPI_IsActive(void)
{
    printf("%s\r\n", __FUNCTION__);
    return 0;
}

void SPI_SetFreq400kHz()
{
    vidor_SPI_setFreq(400*1000);
    printf("%s -> %d\r\n", __FUNCTION__, vidor_SPI_getFreq());
}
void SPI_SetFreq20MHz()
{
    vidor_SPI_setFreq(20*1000*1000);
    printf("%s -> %d\r\n", __FUNCTION__, vidor_SPI_getFreq());
}
void SPI_SetFreq25MHz()
{
    vidor_SPI_setFreq(25*1000*1000);
    printf("%s -> %d\r\n", __FUNCTION__, vidor_SPI_getFreq());
}
void SPI_SetFreqDivide(uint32_t freqDivide)
{
    printf("%s\r\n", __FUNCTION__);
    vidor_SPI_setFreq(48054857 / freqDivide);
}
uint32_t SPI_GetFreq()
{
    printf("%s\r\n", __FUNCTION__);
    return vidor_SPI_getFreq();
}
} // extern "C"
