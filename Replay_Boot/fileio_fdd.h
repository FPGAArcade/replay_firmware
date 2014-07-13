#ifndef FILEIO_FDD_H_INCLUDED
#define FILEIO_FDD_H_INCLUDED

#include "board.h"
#include "fullfat.h"

#define FD_MAX_NUM 4

// FILE buffer size = FDD sector size
#define FDD_BUF_SIZE 512

// floppy disk interface defs
#define CMD_RDTRK 0x01
#define CMD_WRTRK 0x02

// floppy status
#define FD_INSERTED 0x01 /*disk is inserted*/
#define FD_WRITABLE 0x02 /*disk is writable*/

#define FD_STAT_REQ_ACK             0x01
#define FD_STAT_TRANS_ACK_OK        0x02
#define FD_STAT_TRANS_ACK_SEEK_ERR  0x12
#define FD_STAT_TRANS_ACK_TRUNC_ERR 0x22
#define FD_STAT_TRANS_ACK_ABORT_ERR 0x42


// constants
#define FD_MAX_TRACKS (83*2)
#define TRACK_SIZE 12668
#define HEADER_SIZE 0x40
#define DATA_SIZE 0x400
//#define SECTOR_SIZE  512
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
    uint8_t  sector_offset; /*sector offset to handle tricky loaders*/
    uint8_t  track; /*current track*/
    uint8_t  track_prev; /*previous track*/
    //
    char     name[MAX_DISPLAY_FILENAME]; /*floppy name*/
} fddTYPE;

//void FDD_AmigaHeader(adfTYPE *drive,  uint8_t track, uint8_t sector, uint16_t dsksync);
//void FDD_Read(adfTYPE *drive, uint32_t offset, uint16_t size);

void FDD_SendSector(uint8_t *pData, uint8_t sector, uint8_t track, uint8_t dsksynch, uint8_t dsksyncl);
void FDD_AmigaRead(fddTYPE *drive);

//uint8_t FDD_FindSync(fddTYPE *drive);
//uint8_t FDD_GetHeader(uint8_t *pTrack, uint8_t *pSector);
//uint8_t FDD_GetData(void);
//void FDD_WriteTrack(fddTYPE *drive);
void FDD_UpdateDriveStatus(void);
void FDD_Handle(void);

void FDD_Insert(uint8_t drive_number, char *path);
void FDD_Eject(uint8_t drive_number);

uint8_t FDD_Inserted(uint8_t drive_number);
char* FDD_GetName(uint8_t drive_number);

void FDD_Init(void);

#endif
