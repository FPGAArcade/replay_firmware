
#include "fileio_drv.h"
#include "hardware.h"
#include "messaging.h"

const uint8_t DRV08_DEBUG = 1;
#define DRV08_PARAM_DEBUG 1;

#define DRV08_BUF_SIZE 512
// status bits
#define DRV08_ASTATUS_END 0x80
#define DRV08_ASTATUS_IRQ 0x10
#define DRV08_ASTATUS_RDY 0x08
#define DRV08_ASTATUS_REQ 0x04
#define DRV08_ASTATUS_ERR 0x01

// commands
#define DRV08_ACMD_RECALIBRATE                  0x10
#define DRV08_ACMD_READ_SECTORS                 0x20
#define DRV08_ACMD_WRITE_SECTORS                0x30
#define DRV08_ACMD_EXECUTE_DEVICE_DIAGNOSTIC    0x90
#define DRV08_ACMD_INITIALIZE_DEVICE_PARAMETERS 0x91
#define DRV08_ACMD_READ_MULTIPLE                0xC4
#define DRV08_ACMD_WRITE_MULTIPLE               0xC5
#define DRV08_ACMD_SET_MULTIPLE_MODE            0xC6
#define DRV08_ACMD_IDENTIFY_DEVICE              0xEC

#define DRV08_BUF_SIZE 512 // sector size

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

void Drv08_WriteTaskFile(uint8_t ch, uint8_t error, uint8_t sector_count, uint8_t sector_number, uint8_t cylinder_low, uint8_t cylinder_high, uint8_t drive_head)
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

    for (i = 0; sptt[i] !=0; i++)
    {
        spt = sptt[i];
        for (head = 4; head <= 16; head++)
        {
            cyl = total / (head * spt);
            if (size <= 512 * 1024 * 1024)
            {
                if (cyl <= 1023)
                    break;
            }
            else
            {
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
    pDesc->sectors_per_block = 1; // ???? default is ?
}

FF_ERROR Drv08_HardFileSeek(fch_t* pDrive, uint32_t lba)
{
  return FF_Seek(pDrive->fSource, lba*512, FF_SEEK_SET);  // TO DO, REPLACE with <<
}

void Drv08_FileRead(uint8_t ch, fch_t* pDrive, uint8_t *pBuffer)// *fSource)
{

  //#warning TODO: Read block from file and send to FPGA directly
  uint32_t bytes_read;
  bytes_read = FF_Read(pDrive->fSource, DRV08_BUF_SIZE, 1, pBuffer);
  if (bytes_read == 0) return; // catch 0 len file error

  SPI_EnableFileIO();
  SPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_W));
  SPI_WriteBufferSingle(pBuffer, DRV08_BUF_SIZE);
  SPI_DisableFileIO();
}

void Drv08_FileReadEx(FF_FILE *fSource, uint16_t block_count) {
  // calls MMC_READmultiple

  #warning TODO: Read blocks from file and send to FPGA --> in mmc.c of original minimig firmware
}

