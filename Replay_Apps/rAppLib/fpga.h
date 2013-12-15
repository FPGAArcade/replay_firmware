//
// Support routines for the FPGA
//
// This is for Replay-apps only. As they will be called from a configured
// Replay board, they do not include any INIT and SDCARD stuff.
//
// $Id$
//

#ifndef FPGA_H_INCLUDED
#define FPGA_H_INCLUDED

#include "hardware.h"

uint8_t FPGA_WaitStat(uint8_t mask, uint8_t wanted);
uint8_t FPGA_SendBuffer(uint8_t *pBuf, uint16_t buf_tx_size);
uint8_t FPGA_ReadBuffer(uint8_t *pBuf, uint16_t buf_tx_size);

#endif
