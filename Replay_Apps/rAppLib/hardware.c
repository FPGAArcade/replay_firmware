//
// Hardware abstraction layer for Sam7S
//
// This is for Replay-apps only. As they will be called from a configured
// Replay board, they do not include any INIT and SDCARD stuff. 
// Please note, we do not support the CDC peripheral handling here to keep 
// the memory footprint low!
//
// $Id$
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
// IRQ
//
#define IRQ_MASK 0x00000080

static inline unsigned __get_cpsr(void)
{
  unsigned long retval;
  asm volatile (" mrs  %0, cpsr" : "=r" (retval) : /* no inputs */  );
  return retval;
}

static inline void __set_cpsr(unsigned val)
{
  asm volatile (" msr  cpsr, %0" : /* no outputs */ : "r" (val)  );
}

unsigned disableIRQ(void)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();
  __set_cpsr(_cpsr | IRQ_MASK);
  return _cpsr;
}

unsigned enableIRQ(void)
{
  unsigned _cpsr;

  _cpsr = __get_cpsr();
  __set_cpsr(_cpsr & ~IRQ_MASK);
  return _cpsr;
}

//
// USART
//

// for RX, we use a software ring buffer as we are interested in any character
// as soon as it is received.
volatile uint8_t USART_rxbuf[16];
volatile int16_t USART_rxptr, USART_rdptr;

// for TX, we use a hardware buffer triggered to be sent when full or on a CR.
volatile uint8_t USART_txbuf[128];
volatile int16_t USART_txptr, USART_wrptr;

void ISR_USART(void)
{
  uint32_t isr_status = AT91C_BASE_US0->US_CSR;

  // returns if no character
  if (!(isr_status & AT91C_US_RXRDY))
    return;

  USART_rxbuf[USART_rxptr] = AT91C_BASE_US0->US_RHR;
  USART_rxptr = (USART_rxptr+1) & 15;
}

void USART_Putc(void* p, char c)
{ 
  // if both PDC channels are blocked, we still have to wait --> bad luck...
  while (AT91C_BASE_US0->US_TNCR) ;
  // ok, thats the simplest solution - we could still continue in some cases,
  // but I am not sure if it is worth the effort...

  USART_txbuf[USART_wrptr]=c;
  USART_wrptr=(USART_wrptr+1)&127;
  if ((c=='\n')||(!USART_wrptr)) {
    // flush the buffer now (end of line, end of buffer reached or buffer full)
    if ((AT91C_BASE_US0->US_TCR==0)&&(AT91C_BASE_US0->US_TNCR==0)) {
      AT91C_BASE_US0->US_TPR = (uint32_t)&(USART_txbuf[USART_txptr]);
      AT91C_BASE_US0->US_TCR = (128+USART_wrptr-USART_txptr)&127;
      AT91C_BASE_US0->US_PTCR = AT91C_PDC_TXTEN;
      USART_txptr=USART_wrptr;
    } else if (AT91C_BASE_US0->US_TNCR==0) {
      AT91C_BASE_US0->US_TNPR = (uint32_t)&(USART_txbuf[USART_txptr]);
      AT91C_BASE_US0->US_TNCR = (128+USART_wrptr-USART_txptr)&127;      
      USART_txptr=USART_wrptr;
    }
  }
}

uint8_t USART_Getc(void)
{
  uint8_t val;
  if (USART_rxptr!=USART_rdptr) {
    val = USART_rxbuf[USART_rdptr];
    USART_rdptr = (USART_rdptr+1)&15;
  } else {
    val=0;
  }
  return val;
}

uint8_t USART_Peekc(void)
{
  uint8_t val;
  if (USART_rxptr!=USART_rdptr) {
    val = USART_rxbuf[USART_rdptr];
  } else {
    val=0;
  }
  return val;
}

inline int16_t USART_CharAvail(void)
{
  return (16+USART_rxptr-USART_rdptr)&15;
}

int16_t USART_GetBuf(const uint8_t buf[], int16_t len)
{
  uint16_t i;

  if (USART_CharAvail()<len) return 0; // not enough chars to compare

  for (i=0;i<len;++i) {
    if (USART_rxbuf[(USART_rdptr+i)&15]!=buf[i]) break;
  }
  if (i!=len) return 0; // no match

  // got it, remove chars from buffer
  USART_rdptr = (USART_rdptr+len)&15;
  return 1;
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
