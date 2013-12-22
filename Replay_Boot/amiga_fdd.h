#ifndef AMIGA_FDD_H_INCLUDED
#define AMIGA_FDD_H_INCLUDED

#include "board.h"
#include "fullfat.h"

// FILE buffer size = FDD sector size
#define FDD_BUF_SIZE 512

// floppy disk interface defs
#define CMD_RDTRK 0x01
#define CMD_WRTRK 0x02

// floppy status
#define DSK_INSERTED 0x01 /*disk is inserted*/
#define DSK_WRITABLE 0x10 /*disk is writable*/
#define MAX_DISPLAY_FILENAME 23

// constants
#define MAX_TRACKS (83*2)
#define TRACK_SIZE 12668
#define HEADER_SIZE 0x40
#define DATA_SIZE 0x400
#define SECTOR_SIZE (HEADER_SIZE + DATA_SIZE)
#define SECTOR_COUNT 11
#define LAST_SECTOR (SECTOR_COUNT - 1)
#define GAP_SIZE (TRACK_SIZE - SECTOR_COUNT * SECTOR_SIZE)

typedef struct
{
    FF_FILE *fSource;
    uint8_t status; /*status of floppy*/
    uint8_t tracks; /*number of tracks*/
    uint8_t sector_offset; /*sector offset to handle tricky loaders*/
    uint8_t track; /*current track*/
    uint8_t track_prev; /*previous track*/
    char    name[MAX_DISPLAY_FILENAME]; /*floppy name*/
} adfTYPE;

void FDD_SendSector(uint8_t *pData, uint8_t sector, uint8_t track, uint8_t dsksynch, uint8_t dsksyncl);
void FDD_SendGap(void);
void FDD_ReadTrack(adfTYPE *drive);
uint8_t FDD_FindSync(adfTYPE *drive);
uint8_t FDD_GetHeader(uint8_t *pTrack, uint8_t *pSector);
uint8_t FDD_GetData(void);
void FDD_WriteTrack(adfTYPE *drive);
void FDD_UpdateDriveStatus(void);
void FDD_Handle(uint8_t c1, uint8_t c2);
void FDD_Init(void);


#endif
