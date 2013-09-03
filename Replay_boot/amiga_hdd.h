#ifndef AMIGA_HDD_H_INCLUDED
#define AMIGA_HDD_H_INCLUDED

#include "board.h"
#include "fullfat.h"
#define CMD_HDRID 0xAA68

#define CMD_IDECMD 0x04
#define CMD_IDEDAT 0x08

#define CMD_IDE_REGS_RD 0x80
#define CMD_IDE_REGS_WR 0x90
#define CMD_IDE_DATA_WR 0xA0
#define CMD_IDE_DATA_RD 0xB0
#define CMD_IDE_STATUS_WR 0xF0

#define IDE_STATUS_END 0x80
#define IDE_STATUS_IRQ 0x10
#define IDE_STATUS_RDY 0x08
#define IDE_STATUS_REQ 0x04
#define IDE_STATUS_ERR 0x01

#define ACMD_RECALIBRATE 0x10
#define ACMD_EXECUTE_DEVICE_DIAGNOSTIC 0x90
#define ACMD_IDENTIFY_DEVICE 0xEC
#define ACMD_INITIALIZE_DEVICE_PARAMETERS 0x91
#define ACMD_READ_SECTORS 0x20
#define ACMD_WRITE_SECTORS 0x30
#define ACMD_READ_MULTIPLE 0xC4
#define ACMD_WRITE_MULTIPLE 0xC5
#define ACMD_SET_MULTIPLE_MODE 0xC6


#define MAX_DISPLAY_FILENAME 23

typedef struct
{
    FF_FILE *fSource;
    uint16_t cylinders;
    uint16_t heads;
    uint16_t sectors;
    uint16_t sectors_per_block;
    //uint32_t index[1024];
    //uint32_t index_size;
    char     name[MAX_DISPLAY_FILENAME]; /*floppy name*/
} hdfTYPE;

/*void IdentifyDevice(uint16_t *pBuffer, uint8_t unit);
unsigned long chs2lba(unsigned short cylinder, unsigned char head, unsigned short sector, unsigned char unit);
void WriteTaskFile(unsigned char error, unsigned char sector_count, unsigned char sector_number, unsigned char cylinder_low, unsigned char cylinder_high, unsigned char drive_head);
void WriteStatus(unsigned char status);
void HandleHDD(unsigned char c1, unsigned char c2);
void GetHardfileGeometry(hdfTYPE *hdf);
void BuildHardfileIndex(hdfTYPE *hdf);
unsigned char HardFileSeek(hdfTYPE *hdf, unsigned long lba);
unsigned char OpenHardfile(unsigned char unit);
*/
#endif
