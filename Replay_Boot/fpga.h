#ifndef FPGA_H_INCLUDED
#define FPGA_H_INCLUDED

#include "board.h"
#include "fullfat.h"

// file buffer MUST be a equal or a multiple of the FPGA buffer size!
#define FILEBUF_SIZE (512*4)
#define FPGA_MEMBUF_SIZE 512

// Wolfgang: reduced phase by 10 as discussed with Mike (on 16feb2014)
#define kDRAM_PHASE (0x68-10)
#define kDRAM_SEL   0x02

uint8_t FPGA_Default(void);
uint8_t FPGA_Config(FF_FILE *pFile);

uint8_t FPGA_WaitStat(uint8_t mask, uint8_t wanted);
uint8_t FPGA_SendBuffer(uint8_t *pBuf, uint16_t buf_tx_size);
uint8_t FPGA_ReadBuffer(uint8_t *pBuf, uint16_t buf_tx_size);

uint8_t FPGA_FileToMem(FF_FILE *pFile, uint32_t base, uint32_t size, uint32_t offset);
uint8_t FPGA_FileToMemVerify(FF_FILE *pFile, uint32_t base, uint32_t size, uint32_t offset);
uint8_t FPGA_MemToFile(FF_FILE *pFile, uint32_t base, uint32_t size, uint32_t offset);

uint8_t FPGA_BufToMem(uint8_t *pBuf, uint32_t base, uint32_t size);
uint8_t FPGA_MemToBuf(uint8_t *pBuf, uint32_t base, uint32_t size);

void FPGA_ExecMem(uint32_t base, uint16_t len, uint32_t checksum);

uint8_t FPGA_DramTrain(void);
uint8_t FPGA_DramEye(uint8_t mode);
uint8_t FPGA_ProdTest(void);

#endif
