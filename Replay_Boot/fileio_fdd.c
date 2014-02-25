
#include "hardware.h"
#include "config.h"
#include "messaging.h"
#include "fileio_fdd.h"

const uint8_t FDD_DEBUG = 1;

adfTYPE df[4];        // drive 0 information structure

// localized buffer for this module, in case of cache
uint8_t  FDD_fBuf[FDD_BUF_SIZE];

//
// READ
//

void FDD_AmigaHeader(adfTYPE *drive,  uint8_t track, uint8_t sector, uint16_t dsksync)
{
  uint8_t checksum[4];
  uint16_t i;
  uint8_t x;
  uint8_t *p;

  uint32_t addr_ts = (SECTOR_COUNT * SECTOR_SIZE * track) + (SECTOR_SIZE * sector);

  // read block to calc checksum
  // could add local cache check here if offset == last_offset
  FF_Seek(drive->fSource, addr_ts, FF_SEEK_SET); // check return
  FF_Read(drive->fSource, 512, 1, FDD_fBuf);   // check return

  // workaround for Copy Lock in Wiz'n'Liz and North&South (might brake other games)
  if (dsksync == 0x0000 || dsksync == 0x8914 || dsksync == 0xA144)
    dsksync = 0x4489;

  // North&South: $A144
  // Wiz'n'Liz (Copy Lock): $8914
  // Prince of Persia: $4891
  // Commando: $A245

  uint8_t dsksynch = (uint8_t)(dsksync >> 8);
  uint8_t dsksyncl = (uint8_t)(dsksync);

  SPI_EnableFpga();
  SPI(0x40); // enable write

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

  p = FDD_fBuf;
  i = 512 / 4;
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

  SPI_DisableFpga();

}

void FDD_Read(adfTYPE *drive, uint32_t offset, uint16_t size)
{ // track number is updated in drive struct before calling this function
  uint16_t i;
  uint8_t *p;

  if (size > FDD_BUF_SIZE) {
    ERROR("size too big"); // temp, split over buffers
    return;
  }
  // could add local cache check here if offset == last_offset
  FF_Seek(drive->fSource, offset, FF_SEEK_SET); // check return
  FF_Read(drive->fSource, size, 1, FDD_fBuf);   // check return
  SPI_EnableFpga();
  SPI(0x40); // enable write

  i = size;
  p = FDD_fBuf;

  while (i--)
    SPI(*p++);

  SPI_DisableFpga();
}

//
// WRITE
//

/*{{{*/
/*uint8_t FDD_FindSync(adfTYPE *drive)*/
/*// reads data from fifo till it finds sync word or fifo is empty and dma inactive (so no more data is expected)*/
/*{*/
  /*uint8_t  c1, c2, c3, c4;*/
  /*uint16_t n;*/

  /*while (1) {*/
    /*SPI_EnableFpga();*/
    /*c1 = SPI(0); // write request signal*/
    /*c2 = SPI(0); // track number (cylinder & head)*/
    /*if (!(c1 & CMD_WRTRK))*/
      /*break;*/
    /*if (c2 != drive->track)*/
      /*break;*/
    /*SPI(0); // disk sync high byte*/
    /*SPI(0); // disk sync low byte*/
    /*c3 = SPI(0) & 0xBF; // msb of mfm words to transfer*/
    /*c4 = SPI(0);        // lsb of mfm words to transfer*/

    /*if (c3 == 0 && c4 == 0)*/
      /*break;*/

    /*n = ((c3 & 0x3F) << 8) + c4;*/

    /*while (n--) {*/
      /*c3 = SPI(0);*/
      /*c4 = SPI(0);*/
      /*if (c3 == 0x44 && c4 == 0x89) {*/
        /*SPI_DisableFpga();*/
        /*if (FDD_DEBUG)*/
          /*DEBUG(1,"#SYNC");*/
        /*return 1;*/
      /*}*/
    /*}*/
    /*SPI_DisableFpga();*/
  /*}*/
  /*SPI_DisableFpga();*/
  /*return 0;*/
/*}*/
/*}}}*/

