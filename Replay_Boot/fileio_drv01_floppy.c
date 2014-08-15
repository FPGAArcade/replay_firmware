
#include "fileio_drv.h"
#include "hardware.h"
#include "messaging.h"

const uint8_t DRV01_DEBUG = 0;

#define ADF_MAX_TRACKS (83*2)

#define TRACK_SIZE 12668
#define HEADER_SIZE 0x40
#define DATA_SIZE 0x400
#define SECTOR_SIZE (HEADER_SIZE + DATA_SIZE)
#define SECTOR_COUNT 11
#define LAST_SECTOR (SECTOR_COUNT - 1)
#define GAP_SIZE (TRACK_SIZE - SECTOR_COUNT * SECTOR_SIZE)

#define DRV01_STAT_REQ_ACK             0x01
#define DRV01_STAT_TRANS_ACK_OK        0x02
#define DRV01_STAT_TRANS_ACK_SEEK_ERR  0x12
#define DRV01_STAT_TRANS_ACK_TRUNC_ERR 0x22
#define DRV01_STAT_TRANS_ACK_ABORT_ERR 0x42

typedef enum {
    XXX, // unsupported
    ADF,
    SCP
} drv01_format_t;

typedef struct
{
    drv01_format_t  format;
    //
    uint32_t file_size;
    uint16_t tracks;
    uint8_t  cur_sector;
} drv01_desc_t;

void FileIO_Drv01_SendAmigaSector(uint8_t *pData, uint8_t sector, uint8_t track, uint8_t dsksynch, uint8_t dsksyncl)
{
  //DumpBuffer(pData, 64);

  uint8_t checksum[4];
  uint16_t i;
  uint8_t x;
  uint8_t *p;

  // preamble
  SPI(0xAA);
  SPI(0xAA);
  SPI(0xAA);
  SPI(0xAA);

  // synchronization
  SPI(dsksynch);
  SPI(dsksyncl);
  SPI(dsksynch);
  SPI(dsksyncl);

  // odd bits of header
  x = 0x55;
  checksum[0] = x;
  SPI(x);
  x = (track >> 1) & 0x55;
  checksum[1] = x;
  SPI(x);
  x = (sector >> 1) & 0x55;
  checksum[2] = x;
  SPI(x);
  // subtraction has higher prio, added brackets accordingly!
  x = ((11 - sector) >> 1) & 0x55;
  checksum[3] = x;
  SPI(x);

  // even bits of header
  x = 0x55;
  checksum[0] ^= x;
  SPI(x);
  x = track & 0x55;
  checksum[1] ^= x;
  SPI(x);
  x = sector & 0x55;
  checksum[2] ^= x;
  SPI(x);
  // subtraction has higher prio, added brackets accordingly!
  x = (11 - sector) & 0x55;
  checksum[3] ^= x;
  SPI(x);

  // sector label and reserved area (changes nothing to checksum)
  i = 0x20;
  while (i--)
    SPI(0xAA);

  // send header checksum
  SPI(0xAA);
  SPI(0xAA);
  SPI(0xAA);
  SPI(0xAA);
  SPI(checksum[0] | 0xAA);
  SPI(checksum[1] | 0xAA);
  SPI(checksum[2] | 0xAA);
  SPI(checksum[3] | 0xAA);

  // calculate data checksum
  checksum[0] = 0;
  checksum[1] = 0;
  checksum[2] = 0;
  checksum[3] = 0;

  p = pData;
  i = DATA_SIZE / 2 / 4;
  while (i--) {
    x = *p++;
    checksum[0] ^= x ^ x >> 1;
    x = *p++;
    checksum[1] ^= x ^ x >> 1;
    x = *p++;
    checksum[2] ^= x ^ x >> 1;
    x = *p++;
    checksum[3] ^= x ^ x >> 1;
  }

  // send data checksum
  SPI(0xAA);
  SPI(0xAA);
  SPI(0xAA);
  SPI(0xAA);
  SPI(checksum[0] | 0xAA);
  SPI(checksum[1] | 0xAA);
  SPI(checksum[2] | 0xAA);
  SPI(checksum[3] | 0xAA);

  // odd bits of data field
  i = DATA_SIZE / 2;
  p = pData;
  while (i--)
    SPI(*p++ >> 1 | 0xAA);

  // even bits of data field
  i = DATA_SIZE / 2;
  p = pData;
  while (i--)
    SPI(*p++ | 0xAA);
}

