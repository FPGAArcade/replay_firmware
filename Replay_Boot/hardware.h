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
#ifndef HARDWARE_H_INCLUDED
#define HARDWARE_H_INCLUDED
#include "board.h"


#define ACTLED_OFF AT91C_BASE_PIOA->PIO_SODR = PIN_ACT_LED;
#define ACTLED_ON  AT91C_BASE_PIOA->PIO_CODR = PIN_ACT_LED;

void Hardware_Init(void);
void IO_DriveLow_OD(uint32_t pin);
void IO_DriveHigh_OD(uint32_t pin);
uint8_t IO_Input_H(uint32_t pin);
uint8_t IO_Input_L(uint32_t pin);

//
void USART_Init(unsigned long baudrate);
void USART_Putc(void* p, char c);
uint8_t USART_GetValid(void);
uint8_t USART_Getc(void);
uint8_t USART_Peekc(void);
int16_t USART_CharAvail(void);
uint16_t USART_PeekBuf(uint8_t* buf, int16_t len);
uint16_t USART_GetBuf(const uint8_t* buf, int16_t len);
void USART_update(void);

//
void SPI_Init(void);
unsigned char rSPI(unsigned char outByte) __attribute__ ((section (".fastrun")));
void SPI_WriteBufferSingle(void* pBuffer, uint32_t length);
void SPI_ReadBufferSingle(void* pBuffer, uint32_t length);

void SPI_Wait4XferEnd(void);
void SPI_EnableCard(void);
void SPI_DisableCard(void) __attribute__ ((section (".fastrun")));
void SPI_EnableFileIO(void);
void SPI_DisableFileIO(void) __attribute__ ((section (".fastrun")));
void SPI_EnableOsd(void);
void SPI_DisableOsd(void);
void SPI_EnableDirect(void);
void SPI_DisableDirect(void);
//
void SSC_Configure_Boot(void);
void SSC_EnableTxRx(void);
void SSC_DisableTxRx(void);
void SSC_Write(uint32_t frame);
void SSC_WaitDMA(void);
void SSC_WriteBufferSingle(void* pBuffer, uint32_t length, uint32_t wait);
//
void TWI_Configure(void);
void TWI_Stop(void);
uint8_t TWI_ReadByte(void);
void TWI_WriteByte(uint8_t byte);
uint8_t TWI_ByteReceived(void);
uint8_t TWI_ByteSent(void);
uint8_t TWI_TransferComplete(void);
void TWI_StartRead(uint8_t DevAddr, uint8_t IntAddrSize, uint16_t IntAddr);
void TWI_StartWrite(uint8_t DevAddr, uint8_t IntAddrSize, uint16_t IntAddr, uint8_t WriteData);

//
void Timer_Init(void);
uint32_t Timer_Get(uint32_t offset);
uint32_t Timer_Check(uint32_t t);
void Timer_Wait(uint32_t time);
//
void DumpBuffer(const uint8_t* pBuffer, uint32_t size);

#endif
