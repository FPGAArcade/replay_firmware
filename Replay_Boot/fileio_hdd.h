#ifndef FILEIO_HDD_H_INCLUDED
#define FILEIO_HDD_H_INCLUDED

#define HD_MAX_NUM 2

// FILE buffer size = HDD sector size
#define HDD_BUF_SIZE 512

#include "board.h"
#include "fullfat.h"
#define CMD_HDRID 0xAA68

#define CMD_IDECMD 0x80 // bits in request word
#define CMD_IDERX  0x20 // bits in request word
#define CMD_IDETX  0x10 // bits in request word

// SPI Commands
#define CMD_IDE_DATA_RD 0x60
#define CMD_IDE_DATA_WR 0x70
#define CMD_IDE_REGS_RD 0x61
#define CMD_IDE_REGS_WR 0x71
#define CMD_IDE_STATUS_WR 0x21

// status bits
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


// floppy status
#define HD_INSERTED 0x01 /*disk is present*/
#define HD_WRITABLE 0x02 /*disk is writable*/
#define MAX_DISPLAY_FILENAME 23

typedef struct
{
    uint8_t  status;
    FF_FILE *fSource;
    uint8_t  name[MAX_DISPLAY_FILENAME]; /*HDD file name*/
    uint16_t cylinders;
    uint16_t heads;
    uint16_t sectors;
    uint32_t size;
    uint16_t sectors_per_block;
} hdfTYPE;

uint8_t HDD_OpenHardfile(char *filename, uint8_t unit);
void HDD_GetHardfileGeometry(hdfTYPE *hdf);
void HDD_BuildHardfileIndex(hdfTYPE *hdf);
uint32_t HDD_chs2lba(uint16_t cylinder, uint8_t head, uint16_t sector, uint8_t unit);

unsigned char HDD_HardFileSeek(hdfTYPE *pHDF, unsigned long lba);
void HDD_IdentifyDevice(uint16_t *pBuffer, uint8_t unit);

void HDD_FileRead(FF_FILE *fSource);
void HDD_FileReadEx(FF_FILE *fSource, uint16_t block_count);

void HDD_WriteTaskFile(unsigned char error, unsigned char sector_count, unsigned char sector_number, unsigned char cylinder_low, unsigned char cylinder_high, unsigned char drive_head);
void HDD_WriteStatus(unsigned char status);
uint8_t HDD_WaitStat(uint8_t mask, uint8_t wanted);

void HDD_ATA_Handle(uint8_t spi_status);
void HDD_Handle(void);
uint8_t HDD_Inserted(uint8_t drive_number);
void HDD_Init(void);

#endif