void FileIO_Drv01_Process(uint8_t ch, fch_arr_t *pHandle, uint8_t status) // amiga
{
  // add format check...

  // file buffer
  uint8_t  fbuf[512];

  uint8_t  dir          = 0;
  uint8_t  drive_number = 0;
  uint8_t  sector  = 0;
  uint8_t  track   = 0;
  uint16_t dsksync = 0;
  uint32_t offset  = 0;
  uint32_t i       = 0;

  uint8_t  goes = FILEIO_PROCESS_LIMIT; // limit number of requests.

  do {
    dir          = (status >> 2) & 0x01; // high is write
    drive_number = (status >> 4) & 0x03;

    // validate request
    if (!FileIO_FCh_GetInserted(ch, drive_number)) {
      DEBUG(1,"Drv01:Process Ch:%d Drive:%d not mounted", ch, drive_number);
      FileIO_FCh_WriteStat(ch, DRV01_STAT_REQ_ACK); // ack
      FileIO_FCh_WriteStat(ch, DRV01_STAT_TRANS_ACK_ABORT_ERR); // err
      return;
    }

    SPI_EnableFileIO();
    SPI(FCH_CMD(ch,FILEIO_FCH_CMD_CMD_R | 0x0));
    SPI(0x00); // dummy

               SPI(0);  // spare
    track    = SPI(0);
    dsksync  = SPI(0) << 8;
    dsksync |= SPI(0);
    SPI_DisableFileIO();

    FileIO_FCh_WriteStat(ch, DRV01_STAT_REQ_ACK); // ack

    fch_t* drive = (fch_t*) &pHandle[ch][drive_number]; // get base
    drv01_desc_t* pDesc = drive->pDesc;

    if (track >= pDesc->tracks) {
      DEBUG(0,"Illegal track %d read!", track); // no warning, happens quite a bit
      track = pDesc->tracks - 1;
    }

    sector = pDesc->cur_sector;
    if (pDesc->cur_sector == LAST_SECTOR)
      pDesc->cur_sector=0;
    else
      pDesc->cur_sector++;


    // workaround for Copy Lock in Wiz'n'Liz and North&South (might brake other games)
    if (dsksync == 0x0000 || dsksync == 0x8914 || dsksync == 0xA144)
      dsksync = 0x4489;

    // North&South: $A144
    // Wiz'n'Liz (Copy Lock): $8914
    // Prince of Persia: $4891
    // Commando: $A245

    if (DRV01_DEBUG)
      DEBUG(1,"Drv01:Process Ch%d Drive:%02X Dsksync:%04X Track:%02X Sector:%01X",ch,drive_number,dsksync,track,sector);

    // sector size hard coded as 512 bytes
    offset  = (512*11) * track;
    offset += (sector<<9);

    if (FF_Seek(drive->fSource, offset, FF_SEEK_SET)) {
      DEBUG(1,"Drv01:seek error");
      FileIO_FCh_WriteStat(ch, DRV01_STAT_TRANS_ACK_SEEK_ERR); // err
      return;
    }
    FF_Read(drive->fSource, 512, 1, fbuf);

    // send sector
    SPI_EnableFileIO();
    SPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_W));
    FileIO_Drv01_SendAmigaSector(fbuf, sector, track, (uint8_t)(dsksync >> 8), (uint8_t)dsksync);
    SPI_DisableFileIO();

    if (sector == LAST_SECTOR)
    {
      // send gap
      SPI_EnableFileIO();
      SPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_W | 0x1));
      SPI(0xAA);
      SPI(0xAA);
      SPI_DisableFileIO();

      SPI_EnableFileIO();
      SPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_W));
      i = GAP_SIZE-2;
      while (i--)
        SPI(0xAA);
      SPI_DisableFileIO();
    }
    // signal transfer done
    FileIO_FCh_WriteStat(ch, DRV01_STAT_TRANS_ACK_OK); // ok
    // any more to do?
    status = FileIO_FCh_GetStat(ch);
    goes --;
  } while ((status & FILEIO_REQ_ACT) && goes);

}

uint8_t FileIO_Drv01_InsertInit(uint8_t ch, uint8_t drive_number, fch_t* pDrive, char *ext)
{
  DEBUG(1,"Drv01:InsertInit");

  pDrive->pDesc = calloc(1, sizeof(drv01_desc_t)); // 0 everything
  drv01_desc_t* pDesc = pDrive->pDesc;

  pDesc->format = (drv01_format_t)XXX;
  if (strnicmp(ext, "ADF",3) == 0) {

    pDesc->format = (drv01_format_t)ADF;
    pDesc->file_size =  pDrive->fSource->Filesize;
    uint16_t tracks = pDesc->file_size / (512*11);
    if (tracks > ADF_MAX_TRACKS) {
      MSG_warning("UNSUPPORTED ADF SIZE!!! Too many tracks: %d", tracks);
      tracks = ADF_MAX_TRACKS;
    }
    pDesc->tracks = tracks;

  } else {
    WARNING("Drv01:Unsupported format.");
    return (1);
  }

  // send mode, 0 for ADF, 1 for SCP
  SPI_EnableFileIO();
  SPI(FCH_CMD(ch,FILEIO_FCH_CMD_CMD_W));
  SPI(0x00);
  SPI_DisableFileIO();

  //
  DEBUG(1,"Drv01:Size   : %lu (%lu kB)", pDesc->file_size, pDesc->file_size);
  DEBUG(1,"Drv01:Tracks : %u", pDesc->tracks);

  return (0);
}

