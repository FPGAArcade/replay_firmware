
#include "fileio_drv.h"
#include "hardware.h"
#include "messaging.h"

const uint8_t DRV08_DEBUG = 0;
/*#define DRV08_PARAM_DEBUG 1;*/

#define DRV08_BUF_SIZE 512 // THIS must be = or > than BLK SIZE
#define DRV08_BLK_SIZE 512
#define DRV08_MAX_READ_BURST 7 // number of sectors FIFO can hold from AF de-asserted (could have <1 sector in FIFO still)
/*#define DRV08_MAX_READ_BURST 1 // number of sectors FIFO can hold from AF de-asserted (could have <1 sector in FIFO still)*/

// status bits
#define DRV08_STATUS_END 0x80
#define DRV08_STATUS_IRQ 0x10
#define DRV08_STATUS_RDY 0x08
#define DRV08_STATUS_REQ 0x04
#define DRV08_STATUS_ERR 0x01

// error bits
#define DRV08_ERROR_BBK  0x80 // bad block is detected
#define DRV08_ERROR_UNC  0x40 // uncorrectable error
#define DRV08_ERROR_IDNF 0x10 // requested sector ID is in error/not found
#define DRV08_ERROR_ABRT 0x04 // Command has been aborted due to status (not ready / fault / invalid command)
#define DRV08_ERROR_AMNF 0x01 // General error

// commands
#define DRV08_CMD_RECALIBRATE                  0x10  //1X
#define DRV08_CMD_READ_SECTORS                 0x20
#define DRV08_CMD_WRITE_SECTORS                0x30
#define DRV08_CMD_READ_VERIFY                  0x40
#define DRV08_CMD_EXECUTE_DEVICE_DIAGNOSTIC    0x90
#define DRV08_CMD_INITIALIZE_DEVICE_PARAMETERS 0x91
#define DRV08_CMD_READ_MULTIPLE                0xC4
#define DRV08_CMD_WRITE_MULTIPLE               0xC5
#define DRV08_CMD_SET_MULTIPLE_MODE            0xC6
#define DRV08_CMD_IDENTIFY_DEVICE              0xEC


typedef enum {
    XXX, // unsupported
    HDF
} drv08_format_t;

typedef struct
{
  drv08_format_t format;

  uint32_t file_size;
  uint16_t cylinders;
  uint16_t heads;
  uint16_t sectors;
  uint16_t sectors_per_block;
} drv08_desc_t;

void Drv08_SwapBytes(uint8_t *ptr, uint32_t len)
{
  uint8_t x;
  len >>= 1;
  while (len--) {
    x = ptr[0];
    ptr[0] = ptr[1];
    ptr[1] = x;
    ptr += 2;
  }
}

uint32_t Drv08_chs2lba(drv08_desc_t *pDesc, uint16_t cylinder, uint8_t head, uint16_t sector)
{
  return(cylinder * pDesc->heads + head) * pDesc->sectors + sector - 1;
}

void Drv08_IdentifyDevice(fch_t *pDrive, drv08_desc_t *pDesc, uint16_t *pBuffer) // buffer needs to be >=512 bytes
{ // builds Identify Device struct

  char *p;
  uint32_t total_sectors = pDesc->cylinders * pDesc->heads * pDesc->sectors;

  memset(pBuffer, 0, 512);

  pBuffer[0] = 1 << 6; // hard disk
  pBuffer[1] = pDesc->cylinders; // cyl count
  pBuffer[3] = pDesc->heads;     // head count
  pBuffer[6] = pDesc->sectors;   // sectors per track
  memcpy((char*)&pBuffer[10], "FPGA ARCADE ATA DRV ", 20); // serial number
  memcpy((char*)&pBuffer[23], ".100    ", 8); // firmware version
  //
  p = (char*)&pBuffer[27];
  memcpy(p, "Replay                                  ", 40); // model name - byte swapped
  p += 8;
  strncpy(p,(char *)pDrive->name,16);      // file name as part of model name - byte swapped
  Drv08_SwapBytes((uint8_t*)&pBuffer[27], 40);
  //
  pBuffer[47] = 0x8010; //maximum sectors per block in Read/Write Multiple command (16 in this case) 8K bytes (4K * 16)
  pBuffer[53] = 1;
  pBuffer[54] = pDesc->cylinders;
  pBuffer[55] = pDesc->heads;
  pBuffer[56] = pDesc->sectors;
  pBuffer[57] = (uint16_t) total_sectors;
  pBuffer[58] = (uint16_t)(total_sectors >> 16);
  pBuffer[60] = (uint16_t) total_sectors;
  pBuffer[61] = (uint16_t)(total_sectors >> 16);
}

  /*--$DA2000 Data               < tf0*/
  /*--$DA2004 Error / Feature    < tf1*/
  /*--$DA2008 SectorCount        < tf2*/
  /*--$DA200C SectorNumber       < tf3*/
  /*--$DA2010 CylinderLow        < tf4*/
  /*--$DA2014 CylinderHigh       < tf5*/
  /*--$DA2018 Device/Head        < tf6*/
  /*--$DA201C Status / Command   < tf7*/
