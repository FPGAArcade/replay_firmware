
#include "hardware.h"
#include "config.h"
#include "messaging.h"
#include "fileio_hdd.h"

// #define HDD_DEBUG 1
/*#define HDD_DEBUG_READ_MULTIPLE 1*/

/** @brief extern link to sdcard i/o manager

  Having this here is "ugly", but ff_ioman declares this "prototype"
  in the header which is actually code and should be in the .c file.
  Won't fix for now to avoid issues with the fullfat stuff....

  Usually it should be part of the module header where it belongs to,
  not at all in this .h/.c file...
*/
extern FF_IOMAN *pIoman;
uint8_t chb_driver_type = 0; // temp

// hardfile structure
hdfTYPE hdf[CHB_MAX_NUM];

uint8_t HDD_fBuf[HDD_BUF_SIZE];

// pull these out into FILEIO
/*#define FILEIO_HD_STAT_R 0x40*/
/*#define FILEIO_HD_STAT_W 0x48*/
/*#define FILEIO_HD_CMD_W  0x50*/
/*#define FILEIO_HD_CMD_R  0x58*/
/*#define FILEIO_HD_FIFO_R 0x60*/
/*#define FILEIO_HD_FIFO_W 0x70*/


// helper function for byte swapping
// modfified the code for a less elegant version, but gives no more warning!
void HDD_SwapBytes(uint8_t *ptr, uint32_t len)
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

uint32_t HDD_chs2lba(uint16_t cylinder, uint8_t head, uint16_t sector, uint8_t unit)
{
  return(cylinder * hdf[unit].heads + head) * hdf[unit].sectors + sector - 1;
}

