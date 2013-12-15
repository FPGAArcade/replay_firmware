//
// Support routines for the FPGA
//
// This is for Replay-apps only. As they will be called from a configured
// Replay board, they do not include any INIT and SDCARD stuff.
//
// $Id$
//

#include "fpga.h"

//
// DATA IO
//

uint8_t FPGA_WaitStat(uint8_t mask, uint8_t wanted)
{
  uint8_t  stat;
  uint32_t timeout = Timer_Get(100);      // 100 ms timeout
  do {
    SPI_EnableFpga();
    SPI(0x87); // do Read
    stat = SPI(0);
    SPI_DisableFpga();

    if (Timer_Check(timeout)) {
      return (1);
    }
  } while ((stat & mask) != wanted);
  return (0);
}

uint8_t FPGA_SendBuffer(uint8_t *pBuf, uint16_t buf_tx_size)
{
  if (FPGA_WaitStat(0x02, 0)) // !HF
    return (1); // timeout

  SPI_EnableFpga();
  SPI(0xB0);
  SPI_WriteBufferSingle(pBuf, buf_tx_size);
  SPI_DisableFpga();

  return (0);
}

uint8_t FPGA_ReadBuffer(uint8_t *pBuf, uint16_t buf_tx_size)
{
  // we assume read has completed and FIFO contains complete transfer
  SPI_EnableFpga();
  SPI(0xA0); // should check status
  SPI_ReadBufferSingle(pBuf, buf_tx_size);
  SPI_DisableFpga();
  return (0);
}
