/*--------------------------------------------------------------------
 *               Replay Firmware Application (rApp)
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

//
// Hardware abstraction layer for Sam7S
//
// This is for Replay-apps only. As they will be called from a configured
// Replay board, they do not include any INIT and SDCARD stuff. 
// Please note, we do not support the CDC peripheral handling here to keep 
// the memory footprint low!
//

#include "hardware.h"

void IO_DriveLow_OD(uint32_t pin)
{
  // we assume for OD pins the pin is already assigned to PIO and cleared
  AT91C_BASE_PIOA->PIO_OER = pin;
}

void IO_DriveHigh_OD(uint32_t pin)
{
  AT91C_BASE_PIOA->PIO_ODR = pin;
}

uint8_t IO_Input_H(uint32_t pin)  // returns true if pin high
{
  volatile uint32_t read;
  read = AT91C_BASE_PIOA->PIO_PDSR;
  if (!(read & pin))
    return FALSE;
  else
    return TRUE;
}

uint8_t IO_Input_L(uint32_t pin)  // returns true if pin low
{
  volatile uint32_t read;
  read = AT91C_BASE_PIOA->PIO_PDSR;
  if (!(read & pin))
    return TRUE;
  else
    return FALSE;
}

//
// USART
//

void USART_ReInit(unsigned long baudrate)
{
  // Configure PA5 and PA6 for USART0 use
  AT91C_BASE_PIOA->PIO_PDR = AT91C_PA5_RXD0 | AT91C_PA6_TXD0;
  // disable pullup on output
  AT91C_BASE_PIOA->PIO_PPUDR = PIN_TXD;

  // Enable the peripheral clock in the PMC
  AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_US0;

  // Reset and disable receiver & transmitter
  AT91C_BASE_US0->US_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RXDIS | AT91C_US_TXDIS;

  // Configure USART0 mode
  AT91C_BASE_US0->US_MR = AT91C_US_USMODE_NORMAL | AT91C_US_CLKS_CLOCK | AT91C_US_CHRL_8_BITS | AT91C_US_PAR_NONE | AT91C_US_NBSTOP_1_BIT | AT91C_US_CHMODE_NORMAL;

  // Configure the USART0 @115200 bauds
  AT91C_BASE_US0->US_BRGR = BOARD_MCK / 16 / baudrate;

  // Enable receiver & transmitter
  AT91C_BASE_US0->US_CR = AT91C_US_RXEN | AT91C_US_TXEN;
}

void USART_Putc(void* p, char c)
{
  while (!(AT91C_BASE_US0->US_CSR & AT91C_US_TXEMPTY));
  AT91C_BASE_US0->US_THR = c;
}


uint8_t USART_Getc(void)
{
  // returns 0 if no character
  if (!(AT91C_BASE_US0->US_CSR & AT91C_US_RXRDY))
    return 0;
  else {
    AT91C_BASE_US0->US_CR = AT91C_US_RXEN | AT91C_US_TXEN | AT91C_US_RSTSTA;
    return (AT91C_BASE_US0->US_RHR & 0xFF);
  }
}

//
// SPI
//

unsigned char SPI(unsigned char outByte)
{
  volatile uint32_t t = AT91C_BASE_SPI->SPI_RDR;  // warning, but is a must!
  while (!(AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TDRE));
  AT91C_BASE_SPI->SPI_TDR = outByte;
  while (!(AT91C_BASE_SPI->SPI_SR & AT91C_SPI_RDRF));
  return((unsigned char)AT91C_BASE_SPI->SPI_RDR);
}

void SPI_WriteBufferSingle(void *pBuffer, uint32_t length)
{
  // single bank only, assume idle on entry
  AT91C_BASE_SPI->SPI_TPR  = (uint32_t) pBuffer;
  AT91C_BASE_SPI->SPI_TCR  = length;

  AT91C_BASE_SPI->SPI_PTCR = AT91C_PDC_TXTEN;
  //while ((AT91C_BASE_SPI->SPI_SR & AT91C_SPI_ENDTX ) != AT91C_SPI_ENDTX) {}; // no timeout

  uint32_t timeout = Timer_Get(100);      // 100 ms timeout
  while ((AT91C_BASE_SPI->SPI_SR & AT91C_SPI_ENDTX ) != AT91C_SPI_ENDTX) {
    if (Timer_Check(timeout)) {
      AT91C_BASE_SPI->SPI_PTCR = AT91C_PDC_TXTDIS | AT91C_PDC_RXTDIS;
      break;
    }
  };
}

void SPI_ReadBufferSingle(void *pBuffer, uint32_t length)
{
  // we do not care what we send out (current buffer contents), the FPGA will ignore

  AT91C_BASE_SPI->SPI_TPR  = (uint32_t) pBuffer;
  AT91C_BASE_SPI->SPI_TCR  = length;

  AT91C_BASE_SPI->SPI_RPR  = (uint32_t) pBuffer;
  AT91C_BASE_SPI->SPI_RCR  = length;

  AT91C_BASE_SPI->SPI_PTCR = AT91C_PDC_TXTEN | AT91C_PDC_RXTEN;

  uint32_t timeout = Timer_Get(100);      // 100 ms timeout
  while ((AT91C_BASE_SPI->SPI_SR & (AT91C_SPI_ENDTX | AT91C_SPI_ENDRX)) != (AT91C_SPI_ENDTX | AT91C_SPI_ENDRX) ) {
    if (Timer_Check(timeout)) {
      AT91C_BASE_SPI->SPI_PTCR = AT91C_PDC_TXTDIS | AT91C_PDC_RXTDIS;
      break;
    }
  };
}

void SPI_Wait4XferEnd(void)
{
  while (!(AT91C_BASE_SPI->SPI_SR & AT91C_SPI_TXEMPTY));
}

void SPI_EnableFpga(void)
{
  AT91C_BASE_PIOA->PIO_CODR = PIN_FPGA_CTRL0;
}

void SPI_DisableFpga(void)
{
  SPI_Wait4XferEnd();
  AT91C_BASE_PIOA->PIO_SODR = PIN_FPGA_CTRL0;
}

void SPI_EnableOsd(void)
{
  AT91C_BASE_PIOA->PIO_CODR = PIN_FPGA_CTRL1;
}

void SPI_DisableOsd(void)
{
  SPI_Wait4XferEnd();
  AT91C_BASE_PIOA->PIO_SODR = PIN_FPGA_CTRL1;
}

//
// SSC
//

void SSC_EnableTxRx(void)
{
  AT91C_BASE_SSC->SSC_CR = AT91C_SSC_TXEN |  AT91C_SSC_RXEN;
}

void SSC_DisableTxRx(void)
{
  AT91C_BASE_SSC->SSC_CR = AT91C_SSC_TXDIS | AT91C_SSC_RXDIS;
}

void SSC_Write(uint32_t frame)
{
  while ((AT91C_BASE_SSC->SSC_SR & AT91C_SSC_TXRDY) == 0);
  AT91C_BASE_SSC->SSC_THR = frame;
}

void SSC_WriteBufferSingle(void *pBuffer, uint32_t length)
{
  // single bank only, assume idle on entry
  AT91C_BASE_SSC->SSC_TPR = (unsigned int) pBuffer;
  AT91C_BASE_SSC->SSC_TCR = length;
  AT91C_BASE_SSC->SSC_PTCR = AT91C_PDC_TXTEN;
  // wait for buffer to be sent
  while ((AT91C_BASE_SSC->SSC_SR & AT91C_SSC_ENDTX ) != AT91C_SSC_ENDTX) {}; // no timeout

}

//
// TWI
//

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

//
// Timer
//

uint32_t Timer_Get(uint32_t offset)
{
  // note max timer is 4096mS with this setting
  uint32_t systimer = (AT91C_BASE_PITC->PITC_PIIR & AT91C_PITC_PICNT);
  systimer += offset<<20;
  return (systimer); //valid bits [31:20]
}

uint32_t Timer_Check(uint32_t time)
{
  uint32_t systimer = (AT91C_BASE_PITC->PITC_PIIR & AT91C_PITC_PICNT);
  /*calculate difference*/
  time -= systimer;
  /*check if <t> has passed*/
  return(time>(1<<31));
}

void Timer_Wait(uint32_t time)
{
  time =Timer_Get(time);
  while (!Timer_Check(time));
}