void HDD_IdentifyDevice(uint16_t *pBuffer, uint8_t unit)
{ // builds Identify Device struct

  char *p;
  uint32_t total_sectors = hdf[unit].cylinders * hdf[unit].heads * hdf[unit].sectors;

  memset(pBuffer, 0, 512);

  pBuffer[0] = 1 << 6; // hard disk
  pBuffer[1] = hdf[unit].cylinders; // cyl count
  pBuffer[3] = hdf[unit].heads;     // head count
  pBuffer[6] = hdf[unit].sectors;   // sectors per track
  memcpy((char*)&pBuffer[10], "1234567890ABCDEFGHIJ", 20); // serial number
  memcpy((char*)&pBuffer[23], ".100    ", 8); // firmware version
  //
  p = (char*)&pBuffer[27];
  memcpy(p, "Replay                                  ", 40); // model name - byte swapped
  p += 8;
  strncpy(p,(char *)hdf[unit].name,16);      // file name as part of model name - byte swapped
  HDD_SwapBytes((uint8_t*)&pBuffer[27], 40);
  //
  pBuffer[47] = 0x8010; //maximum sectors per block in Read/Write Multiple command (16 in this case) 8K bytes (4K * 16)
  pBuffer[53] = 1;
  pBuffer[54] = hdf[unit].cylinders;
  pBuffer[55] = hdf[unit].heads;
  pBuffer[56] = hdf[unit].sectors;
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

void HDD_WriteTaskFile(unsigned char error, unsigned char sector_count, unsigned char sector_number, unsigned char cylinder_low, unsigned char cylinder_high, unsigned char drive_head)
{
    SPI_EnableFileIO();

    SPI(FILEIO_HD_CMD_W); // write task file registers command
    SPI(0x00);
    SPI(error); // error
    SPI(sector_count); // sector count
    SPI(sector_number); //sector number
    SPI(cylinder_low); // cylinder low
    SPI(cylinder_high); // cylinder high
    SPI(drive_head); // drive/head
    SPI_DisableFileIO();
}

// note : now FDD /HDD are identical driver (bar address), functions should be moved to common fileio file

uint8_t HDD_FileIO_GetStat(void)
{
  uint8_t stat;
  SPI_EnableFileIO();
  SPI(FILEIO_HD_STAT_R);
  stat = SPI(0);
  SPI_DisableFileIO();
  return stat;
}

void HDD_WriteStatus(uint8_t status)
{
    SPI_EnableFileIO();
    SPI(FILEIO_HD_STAT_W);
    SPI(status);
    SPI_DisableFileIO();
}

uint8_t HDD_WaitStat(uint8_t mask, uint8_t wanted)
{
  uint8_t  stat;
  uint32_t timeout = Timer_Get(500);
  do {
    stat = HDD_FileIO_GetStat();

    if (Timer_Check(timeout)) {
      WARNING("HDD_WaitStat:Waitstat timeout.");
      return (1);
    }
  } while ((stat & mask) != wanted);
  return (0);
}

void HDD_ATA_Handle(void)
{
    unsigned short id[256];
    unsigned char  tfr[8];
    unsigned short i;
    unsigned short sector;
    unsigned short cylinder;
    unsigned char  head;
    unsigned char  unit;
    unsigned long  lba;
    unsigned short sector_count;
    unsigned short block_count;
    unsigned char  sector_buffer[512];


    ACTLED_ON;
    SPI_EnableFileIO();
    SPI(FILEIO_HD_CMD_R); // read task file registers
    SPI(0); // dummy
    for (i = 0; i < 8; i++)
    {
      tfr[i] = SPI(0);
    }
    SPI_DisableFileIO();

    #ifdef HDD_DEBUG
    DEBUG(1,"IDE: %02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X",
            tfr[0],tfr[1],tfr[2],tfr[3],tfr[4],tfr[5],tfr[6],tfr[7]);
    #endif

    unit = tfr[6] & 0x10 ? 1 : 0; // master/slave selection
    // unit seems to be valid for all commands

    if (!(hdf[unit].status & HD_INSERTED))
    {
      DEBUG(1,"HDF (%s) accessed but hardfile not present",unit?"slave":"master");
    }
    else if ((tfr[7] & 0xF0) == ACMD_RECALIBRATE) // Recalibrate 0x10-0x1F (class 3 command: no data)
    {
      #ifdef HDD_DEBUG
      DEBUG(1,"Recalibrate");
      #endif
      HDD_WriteTaskFile(0, 0, 1, 0, 0, tfr[6] & 0xF0);
      HDD_WriteStatus(IDE_STATUS_END | IDE_STATUS_IRQ);
    }
    else if (tfr[7] == ACMD_EXECUTE_DEVICE_DIAGNOSTIC) // Execute Device Diagnostic
    {
      #ifdef HDD_DEBUG
      DEBUG(1,"Execute Device Diagnostic");
      #endif
      HDD_WriteTaskFile(0x01, 0x01, 0x01, 0x00, 0x00, 0x00);
      HDD_WriteStatus(IDE_STATUS_END);
    }
    else if (tfr[7] == ACMD_IDENTIFY_DEVICE) // Identify Device
    {
      #ifdef HDD_DEBUG
      DEBUG(1,"Identify Device");
      #endif

      HDD_IdentifyDevice(id, unit);
      HDD_WriteTaskFile(0, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
      HDD_WriteStatus(IDE_STATUS_RDY); // pio in (class 1) command type
      SPI_EnableFileIO();
      SPI(FILEIO_HD_FIFO_W); // write data command
      for (i = 0; i < 256; i++)
      {
          SPI((uint8_t) id[i]);
          SPI((uint8_t)(id[i] >> 8));
      }
      SPI_DisableFileIO();
      HDD_WriteStatus(IDE_STATUS_END | IDE_STATUS_IRQ);
    }
    else if (tfr[7] == ACMD_INITIALIZE_DEVICE_PARAMETERS) // Initiallize Device Parameters
    {
      #ifdef HDD_DEBUG
      DEBUG(1,"Initialize Device Parameters");
      #endif

      HDD_WriteTaskFile(0, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
      HDD_WriteStatus(IDE_STATUS_END | IDE_STATUS_IRQ);
    }
    else if (tfr[7] == ACMD_SET_MULTIPLE_MODE) // Set Multiple Mode
    {
      #ifdef HDD_DEBUG
      DEBUG(1,"Set Multiple Mode");
      #endif

      hdf[unit].sectors_per_block = tfr[2];
      HDD_WriteStatus(IDE_STATUS_END | IDE_STATUS_IRQ);
    }
    else if (tfr[7] == ACMD_READ_SECTORS) // Read Sectors
    {
      #ifdef HDD_DEBUG
      DEBUG(1,"Read Sectors");
      #endif

      HDD_WriteStatus(IDE_STATUS_RDY); // pio in (class 1) command type

      sector   = tfr[3];
      cylinder = tfr[4] | tfr[5]<<8;
      head     = tfr[6] & 0x0F;

      if (tfr[6] & 0x40)
        lba = head<<24 | cylinder<<8 | sector;
      else
        lba = HDD_chs2lba(cylinder, head, sector, unit);

      sector_count = tfr[2];

      if (sector_count == 0)
         sector_count = 0x100;

      #ifdef HDD_DEBUG
      DEBUG(1,"unit     : %d", unit);
      DEBUG(1,"sector   : %d", sector);
      DEBUG(1,"cylinder : %d", cylinder);
      DEBUG(1,"head     : %d", head);
      DEBUG(1,"lba      : %ld", lba);
      DEBUG(1,"count    : %d", sector_count);
      #endif

      HDD_HardFileSeek(&hdf[unit], lba);
      while (sector_count)
      {
          HDD_WriteStatus(IDE_STATUS_IRQ); //
          HDD_WaitStat(FILEIO_HD_REQ_OK_FM_ARM, FILEIO_HD_REQ_OK_FM_ARM); // wait for above command to enable FIFO
          HDD_FileRead(hdf[unit].fSource);
          sector_count--; // decrease sector count
      }
    }
    else if (tfr[7] == ACMD_READ_MULTIPLE) // Read Multiple Sectors (multiple sector transfer per IRQ)
    {
      #ifdef HDD_DEBUG
      DEBUG(1,"Read Sectors Multiple");
      #endif

      HDD_WriteStatus(IDE_STATUS_RDY); // pio in (class 1) command type

      sector   = tfr[3];
      cylinder = tfr[4] | tfr[5]<<8;
      head     = tfr[6] & 0x0F;

      if (tfr[6] & 0x40) // LBA bit
       lba = head<<24 | cylinder<<8 | sector;
      else
       lba = HDD_chs2lba(cylinder, head, sector, unit);

      sector_count = tfr[2];
      if (sector_count == 0) // count 0 = 256 logical sectors
         sector_count = 0x100;

      #ifdef HDD_DEBUG_READ_MULTIPLE
      DEBUG(1,"unit     : %d", unit);
      DEBUG(1,"sector   : %d", sector);
      DEBUG(1,"cylinder : %d", cylinder);
      DEBUG(1,"head     : %d", head);
      DEBUG(1,"lba      : %ld", lba);
      DEBUG(1,"count    : %d", sector_count);
      #endif

      HDD_HardFileSeek(&hdf[unit], lba);

      while (sector_count)
      {

        block_count = sector_count;
        if (block_count > hdf[unit].sectors_per_block)
            block_count = hdf[unit].sectors_per_block;

        HDD_WriteStatus(IDE_STATUS_IRQ);
        for (int cur_block = 0; cur_block < block_count; ++cur_block) // to be optimized
        {
          HDD_WaitStat(FILEIO_HD_REQ_OK_FM_ARM, FILEIO_HD_REQ_OK_FM_ARM); // wait for rx ok
          HDD_FileRead(hdf[unit].fSource);
        /*HDD_FileReadEx(hdf[unit].fSource, block_count);*/

        }

        sector_count -= block_count; // decrease sector count
      }
    }
    else if (tfr[7] == ACMD_WRITE_SECTORS) // write sectors
    {
      DEBUG(1,"HDD write single");
      HDD_WriteStatus(IDE_STATUS_REQ); // pio out (class 2) command type

      sector = tfr[3];
      cylinder = tfr[4] | tfr[5]<<8;
      head = tfr[6] & 0x0F;

      if (tfr[6] & 0x40)
         lba = head<<24 | cylinder<<8 | sector;
      else
         lba =  HDD_chs2lba(cylinder, head, sector, unit);

      sector_count = tfr[2];
      if (sector_count == 0)
          sector_count = 0x100;

      HDD_HardFileSeek(&hdf[unit], lba);

      while (sector_count)
      {
        HDD_WaitStat(FILEIO_HD_REQ_OK_TO_ARM, FILEIO_HD_REQ_OK_TO_ARM);

        SPI_EnableFileIO();
        SPI(FILEIO_HD_FIFO_R);
        SPI_ReadBufferSingle(HDD_fBuf, HDD_BUF_SIZE);
        SPI_DisableFileIO();

        sector_count--; // decrease sector count

        if (sector_count)
          HDD_WriteStatus(IDE_STATUS_IRQ);
        else
          HDD_WriteStatus(IDE_STATUS_END | IDE_STATUS_IRQ);

        FF_Write(hdf[unit].fSource,HDD_BUF_SIZE,1,HDD_fBuf);
      }
    }
    else if (tfr[7] == ACMD_WRITE_MULTIPLE) // write sectors
    {
      DEBUG(1,"HDD write multiple");

      HDD_WriteStatus(IDE_STATUS_REQ); // pio out (class 2) command type

      sector = tfr[3];
      cylinder = tfr[4] | tfr[5]<<8;
      head = tfr[6] & 0x0F;

      if (tfr[6] & 0x40)
         lba = head<<24 | cylinder<<8 | sector;
      else
          lba =  HDD_chs2lba(cylinder, head, sector, unit);

      sector_count = tfr[2];
      if (sector_count == 0)
          sector_count = 0x100;

      HDD_HardFileSeek(&hdf[unit], lba);

      while (sector_count)
      {
        block_count = sector_count;
        if (block_count > hdf[unit].sectors_per_block)
            block_count = hdf[unit].sectors_per_block;

        while (block_count)
        {
          HDD_WaitStat(FILEIO_HD_REQ_OK_TO_ARM, FILEIO_HD_REQ_OK_TO_ARM);

          SPI_EnableFileIO();
          SPI(FILEIO_HD_FIFO_R);
          SPI_ReadBufferSingle(HDD_fBuf, HDD_BUF_SIZE);
          SPI_DisableFileIO();

          FF_Write(hdf[unit].fSource,HDD_BUF_SIZE,1,HDD_fBuf);

          block_count--;  // decrease block count
          sector_count--; // decrease sector count
        }

        if (sector_count)
            HDD_WriteStatus(IDE_STATUS_IRQ);
        else
            HDD_WriteStatus(IDE_STATUS_END | IDE_STATUS_IRQ);
      }
    }
    else
    {
      WARNING("Unknown ATA command:");
      WARNING("%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X",
           tfr[0],tfr[1],tfr[2],tfr[3],tfr[4],tfr[5],tfr[6],tfr[7]);
      HDD_WriteTaskFile(0x04, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
      HDD_WriteStatus(IDE_STATUS_END | IDE_STATUS_IRQ | IDE_STATUS_ERR);
    }
    ACTLED_OFF;
}


unsigned char HDD_HardFileSeek(hdfTYPE *pHDF, unsigned long lba)
{
  return FF_Seek(pHDF->fSource, lba*512, FF_SEEK_SET);
}

void HDD_FileRead(FF_FILE *fSource)
{

  //#warning TODO: Read block from file and send to FPGA directly --> in mmc.c of original minimig firmware
  // calls MMC_READ
  uint32_t bytes_read;
  bytes_read = FF_Read(fSource, HDD_BUF_SIZE, 1, HDD_fBuf);
  if (bytes_read == 0) return; // catch 0 len file error

  SPI_EnableFileIO();
  SPI(FILEIO_HD_FIFO_W);
  SPI_WriteBufferSingle(HDD_fBuf, HDD_BUF_SIZE);
  SPI_DisableFileIO();
}

void HDD_FileReadEx(FF_FILE *fSource, uint16_t block_count) {
  // calls MMC_READmultiple

  #warning TODO: Read blocks from file and send to FPGA --> in mmc.c of original minimig firmware
}

//
//
//

void HDD_GetHardfileGeometry(hdfTYPE *pHDF)
{ // this function comes from WinUAE, should return the same CHS as WinUAE
   uint32_t total;
   uint32_t i, head, cyl, spt;
   uint32_t sptt[] = { 63, 127, 255, -1 };
   uint32_t size = pHDF->size;

   if (size == 0)
     return;

    total = size / 512;

    for (i = 0; sptt[i] >= 0; i++)
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
    pHDF->cylinders = (unsigned short)cyl;
    pHDF->heads = (unsigned short)head;
    pHDF->sectors = (unsigned short)spt;
}

void HDD_OpenHardfile(uint8_t unit, char *filename)
{
  uint32_t time = Timer_Get(0);
  hdf[unit].status = 0;
  hdf[unit].name[0] = 0;
  hdf[unit].fSource = FF_Open(pIoman, filename, FF_MODE_READ | FF_MODE_WRITE, NULL);
  hdf[unit].size = 0;

  if (!hdf[unit].fSource) {
    INFO("HDF (%s) open error",unit?"slave":"master");
    MSG_warning("Insert HD:Could not open file.");
    return;
  }

  hdf[unit].size = hdf[unit].fSource ->Filesize;

  HDD_GetHardfileGeometry(&hdf[unit]);
  // we removed the indexing - assuming the fullfat stuff handles it good enough...

  FileDisplayName(hdf[unit].name, MAX_DISPLAY_FILENAME, filename);

  hdf[unit].status = HD_INSERTED | HD_WRITABLE;

  time = Timer_Get(0) - time;

  INFO("HDF (%s)",unit?"slave":"master");
  INFO("SIZE: %lu (%lu MB)", hdf[unit].size, hdf[unit].size >> 20);
  INFO("CHS : %u.%u.%u", hdf[unit].cylinders, hdf[unit].heads, hdf[unit].sectors);
  INFO("      --> %lu MB", ((((unsigned long) hdf[unit].cylinders) * hdf[unit].heads * hdf[unit].sectors) >> 11));
  INFO("Opened in %lu ms", time >> 20);
}


void HDD_Handle(void)
{
  uint8_t status = HDD_FileIO_GetStat();
  // cheat for now

  if (status & FILEIO_HD_REQ_ACT) {
    HDD_ATA_Handle();
  }
}

void HDD_UpdateDriveStatus(void)
{
  uint8_t status = 0;

  for (int i=0; i<CHB_MAX_NUM; ++i) {
    if (hdf[i].status & HD_INSERTED) status |= (0x01<<i);
    if (hdf[i].status & HD_WRITABLE) status |= (0x10<<i);
  }
  DEBUG(1,"HDD:update status %02X",status);
  OSD_ConfigSendFileIO_CHB(status);
}

void HDD_Insert(uint8_t drive_number, char *path)
{
  DEBUG(1,"attempting to insert hd <%d> : <%s> ", drive_number,path);
  if (drive_number < CHB_MAX_NUM) {
    char* pFile_ext = GetExtension(path);
    if (strnicmp(pFile_ext, "HDF",3) == 0) {
      // HDF
      HDD_OpenHardfile(drive_number, path);
    } else {
      // generic
      MSG_warning("Filetype not supported.");
      return;
    }
  }
  HDD_UpdateDriveStatus();
}

void HDD_Eject(uint8_t drive_number)
{
  DEBUG(1,"Ejecting hd <%d>", drive_number);

  /*if (drive_number < HD_MAX_NUM) {*/
    /*hdfTYPE* drive = &hdf[drive_number];*/
    /*FF_Close(drive->fSource);*/
    /*drive->status = 0;*/
  /*}*/
  HDD_UpdateDriveStatus();

}

uint8_t HDD_Inserted(uint8_t drive_number)
{
  if (drive_number < CHB_MAX_NUM) {
    return (hdf[drive_number].status & HD_INSERTED);
  } else
  return FALSE;
}

char* HDD_GetName(uint8_t drive_number)
{
  if (drive_number < CHB_MAX_NUM) {
    return (hdf[drive_number].name);
  }
  return null_string; // in stringlight.c
}

void HDD_SetDriver(uint8_t type)
{
  chb_driver_type = type;
}

void HDD_Init(void)
{
  DEBUG(1,"HDD:Init");
  uint32_t i;
  for (i=0; i<CHB_MAX_NUM; i++) {
    hdf[i].status = 0; hdf[i].fSource = NULL;
    hdf[i].name[0] = '\0';
  }
}