//
//
//
void Drv08_ATA_Handle(uint8_t ch, fch_t handle[2][FCH_MAX_NUM])
{

    // file buffer
    uint16_t  fbuf16[DRV08_BUF_SIZE/2]; // to get 16 bit alignment for id.
    uint8_t  *fbuf = (uint8_t*) fbuf16; // reuse the buffer

    uint8_t  tfr[8];
    uint16_t i;
    uint8_t  unit;
    uint16_t sector_count;
    uint16_t block_count;
    uint16_t sector;
    uint16_t cylinder;
    uint8_t  head;
    uint32_t lba;

    // read task file
    SPI_EnableFileIO();
    SPI(FCH_CMD(ch,FILEIO_FCH_CMD_CMD_R | 0x0));
    SPI(0x00); // dummy
    for (i = 0; i < 8; i++)
    {
      tfr[i] = SPI(0);
    }
    SPI_DisableFileIO();

    //
    if (DRV08_DEBUG)
      DEBUG(1,"Drv08: %02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X",
            tfr[0],tfr[1],tfr[2],tfr[3],tfr[4],tfr[5],tfr[6],tfr[7]);

    unit = tfr[6] & 0x10 ? 1 : 0; // master/slave selection
    // unit seems to be valid for all commands

    fch_t* pDrive = &handle[ch][unit]; // get base
    drv08_desc_t* pDesc = pDrive->pDesc;

    if (!FileIO_FCh_GetInserted(ch, unit)) {
      DEBUG(1,"Drv08:Process Ch:%d Unit:%d not mounted", ch, unit);
      FileIO_FCh_WriteStat(ch, DRV08_ASTATUS_END | DRV08_ASTATUS_IRQ | DRV08_ASTATUS_ERR);
      return;
    }

    if ((tfr[7] & 0xF0) == DRV08_ACMD_RECALIBRATE) // Recalibrate 0x10-0x1F (class 3 command: no data)
    {
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Recalibrate");
      Drv08_WriteTaskFile(ch, 0, 0, 1, 0, 0, tfr[6] & 0xF0);
      FileIO_FCh_WriteStat(ch, DRV08_ASTATUS_END | DRV08_ASTATUS_IRQ);
    }
    else if (tfr[7] == DRV08_ACMD_EXECUTE_DEVICE_DIAGNOSTIC) // Execute Device Diagnostic
    {
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Execute Device Diagnostic");
      Drv08_WriteTaskFile(ch,0x01, 0x01, 0x01, 0x00, 0x00, 0x00);
      FileIO_FCh_WriteStat(ch, DRV08_ASTATUS_END);
    }
    else if (tfr[7] == DRV08_ACMD_IDENTIFY_DEVICE) // Identify Device
    {
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Identify Device");

      Drv08_IdentifyDevice(pDrive, pDesc, fbuf16);
      Drv08_WriteTaskFile(ch,0, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);

      FileIO_FCh_WriteStat(ch, DRV08_ASTATUS_RDY); // pio in (class 1) command type
      SPI_EnableFileIO();
      SPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_W)); // write data command*/
      for (i = 0; i < 256; i++)
      {
          SPI((uint8_t) fbuf16[i]);
          SPI((uint8_t)(fbuf16[i] >> 8));
      }
      SPI_DisableFileIO();
      FileIO_FCh_WriteStat(ch, DRV08_ASTATUS_END | DRV08_ASTATUS_IRQ);
    }
    else if (tfr[7] == DRV08_ACMD_INITIALIZE_DEVICE_PARAMETERS) // Initiallize Device Parameters
    {
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Initialize Device Parameters");

      Drv08_WriteTaskFile(ch,0, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
      FileIO_FCh_WriteStat(ch, DRV08_ASTATUS_END | DRV08_ASTATUS_IRQ);
    }
    else if (tfr[7] == DRV08_ACMD_SET_MULTIPLE_MODE) // Set Multiple Mode
    {
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Set Multiple Mode");
      pDesc->sectors_per_block = tfr[2];
      FileIO_FCh_WriteStat(ch, DRV08_ASTATUS_END | DRV08_ASTATUS_IRQ);
    }
    else if (tfr[7] == DRV08_ACMD_READ_SECTORS) // Read Sectors
    {
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Read Sectors");

      sector   = tfr[3];
      cylinder = tfr[4] | tfr[5]<<8;
      head     = tfr[6] & 0x0F;

      if (tfr[6] & 0x40)
        lba = head<<24 | cylinder<<8 | sector;
      else
        lba = Drv08_chs2lba(pDesc, cylinder, head, sector);

      sector_count = tfr[2];
      if (sector_count == 0)
         sector_count = 0x100;

      if (DRV08_DEBUG) {
        #ifdef DRV08_PARAM_DEBUG
        DEBUG(1,"unit     : %d", unit);
        DEBUG(1,"sector   : %d", sector);
        DEBUG(1,"cylinder : %d", cylinder);
        DEBUG(1,"head     : %d", head);
        DEBUG(1,"lba      : %ld", lba);
        DEBUG(1,"count    : %d", sector_count);
        #endif
      }

      Drv08_HardFileSeek(pDrive, lba);
      while (sector_count)
      {
          // update sector
          FileIO_FCh_WriteStat(ch, DRV08_ASTATUS_RDY); // pio in (class 1) command type
          Drv08_WriteTaskFile (ch, 0, tfr[2], sector, (uint8_t)cylinder, (uint8_t)(cylinder >> 8), (tfr[6] & 0xF0) | head);
          FileIO_FCh_WaitStat (ch, FILEIO_REQ_OK_FM_ARM, FILEIO_REQ_OK_FM_ARM); // wait for empty buffer
          FileIO_FCh_WriteStat(ch, DRV08_ASTATUS_IRQ);

          Drv08_FileRead(ch, pDrive, fbuf);
          sector_count--; // decrease sector count
      }
    }
    else if (tfr[7] == DRV08_ACMD_READ_MULTIPLE) // Read Multiple Sectors (multiple sector transfer per IRQ)
    {
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Read Sectors Multiple");

      FileIO_FCh_WriteStat(ch, DRV08_ASTATUS_RDY); // pio in (class 1) command type

      sector   = tfr[3];
      cylinder = tfr[4] | tfr[5]<<8;
      head     = tfr[6] & 0x0F;

      if (tfr[6] & 0x40) // LBA bit
       lba = head<<24 | cylinder<<8 | sector;
      else
       lba = Drv08_chs2lba(pDesc, cylinder, head, sector);

      sector_count = tfr[2];
      if (sector_count == 0) // count 0 = 256 logical sectors
         sector_count = 0x100;

      if (DRV08_DEBUG) {
        #ifdef DRV08_PARAM_DEBUG
        DEBUG(1,"unit     : %d", unit);
        DEBUG(1,"sector   : %d", sector);
        DEBUG(1,"cylinder : %d", cylinder);
        DEBUG(1,"head     : %d", head);
        DEBUG(1,"lba      : %ld", lba);
        DEBUG(1,"count    : %d", sector_count);
        #endif
      }


      Drv08_HardFileSeek(pDrive, lba);

      while (sector_count) {

        block_count = sector_count;
        if (block_count > pDesc->sectors_per_block)
            block_count = pDesc->sectors_per_block;

        FileIO_FCh_WriteStat(ch, DRV08_ASTATUS_IRQ);

        while (block_count--) {

          sector_count--;

          FileIO_FCh_WaitStat (ch, FILEIO_REQ_OK_FM_ARM, FILEIO_REQ_OK_FM_ARM); // wait for empty buffer
          Drv08_FileRead(ch, pDrive, fbuf);

        }
        Drv08_WriteTaskFile (ch, 0, tfr[2], sector, (uint8_t)cylinder, (uint8_t)(cylinder >> 8), (tfr[6] & 0xF0) | head);

      }
    }
    else if (tfr[7] == DRV08_ACMD_WRITE_SECTORS) // write sectors
    {
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Write Sectors");
      FileIO_FCh_WriteStat(ch, DRV08_ASTATUS_REQ); // pio out (class 2) command type

      /*sector = tfr[3];*/
      /*cylinder = tfr[4] | tfr[5]<<8;*/
      /*head = tfr[6] & 0x0F;*/

      /*if (tfr[6] & 0x40)*/
         /*lba = head<<24 | cylinder<<8 | sector;*/
      /*else*/
         /*lba =  HDD_chs2lba(cylinder, head, sector, unit);*/

      /*sector_count = tfr[2];*/
      /*if (sector_count == 0)*/
          /*sector_count = 0x100;*/

      /*HDD_HardFileSeek(&hdf[unit], lba);*/

      /*while (sector_count)*/
      /*{*/
        /*HDD_WaitStat(FILEIO_HD_REQ_OK_TO_ARM, FILEIO_HD_REQ_OK_TO_ARM);*/

        /*SPI_EnableFileIO();*/
        /*SPI(FILEIO_HD_FIFO_R);*/
        /*SPI_ReadBufferSingle(HDD_fBuf, HDD_BUF_SIZE);*/
        /*SPI_DisableFileIO();*/

        /*sector_count--; // decrease sector count*/

        if (sector_count)
          FileIO_FCh_WriteStat(ch, DRV08_ASTATUS_IRQ);
        else
          FileIO_FCh_WriteStat(ch, DRV08_ASTATUS_END | DRV08_ASTATUS_IRQ);

        /*FF_Write(hdf[unit].fSource,HDD_BUF_SIZE,1,HDD_fBuf);*/
      /*}*/
    }
    else if (tfr[7] == DRV08_ACMD_WRITE_MULTIPLE) // write sectors
    {
      if (DRV08_DEBUG) DEBUG(1,"Drv08:Write Sectors Multiple");

      FileIO_FCh_WriteStat(ch, DRV08_ASTATUS_REQ); // pio out (class 2) command type

      /*sector = tfr[3];*/
      /*cylinder = tfr[4] | tfr[5]<<8;*/
      /*head = tfr[6] & 0x0F;*/

      /*if (tfr[6] & 0x40)*/
         /*lba = head<<24 | cylinder<<8 | sector;*/
      /*else*/
          /*lba =  HDD_chs2lba(cylinder, head, sector, unit);*/

      /*sector_count = tfr[2];*/
      /*if (sector_count == 0)*/
          /*sector_count = 0x100;*/

      /*HDD_HardFileSeek(&hdf[unit], lba);*/

      /*while (sector_count)*/
      /*{*/
        /*block_count = sector_count;*/
        /*if (block_count > hdf[unit].sectors_per_block)*/
            /*block_count = hdf[unit].sectors_per_block;*/

        /*while (block_count)*/
        /*{*/
          /*HDD_WaitStat(FILEIO_HD_REQ_OK_TO_ARM, FILEIO_HD_REQ_OK_TO_ARM);*/

          /*SPI_EnableFileIO();*/
          /*SPI(FILEIO_HD_FIFO_R);*/
          /*SPI_ReadBufferSingle(HDD_fBuf, HDD_BUF_SIZE);*/
          /*SPI_DisableFileIO();*/

          /*FF_Write(hdf[unit].fSource,HDD_BUF_SIZE,1,HDD_fBuf);*/

          /*block_count--;  // decrease block count*/
          /*sector_count--; // decrease sector count*/
        /*}*/

        if (sector_count)
            FileIO_FCh_WriteStat(ch, DRV08_ASTATUS_IRQ);
        else
            FileIO_FCh_WriteStat(ch, DRV08_ASTATUS_END | DRV08_ASTATUS_IRQ);
      /*}*/
    }
    else
    {
      WARNING("Unknown ATA command:");
      WARNING("%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X",
           tfr[0],tfr[1],tfr[2],tfr[3],tfr[4],tfr[5],tfr[6],tfr[7]);
      Drv08_WriteTaskFile(ch, 0x04, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
      FileIO_FCh_WriteStat(ch, DRV08_ASTATUS_END | DRV08_ASTATUS_IRQ | DRV08_ASTATUS_ERR);
    }
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

uint8_t FileIO_Drv08_InsertInit(uint8_t ch, uint8_t drive_number, fch_t* pDrive, char *ext)
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

  Drv08_GetHardfileGeometry(pDrive, pDesc);

  time = Timer_Get(0) - time;

  INFO("SIZE: %lu (%lu MB)", pDesc->file_size, pDesc->file_size >> 20);
  INFO("CHS : %u.%u.%u", pDesc->cylinders, pDesc->heads, pDesc->sectors);
  INFO("      --> %lu MB", ((((unsigned long) pDesc->cylinders) * pDesc->heads * pDesc->sectors) >> 11));
  INFO("Opened in %lu ms", time >> 20);

  return (0);
}