/*{{{*/
/*uint8_t FDD_GetHeader(uint8_t *pTrack, uint8_t *pSector)*/
/*// this function reads data from fifo till it finds sync word or dma is inactive*/
/*{*/
  /*uint8_t c, c1, c2, c3, c4;*/
  /*uint8_t i;*/
  /*uint8_t checksum[4];*/
  /*uint8_t error = 0;*/

  /*while (1) {*/
    /*SPI_EnableFpga();*/
    /*c1 = SPI(0); // write request signal*/
    /*c2 = SPI(0); // track number (cylinder & head)*/
    /*if (!(c1 & CMD_WRTRK))*/
        /*break;*/
    /*SPI(0); // disk sync high byte*/
    /*SPI(0); // disk sync low byte*/
    /*c3 = SPI(0); // msb of mfm words to transfer*/
    /*c4 = SPI(0); // lsb of mfm words to transfer*/

    /*if ((c3 & 0x3F) != 0 || c4 > 24) {// remaining header data is 25 mfm words*/
      /*c1 = SPI(0); // second sync lsb*/
      /*c2 = SPI(0); // second sync msb*/
      /*if (c1 != 0x44 || c2 != 0x89) {*/
        /*error = 21;*/
        /*ERROR("Second sync word missing!");*/
        /*break;*/
      /*}*/

      /*c = SPI(0);*/
      /*checksum[0] = c;*/
      /*c1 = (c & 0x55) << 1;*/
      /*c = SPI(0);*/
      /*checksum[1] = c;*/
      /*c2 = (c & 0x55) << 1;*/
      /*c = SPI(0);*/
      /*checksum[2] = c;*/
      /*c3 = (c & 0x55) << 1;*/
      /*c = SPI(0);*/
      /*checksum[3] = c;*/
      /*c4 = (c & 0x55) << 1;*/

      /*c = SPI(0);*/
      /*checksum[0] ^= c;*/
      /*c1 |= c & 0x55;*/
      /*c = SPI(0);*/
      /*checksum[1] ^= c;*/
      /*c2 |= c & 0x55;*/
      /*c = SPI(0);*/
      /*checksum[2] ^= c;*/
      /*c3 |= c & 0x55;*/
      /*c = SPI(0);*/
      /*checksum[3] ^= c;*/
      /*c4 |= c & 0x55;*/

      /*if (c1 != 0xFF) // always 0xFF*/
         /*error = 22;*/
      /*else if (c2 > 159) // Track number (0-159)*/
         /*error = 23;*/
      /*else if (c3 > 10) // Sector number (0-10)*/
         /*error = 24;*/
      /*else if (c4 > 11 || c4 == 0) // Number of sectors to gap (1-11)*/
         /*error = 25;*/

      /*if (error) {*/
        /*ERROR("Wrong header: %u.%u.%u.%u", c1, c2, c3, c4);*/
        /*break;*/
      /*}*/
      /**pTrack = c2;*/
      /**pSector = c3;*/

      /*for (i = 0; i < 8; i++) {*/
        /*checksum[0] ^= SPI(0);*/
        /*checksum[1] ^= SPI(0);*/
        /*checksum[2] ^= SPI(0);*/
        /*checksum[3] ^= SPI(0);*/
      /*}*/
      /*checksum[0] &= 0x55;*/
      /*checksum[1] &= 0x55;*/
      /*checksum[2] &= 0x55;*/
      /*checksum[3] &= 0x55;*/

      /*c1 = (SPI(0) & 0x55) << 1;*/
      /*c2 = (SPI(0) & 0x55) << 1;*/
      /*c3 = (SPI(0) & 0x55) << 1;*/
      /*c4 = (SPI(0) & 0x55) << 1;*/

      /*c1 |= SPI(0) & 0x55;*/
      /*c2 |= SPI(0) & 0x55;*/
      /*c3 |= SPI(0) & 0x55;*/
      /*c4 |= SPI(0) & 0x55;*/

      /*if (c1 != checksum[0] || c2 != checksum[1] || c3 != checksum[2] || c4 != checksum[3]) {*/
        /*error = 26;*/
         /*break;*/
      /*}*/

      /*SPI_DisableFpga();*/
      /*return 1;*/
    /*} else if ((c3 & 0x80) == 0) {*/
      /*// not enough data for header and write dma is not active*/
      /*error = 20;*/
      /*break;*/
    /*}*/

    /*SPI_DisableFpga();*/
  /*}*/
  /*SPI_DisableFpga();*/

  /*if (error) {*/
    /*ERROR("GetHeader: error %u", error);*/
  /*}*/

  /*return 0;*/
/*}*/
/*}}}*/