inline void Drv08_WriteTaskFile(uint8_t ch, uint8_t error, uint8_t sector_count, uint8_t sector_number, uint8_t cylinder_low, uint8_t cylinder_high, uint8_t drive_head)
{
    SPI_EnableFileIO();
    SPI(FCH_CMD(ch, FILEIO_FCH_CMD_CMD_W)); // write task file registers command
    SPI(0x00);
    SPI(error); // error
    SPI(sector_count); // sector count
    SPI(sector_number); //sector number
    SPI(cylinder_low); // cylinder low
    SPI(cylinder_high); // cylinder high
    SPI(drive_head); // drive/head
    SPI_DisableFileIO();
}


FF_ERROR Drv08_HardFileSeek(fch_t *pDrive, uint32_t lba)
{
  return FF_Seek(pDrive->fSource, lba*512, FF_SEEK_SET);  // TO DO, REPLACE with <<
}

void Drv08_FileReadSend(uint8_t ch, fch_t *pDrive, uint8_t *pBuffer)
{

  uint32_t bytes_r = FF_Read(pDrive->fSource, DRV08_BLK_SIZE, 1, pBuffer);

  /*DumpBuffer(pBuffer,DRV08_BLK_SIZE);*/

  // add error handling
  if (bytes_r != DRV08_BLK_SIZE) {
    DEBUG(1,"Drv08:!! Read Fail!!");
  }

  SPI_EnableFileIO();
  SPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_W));
  SPI_WriteBufferSingle(pBuffer, DRV08_BLK_SIZE);
  SPI_DisableFileIO();
}

void Drv08_FileReadSendDirect(uint8_t ch, fch_t *pDrive, uint8_t sector_count)
{
  // on entry assumes file is in correct position
  // no flow control check, FPGA must be able to sync entire transfer.
  SPI_EnableFileIO();
  SPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_W));
  SPI_EnableDirect();
  SPI_DisableFileIO();

  uint32_t bytes_r = FF_ReadDirect(pDrive->fSource, DRV08_BLK_SIZE, sector_count);

  SPI_DisableDirect();

  // add error handling
  if (bytes_r != (DRV08_BLK_SIZE * sector_count)) {
    DEBUG(1,"Drv08:!! Direct Read Fail!!");
  }

}

void Drv08_FileWrite(uint8_t ch, fch_t *pDrive, uint8_t *pBuffer)
{
  // fix error handling
  uint32_t bytes_w = FF_Write(pDrive->fSource, DRV08_BLK_SIZE, 1, pBuffer);
  if (bytes_w != DRV08_BLK_SIZE) {
    DEBUG(1,"Drv08:!! Write Fail!!");
  }
}

inline void Drv08_GetParams(uint8_t tfr[8], drv08_desc_t *pDesc,  // inputs
                            uint16_t *sector, uint16_t *cylinder, uint8_t *head, uint16_t *sector_count, uint32_t *lba, uint8_t *lba_mode)
{
  // given the register file, extract address and sector count.
  // then convert this into an LBA.

  *sector_count = tfr[2];
  *sector       = tfr[3]; // note, 8>16 bit
  *cylinder     = tfr[4] | tfr[5]<<8;
  *head         = tfr[6] & 0x0F;
  *lba_mode     = (tfr[6] & 0x40);

  if (*sector_count == 0)
    *sector_count = 0x100;

  if (*lba_mode)
    *lba = (*head)<<24 | (*cylinder)<<8 | (*sector);
  else
    *lba = ( ((*cylinder) * pDesc->heads + (*head)) * pDesc->sectors) + (*sector) - 1;

  if (DRV08_DEBUG) {
    #ifdef DRV08_PARAM_DEBUG
      DEBUG(1,"sector   : %d", *sector);
      DEBUG(1,"cylinder : %d", *cylinder);
      DEBUG(1,"head     : %d", *head);
      DEBUG(1,"lba      : %ld", *lba);
      DEBUG(1,"count    : %d", *sector_count);
    #endif
  }
}

