#ifndef FPGA_H_INCLUDED
#define FPGA_H_INCLUDED

#include "board.h"
#include "fullfat.h"

#define kDRAM_PHASE 0x68
#define kDRAM_SEL   0x02

uint8_t FPGA_Config(FF_FILE *pFile);

uint8_t FPGA_WaitStat(uint8_t mask, uint8_t wanted);
uint8_t FPGA_SendBuffer(uint8_t *pBuf, uint16_t buf_tx_size);
uint8_t FPGA_ReadBuffer(uint8_t *pBuf, uint16_t buf_tx_size);

uint8_t FPGA_FileToMem(FF_FILE *pFile, uint32_t base, uint32_t size);
uint8_t FPGA_FileToMemVerify(FF_FILE *pFile, uint32_t base, uint32_t size);

uint8_t FPGA_BufToMem(uint8_t *pBuf, uint32_t base, uint32_t size);
uint8_t FPGA_MemToBuf(uint8_t *pBuf, uint32_t base, uint32_t size);

void FPGA_ExecMem(uint32_t base, uint16_t len, uint32_t checksum);

uint8_t FPGA_DramTrain(void);
uint8_t FPGA_ProdTest(void);

#endif