/*{{{*/
/*uint8_t FDD_GetData(void)*/
/*{*/
  /*uint8_t  c, c1, c2, c3, c4;*/
  /*uint8_t  i;*/
  /*uint8_t  *p;*/
  /*uint16_t n;*/
  /*uint8_t  checksum[4];*/

  /*uint8_t error = 0;*/
  /*while (1) {*/
    /*SPI_EnableFpga();*/
    /*c1 = SPI(0); // write request signal*/
    /*c2 = SPI(0); // track number (cylinder & head)*/
    /*if (!(c1 & CMD_WRTRK))*/
      /*break;*/
    /*SPI(0); // disk sync high byte*/
    /*SPI(0); // disk sync low byte*/
    /*c3 = SPI(0); // msb of mfm words to transfer*/
    /*c4 = SPI(0); // lsb of mfm words to transfer*/

    /*n = ((c3 & 0x3F) << 8) + c4;*/

    /*if (n >= 0x204) {*/
      /*c1 = (SPI(0) & 0x55) << 1;*/
      /*c2 = (SPI(0) & 0x55) << 1;*/
      /*c3 = (SPI(0) & 0x55) << 1;*/
      /*c4 = (SPI(0) & 0x55) << 1;*/

      /*c1 |= SPI(0) & 0x55;*/
      /*c2 |= SPI(0) & 0x55;*/
      /*c3 |= SPI(0) & 0x55;*/
      /*c4 |= SPI(0) & 0x55;*/

      /*checksum[0] = 0;*/
      /*checksum[1] = 0;*/
      /*checksum[2] = 0;*/
      /*checksum[3] = 0;*/

      /*// odd bits of data field*/
      /*i = 128;*/
      /*p = FDD_fBuf;*/
      /*do {*/
        /*c = SPI(0);*/
        /*checksum[0] ^= c;*/
        /**p++ = (c & 0x55) << 1;*/
        /*c = SPI(0);*/
        /*checksum[1] ^= c;*/
        /**p++ = (c & 0x55) << 1;*/
        /*c = SPI(0);*/
        /*checksum[2] ^= c;*/
        /**p++ = (c & 0x55) << 1;*/
        /*c = SPI(0);*/
        /*checksum[3] ^= c;*/
        /**p++ = (c & 0x55) << 1;*/
      /*}*/
      /*while (--i);*/

      /*// even bits of data field*/
      /*i = 128;*/
      /*p = FDD_fBuf;*/
      /*do {*/
        /*c = SPI(0);*/
        /*checksum[0] ^= c;*/
        /**p++ |= c & 0x55;*/
        /*c = SPI(0);*/
        /*checksum[1] ^= c;*/
        /**p++ |= c & 0x55;*/
        /*c = SPI(0);*/
        /*checksum[2] ^= c;*/
        /**p++ |= c & 0x55;*/
        /*c = SPI(0);*/
        /*checksum[3] ^= c;*/
        /**p++ |= c & 0x55;*/
      /*}*/

      /*while (--i);*/
       /*checksum[0] &= 0x55;*/
       /*checksum[1] &= 0x55;*/
       /*checksum[2] &= 0x55;*/
       /*checksum[3] &= 0x55;*/

       /*if (c1 != checksum[0] || c2 != checksum[1] || c3 != checksum[2] || c4 != checksum[3]) {*/
         /*error = 29;*/
         /*break;*/
       /*}*/

       /*SPI_DisableFpga();*/
       /*return 1;*/
     /*} else if ((c3 & 0x80) == 0) {*/
       /*// not enough data in fifo and write dma is not active*/
       /*error = 28;*/
       /*break;*/
     /*}*/

    /*SPI_DisableFpga();*/
  /*}*/
  /*SPI_DisableFpga();*/

  /*if (error) {*/
    /*ERROR("GetData: error %u", error);*/
  /*}*/

  /*return 0;*/
/*}*/
/*}}}*/