inline void Drv08_UpdateParams(uint8_t ch, uint8_t tfr[8], uint16_t sector, uint16_t cylinder, uint8_t head, uint32_t lba, uint8_t lba_mode)
{
  // note, we can only update the taskfile when BUSY
  // all params inputs
  uint8_t drive = tfr[6] & 0xF0;

  SPI_EnableFileIO();
  SPI(FCH_CMD(ch, FILEIO_FCH_CMD_CMD_W | 0x03)); // write task file registers command

  if (lba_mode) {
    SPI((uint8_t) (lba      ));
    SPI((uint8_t) (lba >>  8));
    SPI((uint8_t) (lba >> 16));
    SPI((uint8_t) (drive | ( (lba >> 24) & 0x0F)));
  } else {
    SPI((uint8_t)  sector);
    SPI((uint8_t)  cylinder);
    SPI((uint8_t) (cylinder >> 8));
    SPI((uint8_t) (drive | (head & 0x0F)));
  }
  SPI_DisableFileIO();
}

inline void Drv08_IncParams(drv08_desc_t *pDesc,  // inputs
                            uint16_t *sector, uint16_t *cylinder, uint8_t *head, uint32_t *lba)
{
  // increment address.
  // At command completion, the Command Block Registers contain the cylinder, head and sector number of the last sector read
  if (*sector == pDesc->sectors) {
    *sector = 1;
    (*head)++;
    if (*head == pDesc->heads) {
      *head =0;
      (*cylinder)++;
    }
  } else {
    (*sector)++;
  }
  // and the LBA
  (*lba)++;
}
//
//
//
void Drv08_ATA_Handle(uint8_t ch, fch_t handle[2][FCH_MAX_NUM])
{

    // file buffer
    // to do, check how expensive this allocation is every time.
    // static buffer?

    uint16_t  fbuf16[DRV08_BUF_SIZE/2]; // to get 16 bit alignment for id.
    uint8_t  *fbuf = (uint8_t*) fbuf16; // reuse the buffer

    uint8_t  tfr[8];
    uint16_t i;
    uint8_t  unit         = 0;
    uint16_t sector_count = 0;
    uint16_t block_count  = 0;

    uint16_t sector   = 0;
    uint16_t cylinder = 0;
    uint8_t  head     = 0;
    uint32_t lba      = 0;
    uint8_t  lba_mode = 0;
    uint8_t  first    = 1;

    // read task file
    SPI_EnableFileIO();
    SPI(FCH_CMD(ch,FILEIO_FCH_CMD_CMD_R | 0x0));
    SPI(0x00); // dummy
    for (i = 0; i < 8; i++) {
      tfr[i] = SPI(0);
    }
    SPI_DisableFileIO();

    //
    if (DRV08_DEBUG)
      DEBUG(1,"Drv08:CMD %02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X",
            tfr[0],tfr[1],tfr[2],tfr[3],tfr[4],tfr[5],tfr[6],tfr[7]);

    unit = tfr[6] & 0x10 ? 1 : 0; // master/slave selection
    // 0 = master, 1 = slave

    fch_t* pDrive = &handle[ch][unit]; // get base
    drv08_desc_t* pDesc = pDrive->pDesc;

    if (!FileIO_FCh_GetInserted(ch, unit)) {
      DEBUG(1,"Drv08:Process Ch:%d Unit:%d not mounted", ch, unit);
      Drv08_WriteTaskFile (ch, DRV08_ERROR_ABRT, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
      FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ | DRV08_STATUS_ERR);
      return;
    }

    //
    if        ((tfr[7] & 0xF0) == DRV08_CMD_RECALIBRATE) { // Recalibrate 0x10-0x1F (class 3 command: no data)
      //
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Recalibrate");
      //
      Drv08_WriteTaskFile (ch, 0, 0, 1, 0, 0, tfr[6] & 0xF0);
      FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ);
      //
    } else if (tfr[7] == DRV08_CMD_EXECUTE_DEVICE_DIAGNOSTIC) {
      //
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Execute Device Diagnostic");
      //
      Drv08_WriteTaskFile (ch,0x01, 0x01, 0x01, 0x00, 0x00, 0x00);
      FileIO_FCh_WriteStat(ch, DRV08_STATUS_END);
      //
    } else if (tfr[7] == DRV08_CMD_IDENTIFY_DEVICE) {
      //
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Identify Device");
      //
      Drv08_IdentifyDevice(pDrive, pDesc, fbuf16);
      Drv08_WriteTaskFile (ch,0, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);

      FileIO_FCh_WriteStat(ch, DRV08_STATUS_RDY); // pio in (class 1) command type
      SPI_EnableFileIO();
      SPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_W)); // write data command*/
      for (i = 0; i < 256; i++)
      {
          SPI((uint8_t) fbuf16[i]);
          SPI((uint8_t)(fbuf16[i] >> 8));
      }
      SPI_DisableFileIO();
      FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ);
      //
    } else if (tfr[7] == DRV08_CMD_INITIALIZE_DEVICE_PARAMETERS) {
      //
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Initialize Device Parameters");
      //
      Drv08_WriteTaskFile (ch,0, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
      FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ);
      //
    } else if (tfr[7] == DRV08_CMD_SET_MULTIPLE_MODE) {
      //
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Set Multiple Mode");
      //
      pDesc->sectors_per_block = tfr[2];
      FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ);
      //
    } else if (tfr[7] == DRV08_CMD_READ_SECTORS) {
      //
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Read Sectors");
      //
      Drv08_GetParams(tfr, pDesc, &sector, &cylinder, &head, &sector_count, &lba, &lba_mode);
      Drv08_HardFileSeek(pDrive, lba);

      while (sector_count)
      {
        FileIO_FCh_WriteStat(ch, DRV08_STATUS_RDY); // pio in (class 1) command type
        FileIO_FCh_WaitStat (ch, FILEIO_REQ_OK_FM_ARM, FILEIO_REQ_OK_FM_ARM); // wait for nearly empty buffer
        FileIO_FCh_WriteStat(ch, DRV08_STATUS_IRQ);

        if (!first) {
          Drv08_IncParams(pDesc, &sector, &cylinder, &head, &lba);
          Drv08_UpdateParams(ch, tfr, sector, cylinder,  head, lba, lba_mode);
        }
        first = 0;

        /*Drv08_FileReadSend(ch, pDrive, fbuf);*/
        Drv08_FileReadSendDirect(ch, pDrive,1); // read and send block


        sector_count--; // decrease sector count
      }
      //
    } else if (tfr[7] == DRV08_CMD_READ_MULTIPLE) { // Read Multiple Sectors (multiple sector transfer per IRQ)
      //
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Read Sectors Multiple");
      //
      if (pDesc->sectors_per_block == 0) {
        WARNING("Drv08:Set Multiple not done");
        Drv08_WriteTaskFile (ch, DRV08_ERROR_ABRT, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
        FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ | DRV08_STATUS_ERR);
        return;
      }

      FileIO_FCh_WriteStat(ch, DRV08_STATUS_RDY); // pio in (class 1) command type

      Drv08_GetParams(tfr, pDesc, &sector, &cylinder, &head, &sector_count, &lba, &lba_mode);
      Drv08_HardFileSeek(pDrive, lba);

      while (sector_count) {
        block_count = sector_count;
        if (block_count > pDesc->sectors_per_block)
            block_count = pDesc->sectors_per_block;

        FileIO_FCh_WriteStat(ch, DRV08_STATUS_IRQ);

        // calc final sector number in block
        i = block_count;
        while (i--) {
          if (!first) Drv08_IncParams(pDesc, &sector, &cylinder, &head, &lba);
          first = 0;
        }
        // update, this should be done at the end really, but we need to make sure the transfer has not completed
        Drv08_UpdateParams(ch, tfr, sector, cylinder,  head, lba, lba_mode);

        // do transfer
        while (block_count) {
          i = block_count;
          if (i > DRV08_MAX_READ_BURST) i = DRV08_MAX_READ_BURST;
          FileIO_FCh_WaitStat (ch, FILEIO_REQ_OK_FM_ARM, FILEIO_REQ_OK_FM_ARM);
          Drv08_FileReadSendDirect(ch, pDrive, i); // read and send block(s)

          sector_count-= i;
          block_count -= i;
        } // end block

      }  // end sector
      //
    } else if (tfr[7] == DRV08_CMD_READ_VERIFY) { // Read verify
      //
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Read Verify");
      //
      Drv08_WriteTaskFile (ch,0, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);

      FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ); // IRQ or not?

    } else if (tfr[7] == DRV08_CMD_WRITE_SECTORS) {  // write sectors
      // ADD CHECK FOR READ ONLY FILE
      //
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Write Sectors");
      //
      FileIO_FCh_WriteStat(ch, DRV08_STATUS_REQ); // pio out (class 2) command type
      Drv08_GetParams(tfr, pDesc, &sector, &cylinder, &head, &sector_count, &lba, &lba_mode);
      Drv08_HardFileSeek(pDrive, lba);

      while (sector_count)
      {
        FileIO_FCh_WaitStat (ch, FILEIO_REQ_OK_TO_ARM, FILEIO_REQ_OK_TO_ARM); // wait for data

        // fetch
        SPI_EnableFileIO();
        SPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_R));
        SPI_ReadBufferSingle(fbuf, DRV08_BLK_SIZE);
        SPI_DisableFileIO();

        // safe to do after fetch, STATUS_END clears busy
        if (!first) {
          Drv08_IncParams(pDesc, &sector, &cylinder, &head, &lba);
          Drv08_UpdateParams(ch, tfr, sector, cylinder,  head, lba, lba_mode);
        }
        first = 0;

        sector_count--; // decrease sector count

        // release core
        if (sector_count)
          FileIO_FCh_WriteStat(ch, DRV08_STATUS_IRQ);
        else
          FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ); // last one

        // optimal to put this after the status update, but then we cannot indicate write failure
        // write to file
        Drv08_FileWrite(ch, pDrive, fbuf);
      }
    } else if (tfr[7] == DRV08_CMD_WRITE_MULTIPLE) { // write sectors
      // ADD CHECK FOR READ ONLY FILE
      //
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Write Sectors Multiple");
      //
      if (pDesc->sectors_per_block == 0) {
        WARNING("Drv08:Set Multiple not done");
        Drv08_WriteTaskFile (ch, DRV08_ERROR_ABRT, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
        FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ | DRV08_STATUS_ERR);
        return;
      }

      FileIO_FCh_WriteStat(ch, DRV08_STATUS_REQ); // pio out (class 2) command type

      Drv08_GetParams(tfr, pDesc, &sector, &cylinder, &head, &sector_count, &lba, &lba_mode);
      Drv08_HardFileSeek(pDrive, lba);

      while (sector_count)
      {
        block_count = sector_count;
        if (block_count > pDesc->sectors_per_block)
            block_count = pDesc->sectors_per_block;

        while (block_count) {

          FileIO_FCh_WaitStat (ch, FILEIO_REQ_OK_TO_ARM, FILEIO_REQ_OK_TO_ARM); // wait for data

          // fetch
          SPI_EnableFileIO();
          SPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_R));
          SPI_ReadBufferSingle(fbuf, DRV08_BLK_SIZE);
          SPI_DisableFileIO();

          // write to file
          Drv08_FileWrite(ch, pDrive, fbuf);

          if (!first) {
            Drv08_IncParams(pDesc, &sector, &cylinder, &head, &lba);
          }
          first = 0;

          block_count--;
          sector_count--; // decrease sector count
        }
        Drv08_UpdateParams(ch, tfr, sector, cylinder,  head, lba, lba_mode);

        // release core
        if (sector_count)
          FileIO_FCh_WriteStat(ch, DRV08_STATUS_IRQ);
        else
          FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ); // last one

      } // end sector
      //
    } else {
      //
      WARNING("Drv08:Unknown ATA command:");
      WARNING("%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X",
           tfr[0],tfr[1],tfr[2],tfr[3],tfr[4],tfr[5],tfr[6],tfr[7]);
      Drv08_WriteTaskFile(ch, DRV08_ERROR_ABRT, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
      FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ | DRV08_STATUS_ERR);
    }
}

