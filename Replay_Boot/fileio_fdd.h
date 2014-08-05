#ifndef FILEIO_FDD_H_INCLUDED
#define FILEIO_FDD_H_INCLUDED

#include "board.h"
#include "fullfat.h"

#define CHA_MAX_NUM 4

// FILE buffer size = FDD sector size
#define FDD_BUF_SIZE 512

// floppy disk interface defs
//#define CMD_RDTRK 0x01
//#define CMD_WRTRK 0x02

// floppy status
#define FD_INSERTED 0x01 /*disk is inserted*/
#define FD_WRITABLE 0x02 /*disk is writable*/


#define FD_STAT_REQ_ACK             0x01
#define FD_STAT_TRANS_ACK_OK        0x02
#define FD_STAT_TRANS_ACK_SEEK_ERR  0x12
#define FD_STAT_TRANS_ACK_TRUNC_ERR 0x22
#define FD_STAT_TRANS_ACK_ABORT_ERR 0x42

// pull these out into FILEIO
#define FILEIO_FD_REQ_ACT         0x08
#define FILEIO_FD_REQ_DIR_TO_ARM  0x04
#define FILEIO_FD_REQ_OK_FM_ARM   0x02  /* read from arm */
#define FILEIO_FD_REQ_OK_TO_ARM   0x01

#define FILEIO_FD_STAT_R 0x00
#define FILEIO_FD_STAT_W 0x08
#define FILEIO_FD_CMD_W  0x10
#define FILEIO_FD_CMD_R  0x18
#define FILEIO_FD_FIFO_R 0x20
#define FILEIO_FD_FIFO_W 0x30

// constants
#define FD_MAX_TRACKS (83*2)
#define TRACK_SIZE 12668
#define HEADER_SIZE 0x40
#define DATA_SIZE 0x400
#define SECTOR_SIZE (HEADER_SIZE + DATA_SIZE)
#define SECTOR_COUNT 11
#define LAST_SECTOR (SECTOR_COUNT - 1)
#define GAP_SIZE (TRACK_SIZE - SECTOR_COUNT * SECTOR_SIZE)

typedef struct
{
    uint8_t  status; /*status of floppy*/
    FF_FILE *fSource;
    // adf
    uint16_t tracks; /*number of tracks*/
    uint8_t  cur_sector;
    uint8_t  track; /*current track*/
    uint8_t  track_prev; /*previous track*/
    //
    char     name[MAX_DISPLAY_FILENAME]; /*floppy name*/
} fddTYPE;


uint8_t FDD_FileIO_GetStat(void);
void    FDD_FileIO_WriteStat(uint8_t stat);
uint8_t FDD_WaitStat(uint8_t mask, uint8_t wanted);

void FDD_SendAmigaSector(uint8_t *pData, uint8_t sector, uint8_t track, uint8_t dsksynch, uint8_t dsksyncl);

void    FDD_Handle_0x00(uint8_t status);
void    FDD_Handle_0x01(uint8_t status);
void    FDD_Handle(void);

void    FDD_UpdateDriveStatus(void);
void    FDD_InsertParse_Generic(fddTYPE *drive);
void    FDD_InsertParse_ADF(fddTYPE *drive);
void    FDD_InsertInit_0x00(uint8_t drive_number);
void    FDD_InsertInit_0x01(uint8_t drive_number);

void    FDD_Insert(uint8_t drive_number, char *path);
void    FDD_Eject(uint8_t drive_number);

uint8_t FDD_Inserted(uint8_t drive_number);
char*   FDD_GetName(uint8_t drive_number);

void    FDD_SetDriver(uint8_t type);
void    FDD_Init(void);

#endif
