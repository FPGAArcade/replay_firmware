#ifndef FILEIO_H_INCLUDED
#define FILEIO_H_INCLUDED

#include "board.h"
#include "fullfat.h"

// file buffer MUST be a equal or a multiple of the FPGA buffer size!
#define FILEBUF_SIZE (512*4)
#define FILEIO_MEMBUF_SIZE 512

uint8_t FileIO_MCh_WaitStat(uint8_t mask, uint8_t wanted);
uint8_t FileIO_MCh_SendBuffer(uint8_t *pBuf, uint16_t buf_tx_size);
uint8_t FileIO_MCh_ReadBuffer(uint8_t *pBuf, uint16_t buf_tx_size);

uint8_t FileIO_MCh_FileToMem(FF_FILE *pFile, uint32_t base, uint32_t size, uint32_t offset);
uint8_t FileIO_MCh_FileToMemVerify(FF_FILE *pFile, uint32_t base, uint32_t size, uint32_t offset);
uint8_t FileIO_MCh_MemToFile(FF_FILE *pFile, uint32_t base, uint32_t size, uint32_t offset);

uint8_t FileIO_MCh_BufToMem(uint8_t *pBuf, uint32_t base, uint32_t size);
uint8_t FileIO_MCh_MemToBuf(uint8_t *pBuf, uint32_t base, uint32_t size);

#endif
