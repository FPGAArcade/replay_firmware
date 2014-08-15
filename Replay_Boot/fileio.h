#ifndef FILEIO_H_INCLUDED
#define FILEIO_H_INCLUDED

#include "board.h"
#include "fullfat.h"

#define FILEIO_STAT_INSERTED 0x01 /*disk is inserted*/
#define FILEIO_STAT_WRITABLE 0x02 /*disk is writable*/

#define FILEIO_REQ_ACT         0x08
#define FILEIO_REQ_DIR_TO_ARM  0x04
#define FILEIO_REQ_OK_FM_ARM   0x02  /* read from arm */
#define FILEIO_REQ_OK_TO_ARM   0x01

// note for chan A. Chan B is + 0x40
#define FILEIO_FCH_CMD_STAT_R 0x00
#define FILEIO_FCH_CMD_STAT_W 0x08
#define FILEIO_FCH_CMD_CMD_W  0x10
#define FILEIO_FCH_CMD_CMD_R  0x18
#define FILEIO_FCH_CMD_FIFO_R 0x20
#define FILEIO_FCH_CMD_FIFO_W 0x30

// file buffer MUST be a equal or a multiple of the FPGA buffer size!
#define FILEBUF_SIZE (512*4)    // dynamically allocated file buffer. Use static global instead?
#define FILEIO_MEMBUF_SIZE 512  // max size to transfer in one chunk

#define FCH_BUF_SIZE 512        // dynamically allocated sector buffer. For now.

#define FCH_MAX_NUM 4

typedef struct
{
    uint8_t  status; /*status of floppy*/
    FF_FILE *fSource;
    void    *pDesc;  /*pointer to format specific struct */
    //
    char     name[MAX_DISPLAY_FILENAME];
} fch_t;

typedef fch_t fch_arr_t[2][FCH_MAX_NUM];

uint8_t FileIO_MCh_WaitStat(uint8_t mask, uint8_t wanted);
uint8_t FileIO_MCh_SendBuffer(uint8_t *pBuf, uint16_t buf_tx_size);
uint8_t FileIO_MCh_ReadBuffer(uint8_t *pBuf, uint16_t buf_tx_size);

uint8_t FileIO_MCh_FileToMem(FF_FILE *pFile, uint32_t base, uint32_t size, uint32_t offset);
uint8_t FileIO_MCh_FileToMemVerify(FF_FILE *pFile, uint32_t base, uint32_t size, uint32_t offset);
uint8_t FileIO_MCh_MemToFile(FF_FILE *pFile, uint32_t base, uint32_t size, uint32_t offset);

uint8_t FileIO_MCh_BufToMem(uint8_t *pBuf, uint32_t base, uint32_t size);
uint8_t FileIO_MCh_MemToBuf(uint8_t *pBuf, uint32_t base, uint32_t size);

// ch is 0 for 'A' and 1 for 'B'
uint8_t FCH_CMD(uint8_t ch, uint8_t cmd);
uint8_t FileIO_FCh_GetStat(uint8_t ch);
void    FileIO_FCh_WriteStat(uint8_t ch, uint8_t stat);
uint8_t FileIO_FCh_WaitStat(uint8_t ch, uint8_t mask, uint8_t wanted);

void    FileIO_FCh_Process(uint8_t ch);
void    FileIO_FCh_UpdateDriveStatus(uint8_t ch);

void    FileIO_FCh_Insert(uint8_t ch, uint8_t drive_number, char *path);
void    FileIO_FCh_Eject(uint8_t ch, uint8_t drive_number);

uint8_t FileIO_FCh_GetInserted(uint8_t ch,uint8_t drive_number);
uint8_t FileIO_FCh_GetReadOnly(uint8_t ch,uint8_t drive_number);
char*   FileIO_FCh_GetName(uint8_t ch,uint8_t drive_number);

void    FileIO_FCh_SetDriver(uint8_t ch,uint8_t type);

void    FileIO_FCh_Init(void); // all chans

#endif