/*{{{*/
/*void FDD_WriteTrack(adfTYPE *drive)*/
/*{*/

  /*uint8_t rx_track  = 0;*/
  /*uint8_t rx_sector = 0;*/
  /*uint8_t sector;*/

  /*uint8_t error = 0;*/

  /*// setting file pointer to begining of current track*/
  /*uint32_t offset = (512*11) * drive->track;*/
  /*drive->track_prev = drive->track + 1;  // just to force next read from the start of current track */
  /*drive->sector_offset = 0;*/
  /*sector = 0;*/

  /*FF_Seek(drive->fSource, offset, FF_SEEK_SET);*/

  /*if (FDD_DEBUG)*/
    /*DEBUG(1,"*%u", drive->track);*/

  /*while (FDD_FindSync(drive)) {*/
    /*if (FDD_GetHeader(&rx_track, &rx_sector)) {*/
      /*if (rx_track == drive->track) {*/
        /*while (sector != rx_sector) {*/

          /*if (sector < rx_sector) {*/
            /*// next sector*/
            /*FF_Seek(drive->fSource, 512, FF_SEEK_CUR);*/
            /*sector++;*/
          /*} else {*/
            /*// set to start of track*/
            /*offset = (512*11) * drive->track;*/
            /*drive->sector_offset = 0;*/
            /*sector = 0;*/
            /*FF_Seek(drive->fSource, offset, FF_SEEK_SET);*/
          /*}*/
        /*}*/

        /*if (FDD_GetData()) {*/
          /*if (drive->status & DSK_WRITABLE)*/
            /*FF_Write(drive->fSource, FDD_BUF_SIZE, 1, FDD_fBuf);*/
          /*else {*/
            /*error = 30;*/
            /*ERROR("Write attempt to protected disk!");*/
          /*}*/
        /*}*/
      /*} else {*/
        /*error = 27; //track number reported in sector header is not the same as current drive track*/
      /*}*/
    /*}*/

    /*if (error) {*/
      /*ERROR("WriteTrack: error %u", error);*/
    /*}*/
  /*}*/
/*}*/
/*}}}*/

void FDD_UpdateDriveStatus(void)
{
  SPI_EnableFpga();
  SPI(0x10);
  SPI(df[0].status | (df[1].status << 1) | (df[2].status << 2) | (df[3].status << 3));
  SPI_DisableFpga();
}

void FDD_Handle(void)
{
  // request must be asserted to get here

  //if (status & 0x4) { // write
    /*ACTLED_ON;*/
    /*sel = (c1 >> 6) & 0x03;*/
    /*df[sel].track = c2;*/
    /*FDD_WriteTrack(&df[sel]);*/
    /*ACTLED_OFF;*/
  //} else {
   // ACTLED_ON;
  //  df[sel].track = (uint8_t) (addr & 0xFF);
  //  FDD_ReadTrack(&df[sel]);
  //  ACTLED_OFF;
  //}

  uint8_t  status;
  uint16_t cmd;
  uint16_t user;
  uint8_t  sel;
  uint8_t  req;
  uint16_t addr;
  uint16_t size;
  uint32_t addr_ts;

  SPI_EnableFpga();
  SPI(0x00);
  status   = SPI(0); // cmd request
  cmd      = SPI(0) << 8;
  cmd     |= SPI(0);
  user     = SPI(0) << 8;
  user    |= SPI(0);
  SPI_DisableFpga();

  // ack all commands
  SPI_EnableFpga();
  SPI(0x19);
  SPI_DisableFpga();

  sel  = status & 0x03;
  req  = (cmd & 0xF000) >> 12;
  addr = (cmd & 0x0FFF);

  /*size = 512; // default*/
  /*switch (req & 0x07) {*/
    /*case 0 : size =   32; break;*/
    /*case 1 : size =   64; break;*/
    /*case 2 : size =  128; break;*/
    /*case 3 : size =  256; break;*/
    /*case 4 : size =  512; break;*/
    /*case 5 : size = 1024; break;*/
    /*case 6 : break; // not defined*/
    /*case 7 : break; // not defined*/
  /*}*/

  addr_ts = (SECTOR_COUNT * SECTOR_SIZE) * (addr & 0xFF) + (SECTOR_SIZE * (addr>>8));

  if (status & 0x4) { // write
  } else {
    DEBUG(1,"FDD:handle request drive:%02X req:%02X addr:%04X user:%08X",sel,req,addr,user);

    switch (req) {
      // amiga floppy header
      case 8 : FDD_AmigaHeader(&df[sel], (addr & 0xFF), (addr>>8), user); break;
      // block transfer, 512 with address from track/sector
      case 9 : FDD_Read(&df[sel], addr_ts, 512); break;

      // note we do 3x seek and reads of the same block now.
      // check file system caches otherwise we need to do here
    }
  }
  // transfer complete
  SPI_EnableFpga();
  SPI(0x1A);
  SPI_DisableFpga();

}

void FDD_Init(void)
{
  DEBUG(1,"FDD:Init");
  uint32_t i;
  for (i=0; i<4; i++) {
    df[i].status = 0; df[i].fSource = NULL;
  }
}