// insert functions
void Drv08_GetHardfileType(fch_t *pDrive, drv08_desc_t *pDesc)
{
  uint8_t fbuf[DRV08_BLK_SIZE];
  uint32_t i = 0;

  FF_Seek(pDrive->fSource, 0, FF_SEEK_SET);
  for (i = 0; i < 16; ++i) { // check first 16 blocks
    FF_Read(pDrive->fSource, DRV08_BLK_SIZE, 1, fbuf);

    /*DumpBuffer(fbuf, 32);*/

    if (!memcmp(fbuf, "RDSK", 4)) {
      INFO("Drv08:RDB OK");
      return;
    }

    if ( (!memcmp(fbuf, "DOS", 3)) || (!memcmp(fbuf, "PFS", 3)) || (!memcmp(fbuf, "SFS", 3)) ) {
      WARNING("Drv08:RDB MISSING");
      return;
    }
  }
  WARNING("Drv08:Unknown HDF format");
}

void Drv08_GetHardfileGeometry(fch_t *pDrive, drv08_desc_t *pDesc)
{ // this function comes from WinUAE, should return the same CHS as WinUAE
   uint32_t total;
   uint32_t i;
   uint32_t head = 0;
   uint32_t cyl  = 0;
   uint32_t spt  = 0;
   uint32_t sptt[] = { 63, 127, 255, 0 };
   uint32_t size = pDesc->file_size;

   if (size == 0)
     return;

    total = size / 512;

    for (i = 0; sptt[i] !=0; i++) {
        spt = sptt[i];
        for (head = 4; head <= 16; head++) {
            cyl = total / (head * spt);
            if (size <= 512 * 1024 * 1024) {
              if (cyl <= 1023)
                 break;
            } else {
              if (cyl < 16383)
                break;
              if (cyl < 32767 && head >= 5)
                break;
              if (cyl <= 65535)
                break;
            }
        }
        if (head <= 16)
          break;
    }
    pDesc->cylinders         = (unsigned short)cyl;
    pDesc->heads             = (unsigned short)head;
    pDesc->sectors           = (unsigned short)spt;
    pDesc->sectors_per_block = 0; // catch if not set to !=0
}


