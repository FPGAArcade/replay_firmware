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
#include "hardware/twi.h"
#include "hardware/io.h"
#include "hardware/timer.h"

void TWI_Configure(void)
{
    volatile __attribute__((unused)) uint32_t read;
    // PA4  SCL                     output  (TWI)
    // PA3  SDA                     output  (TWI)

    // bug fix for THS
    AT91C_BASE_PIOA->PIO_CODR = PIN_SCL | PIN_SDA;
    IO_DriveHigh_OD(PIN_SDA);

    for (uint8_t j = 0; j < 9; j++) {
        Timer_Wait(1);
        IO_DriveLow_OD(PIN_SCL);
        Timer_Wait(1);
        IO_DriveHigh_OD(PIN_SCL);
    }

    // Enable SSC peripheral clock
    AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_TWI;

    // Reset the TWI
    AT91C_BASE_TWI->TWI_CR = AT91C_TWI_SWRST;
    read = AT91C_BASE_TWI->TWI_RHR;
    // Disable
    AT91C_BASE_TWI->TWI_CR = AT91C_TWI_MSDIS;

    // Enable Master mode
    AT91C_BASE_TWI->TWI_CR = AT91C_TWI_MSEN;
    // Set clock. Note, max speed 100KHz
    AT91C_BASE_TWI->TWI_CWGR = 0;  // stop clock
    AT91C_BASE_TWI->TWI_CWGR = (0x2 << 16) | (0x77 << 8) | 0x77; // ~50KHz

    // disable pull ups
    AT91C_BASE_PIOA->PIO_PPUDR = PIN_SCL | PIN_SDA;
    // enable multi driver
    AT91C_BASE_PIOA->PIO_MDER  = PIN_SCL | PIN_SDA;
    // assign clock and data
    AT91C_BASE_PIOA->PIO_ASR = PIN_SCL | PIN_SDA;
    AT91C_BASE_PIOA->PIO_PDR = PIN_SCL | PIN_SDA;
}

void TWI_Stop(void)
{
    AT91C_BASE_TWI->TWI_CR = AT91C_TWI_STOP;
}

uint8_t TWI_ReadByte(void)
{
    return AT91C_BASE_TWI->TWI_RHR;
}

void TWI_WriteByte(uint8_t byte)
{
    AT91C_BASE_TWI->TWI_THR = byte;
}

uint8_t TWI_ByteReceived(void)
{
    return ((AT91C_BASE_TWI->TWI_SR & AT91C_TWI_RXRDY) == AT91C_TWI_RXRDY);
}

uint8_t TWI_ByteSent(void)
{
    return ((AT91C_BASE_TWI->TWI_SR & AT91C_TWI_TXRDY) == AT91C_TWI_TXRDY);
}

uint8_t TWI_TransferComplete(void)
{
    return ((AT91C_BASE_TWI->TWI_SR & AT91C_TWI_TXCOMP) == AT91C_TWI_TXCOMP);
}

void TWI_StartRead(uint8_t DevAddr, uint8_t IntAddrSize, uint16_t IntAddr)
{
    // send address. bit 0 is write_h (MMR)
    AT91C_BASE_TWI->TWI_MMR = (IntAddrSize << 8) | AT91C_TWI_MREAD | (DevAddr << 16);

    // Set internal address bytes
    AT91C_BASE_TWI->TWI_IADR = IntAddr;

    // Send START condition
    AT91C_BASE_TWI->TWI_CR = AT91C_TWI_START;
}

void TWI_StartWrite(uint8_t DevAddr, uint8_t IntAddrSize, uint16_t IntAddr, uint8_t WriteData)
{
    // send address. bit 0 is write_h (MMR)
    AT91C_BASE_TWI->TWI_MMR = (IntAddrSize << 8) | (DevAddr << 16);

    // Set internal address bytes
    AT91C_BASE_TWI->TWI_IADR = IntAddr;

    // Write first byte to send (does start bit)
    TWI_WriteByte(WriteData);
}
