
#include "hardware.h"
#include "config.h"
#include "messaging.h"
#include "amiga_hdd.h"


/*#define HDD_DEBUG 1*/
/*#define HDD_DEBUG_READ_MULTIPLE 1*/

/** @brief extern link to sdcard i/o manager

  Having this here is "ugly", but ff_ioman declares this "prototype"
  in the header which is actually code and should be in the .c file.
  Won't fix for now to avoid issues with the fullfat stuff....

  Usually it should be part of the module header where it belongs to,
  not at all in this .h/.c file...
*/
extern FF_IOMAN *pIoman;

// hardfile structure
hdfTYPE hdf[2];

uint8_t HDD_fBuf[HDD_BUF_SIZE];

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
    SPI_EnableFpga();

    SPI(CMD_IDE_REGS_WR); // write task file registers command
    SPI(0x00);
    SPI(error); // error
    SPI(sector_count); // sector count
    SPI(sector_number); //sector number
    SPI(cylinder_low); // cylinder low
    SPI(cylinder_high); // cylinder high
    SPI(drive_head); // drive/head
    SPI_DisableFpga();
}

void HDD_WriteStatus(uint8_t status)
{
    SPI_EnableFpga();

    SPI(CMD_IDE_STATUS_WR);
    SPI(status);
    SPI_DisableFpga();
}

uint8_t HDD_WaitStat(uint8_t mask, uint8_t wanted)
{
  uint8_t  stat;
  uint32_t timeout = Timer_Get(100);      // 100 ms timeout
  do {
    SPI_EnableFpga();
    SPI(0x00); // do Read
    stat = SPI(0);
    SPI_DisableFpga();

    if (Timer_Check(timeout)) {
      ERROR("FPGA:Waitstat timeout.");
      return (1);
    }
  } while ((stat & mask) != wanted);
  return (0);
}

void HDD_Handle(uint8_t spi_status)
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

    if (spi_status & CMD_IDECMD)
    {
      /*DEBUG(1,"IDE CMD HANDLE");*/

      ACTLED_ON;
      SPI_EnableFpga();
      SPI(CMD_IDE_REGS_RD); // read task file registers
      SPI(0); // dummy
      SPI(0); // dummy
      for (i = 0; i < 8; i++)
      {
        tfr[i] = SPI(0);
      }
      SPI_DisableFpga();

      #ifdef HDD_DEBUG
      DEBUG(1,"IDE: %02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X",
              tfr[0],tfr[1],tfr[2],tfr[3],tfr[4],tfr[5],tfr[6],tfr[7]);
      #endif

      unit = tfr[6] & 0x10 ? 1 : 0; // master/slave selection
      // unit seems to be valid for all commands

      if (!hdf[unit].size)
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
        SPI_EnableFpga();
        SPI(CMD_IDE_DATA_WR); // write data command
        for (i = 0; i < 256; i++)
        {
            SPI((uint8_t) id[i]);
            SPI((uint8_t)(id[i] >> 8));
        }
        SPI_DisableFpga();
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
            HDD_WaitStat(0x20, 0x20); // wait for above command to enable FIFO
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
          for (int i = 0; i < block_count; ++i) // to be optimized
          {
            HDD_WaitStat(0x20, 0x20); // wait for rx ok
            HDD_FileRead(hdf[unit].fSource);
          /*HDD_FileReadEx(hdf[unit].fSource, block_count);*/

          }

          sector_count -= block_count; // decrease sector count
        }
      }
      else if (tfr[7] == ACMD_WRITE_SECTORS) // write sectors
      {
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
          #warning TODO - FPGA_Status
          //while (!(GetFPGAStatus() & CMD_IDEDAT)); // wait for full write buffer

          SPI_EnableFpga();
          SPI(CMD_IDE_DATA_RD); // read data command
          SPI(0x00);
          SPI(0x00);
          SPI(0x00);
          SPI(0x00);
          SPI(0x00);
          for (i = 0; i < 512; i++)
            sector_buffer[i] = SPI(0xFF);
          SPI_DisableFpga();

          sector_count--; // decrease sector count

          if (sector_count)
            HDD_WriteStatus(IDE_STATUS_IRQ);
          else
            HDD_WriteStatus(IDE_STATUS_END | IDE_STATUS_IRQ);

          FF_Write(hdf[unit].fSource,512,1,sector_buffer);
        }
      }
      else if (tfr[7] == ACMD_WRITE_MULTIPLE) // write sectors
      {
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
            #warning TODO - FPGA_Status
            //while (!(GetFPGAStatus() & CMD_IDEDAT)); // wait for full write buffer

            SPI_EnableFpga();
            SPI(CMD_IDE_DATA_RD); // read data command
            SPI(0x00);
            SPI(0x00);
            SPI(0x00);
            SPI(0x00);
            SPI(0x00);
            for (i = 0; i < 512; i++)
                sector_buffer[i] = SPI(0xFF);
            SPI_DisableFpga();
            FF_Write(hdf[unit].fSource,512,1,sector_buffer);

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
}

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

unsigned char HDD_HardFileSeek(hdfTYPE *pHDF, unsigned long lba)
{
  return FF_Seek(pHDF->fSource, lba*512, FF_SEEK_SET);
}

uint8_t HDD_OpenHardfile(char *filename, uint8_t unit)
{
  uint32_t time = Timer_Get(0);
  hdf[unit].present = 0;
  hdf[unit].name[0] = 0;
  hdf[unit].fSource = FF_Open(pIoman, filename, FF_MODE_READ, NULL);
  hdf[unit].size = 0;

  if (hdf[unit].fSource)
  {
    hdf[unit].size = hdf[unit].fSource ->Filesize;

    HDD_GetHardfileGeometry(&hdf[unit]);
    // we removed the indexing - assuming the fullfat stuff handles it good enough...
    strncpy((char *)hdf[unit].name,filename,MAX_DISPLAY_FILENAME);
    hdf[unit].present = 1;
    time = Timer_Get(0) - time;

    INFO("HDF (%s)",unit?"slave":"master");
    INFO("SIZE: %lu (%lu MB)", hdf[unit].size, hdf[unit].size >> 20);
    INFO("CHS : %u.%u.%u", hdf[unit].cylinders, hdf[unit].heads, hdf[unit].sectors);
    INFO("      --> %lu MB", ((((unsigned long) hdf[unit].cylinders) * hdf[unit].heads * hdf[unit].sectors) >> 11));
    INFO("Opened in %lu ms", time >> 20);
    return 1;
  }

  return 0;
}

void HDD_FileRead(FF_FILE *fSource) 
{

  //#warning TODO: Read block from file and send to FPGA directly --> in mmc.c of original minimig firmware
  // calls MMC_READ
  uint32_t bytes_read;
  bytes_read = FF_Read(fSource, HDD_BUF_SIZE, 1, HDD_fBuf);
  if (bytes_read == 0) return; // catch 0 len file error

  SPI_EnableFpga();
  SPI(CMD_IDE_DATA_WR);
  SPI_WriteBufferSingle(HDD_fBuf, HDD_BUF_SIZE);
  SPI_DisableFpga();
}

void HDD_FileReadEx(FF_FILE *fSource, uint16_t block_count) {
  // calls MMC_READmultiple

  #warning TODO: Read blocks from file and send to FPGA --> in mmc.c of original minimig firmware
}