//
// interface
//

void FileIO_Drv08_Process(uint8_t ch, fch_t handle[2][FCH_MAX_NUM], uint8_t status) // ata
{
  // only one format supported at the moment.
  // we don't know what drive yet, status does not contain unit id (although it could ...)
  Drv08_ATA_Handle(ch, handle);
}

uint8_t FileIO_Drv08_InsertInit(uint8_t ch, uint8_t drive_number, fch_t *pDrive, char *ext)
{
  DEBUG(1,"Drv08:InsertInit");

  uint32_t time = Timer_Get(0);

  //pDrive points to the base fch_t struct for this unit. It contains a pointer (pDesc) to our drv08_desc_t

  pDrive->pDesc = calloc(1, sizeof(drv08_desc_t)); // 0 everything
  drv08_desc_t* pDesc = pDrive->pDesc;

  pDesc->format    = (drv08_format_t)XXX;
  pDesc->file_size =  pDrive->fSource->Filesize;

  if (strnicmp(ext, "HDF",3) == 0) {
    //
    pDesc->format    = (drv08_format_t)HDF;
  } else {
    WARNING("Drv01:Unsupported format.");
    return (1);
  }
  // common stuff (as only HDF supported for now...

  Drv08_GetHardfileType(pDrive, pDesc);
  Drv08_GetHardfileGeometry(pDrive, pDesc);

  time = Timer_Get(0) - time;

  INFO("SIZE: %lu (%lu MB)", pDesc->file_size, pDesc->file_size >> 20);
  INFO("CHS : %u.%u.%u", pDesc->cylinders, pDesc->heads, pDesc->sectors);
  INFO("      --> %lu MB", ((((unsigned long) pDesc->cylinders) * pDesc->heads * pDesc->sectors) >> 11));
  INFO("Opened in %lu ms", time >> 20);

  return (0);
}

