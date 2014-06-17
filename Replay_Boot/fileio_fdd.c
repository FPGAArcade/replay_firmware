
#include "hardware.h"
#include "config.h"
#include "messaging.h"
#include "fileio_fdd.h"


const uint8_t FDD_DEBUG = 1;

extern FF_IOMAN *pIoman;

adfTYPE fdf[FD_MAX_NUM];        // drive 0 information structure

// localized buffer for this module, in case of cache
uint8_t  FDD_fBuf[FDD_BUF_SIZE];

//
// FILEIO (to be moved)
//

uint8_t FDD_FileIO_GetStat(void)
{
  uint8_t stat;
  SPI_EnableFpga();
  SPI(0x00);
  stat = SPI(0);
  SPI_DisableFpga();
  return stat;
}

//
// READ
//


/*{{{*/
/*void FDD_AmigaHeader(adfTYPE *drive,  uint8_t track, uint8_t sector, uint16_t dsksync)*/
/*{*/
  /*uint8_t checksum[4];*/
  /*uint16_t i;*/
  /*uint8_t x;*/
  /*uint8_t *p;*/

  /*uint32_t addr_ts = (SECTOR_COUNT * SECTOR_SIZE * track) + (SECTOR_SIZE * sector);*/
  /*uint32_t time;*/

  /*// read block to calc checksum*/
  /*// could add local cache check here if offset == last_offset*/

  /*time = Timer_Get(0);*/
  /*FF_Seek(drive->fSource, addr_ts, FF_SEEK_SET); // check return*/
  /*time = Timer_Get(0) - time;*/
  /*INFO("header seek in %lu ms", time >> 20);*/

  /*time = Timer_Get(0);*/
  /*FF_Read(drive->fSource, 512, 1, FDD_fBuf);   // check return*/
  /*time = Timer_Get(0) - time;*/
  /*INFO("header read in %lu ms", time >> 20);*/


  /*//DEBUG(1,"FDD:header seek :%08X",addr_ts);*/

  /*// workaround for Copy Lock in Wiz'n'Liz and North&South (might brake other games)*/
  /*if (dsksync == 0x0000 || dsksync == 0x8914 || dsksync == 0xA144)*/
    /*dsksync = 0x4489;*/

  /*// North&South: $A144*/
  /*// Wiz'n'Liz (Copy Lock): $8914*/
  /*// Prince of Persia: $4891*/
  /*// Commando: $A245*/

  /*uint8_t dsksynch = (uint8_t)(dsksync >> 8);*/
  /*uint8_t dsksyncl = (uint8_t)(dsksync);*/

  /*SPI_EnableFpga();*/
  /*SPI(0x50); // enable write*/

  /*// preamble*/
  /*SPI(0xAA);*/
  /*SPI(0xAA);*/
  /*SPI(0xAA);*/
  /*SPI(0xAA);*/

  /*// synchronization*/
  /*SPI(dsksynch);*/
  /*SPI(dsksyncl);*/
  /*SPI(dsksynch);*/
  /*SPI(dsksyncl);*/

  /*// odd bits of header*/
  /*x = 0x55;*/
  /*checksum[0] = x;*/
  /*SPI(x);*/
  /*x = (track >> 1) & 0x55;*/
  /*checksum[1] = x;*/
  /*SPI(x);*/
  /*x = (sector >> 1) & 0x55;*/
  /*checksum[2] = x;*/
  /*SPI(x);*/
  /*// subtraction has higher prio, added brackets accordingly!*/
  /*x = ((11 - sector) >> 1) & 0x55;*/
  /*checksum[3] = x;*/
  /*SPI(x);*/

  /*// even bits of header*/
  /*x = 0x55;*/
  /*checksum[0] ^= x;*/
  /*SPI(x);*/
  /*x = track & 0x55;*/
  /*checksum[1] ^= x;*/
  /*SPI(x);*/
  /*x = sector & 0x55;*/
  /*checksum[2] ^= x;*/
  /*SPI(x);*/
  /*// subtraction has higher prio, added brackets accordingly!*/
  /*x = (11 - sector) & 0x55;*/
  /*checksum[3] ^= x;*/
  /*SPI(x);*/

  /*// sector label and reserved area (changes nothing to checksum)*/
  /*i = 0x20;*/
  /*while (i--)*/
    /*SPI(0xAA);*/

  /*// send header checksum*/
  /*SPI(0xAA);*/
  /*SPI(0xAA);*/
  /*SPI(0xAA);*/
  /*SPI(0xAA);*/
  /*SPI(checksum[0] | 0xAA);*/
  /*SPI(checksum[1] | 0xAA);*/
  /*SPI(checksum[2] | 0xAA);*/
  /*SPI(checksum[3] | 0xAA);*/

  /*// calculate data checksum*/
  /*checksum[0] = 0;*/
  /*checksum[1] = 0;*/
  /*checksum[2] = 0;*/
  /*checksum[3] = 0;*/

  /*p = FDD_fBuf;*/
  /*i = 512 / 4;*/
  /*while (i--) {*/
    /*x = *p++;*/
    /*checksum[0] ^= x ^ x >> 1;*/
    /*x = *p++;*/
    /*checksum[1] ^= x ^ x >> 1;*/
    /*x = *p++;*/
    /*checksum[2] ^= x ^ x >> 1;*/
    /*x = *p++;*/
    /*checksum[3] ^= x ^ x >> 1;*/
  /*}*/

  /*// send data checksum*/
  /*SPI(0xAA);*/
  /*SPI(0xAA);*/
  /*SPI(0xAA);*/
  /*SPI(0xAA);*/
  /*SPI(checksum[0] | 0xAA);*/
  /*SPI(checksum[1] | 0xAA);*/
  /*SPI(checksum[2] | 0xAA);*/
  /*SPI(checksum[3] | 0xAA);*/

  /*SPI_DisableFpga();*/

/*}*/
/*}}}*/

/*{{{*/
/*void FDD_Read(adfTYPE *drive, uint32_t offset, uint16_t size)*/
/*{ // track number is updated in drive struct before calling this function*/
  /*uint16_t i;*/
  /*uint8_t *p;*/
  /*uint32_t time;*/

  /*if (size > FDD_BUF_SIZE) {*/
    /*ERROR("size too big"); // temp, split over buffers*/
    /*return;*/
  /*}*/

  /*//DEBUG(1,"FDD:sector seek: %08X, size %04X",offset, size);*/

  /*// could add local cache check here if offset == last_offset*/
  /*// looks like FF is optimal with seeking to current pos, but worth checking*/
  /*time = Timer_Get(0);*/
  /*FF_Seek(drive->fSource, offset, FF_SEEK_SET); // check return*/
  /*time = Timer_Get(0) - time;*/
  /*INFO("block seek in %lu ms", time >> 20);*/

  /*time = Timer_Get(0);*/
  /*FF_Read(drive->fSource, size, 1, FDD_fBuf);   // check return*/
  /*time = Timer_Get(0) - time;*/
  /*INFO("block read in %lu ms", time >> 20);*/

  /*SPI_EnableFpga();*/
  /*SPI(0x50); // enable write*/

  /*i = size;*/
  /*p = FDD_fBuf;*/

  /*while (i--)*/
    /*SPI(*p++);*/

  /*SPI_DisableFpga();*/
/*}*/
/*}}}*/

void FDD_SendSector(uint8_t *pData, uint8_t sector, uint8_t track, uint8_t dsksynch, uint8_t dsksyncl)
{
  //DumpBuffer(pData, 64);
  if (FDD_DEBUG)
    DEBUG(1,"FDD_SendSector %04X, %04X", sector, track);

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

void FDD_AmigaRead(adfTYPE *drive)
{ // track number is updated in drive struct before calling this function
  uint8_t  sector;
  uint8_t  track;
 //uint16_t dsklen;
  uint32_t offset;

  uint8_t  status;
  uint8_t  cmd;
  uint16_t size;
  uint16_t addr;
  uint16_t dsksync; //user
  uint16_t i;

  if (drive->track >= drive->tracks) {
    ERROR("Illegal track %u read!", drive->track);
    drive->track = drive->tracks - 1;
  }

  // display track number: cylinder & head
  if (FDD_DEBUG)
    DEBUG(1,"FDD_ReadTrack #%u", drive->track);

  offset = (512*11) * drive->track;

  if (drive->track != drive->track_prev) {
    // track step or track 0, start at beginning of track
    drive->track_prev = drive->track;
    sector = 0;
    drive->sector_offset = sector;
    FF_Seek(drive->fSource, offset, FF_SEEK_SET);
  } else {
    // same track, start at next sector in track
    sector = drive->sector_offset;
    FF_Seek(drive->fSource, offset + (sector<<9), FF_SEEK_SET);
  }
  while (1) {
    // note read moves on file pointer automatically
    FF_Read(drive->fSource, FDD_BUF_SIZE, 1, FDD_fBuf);

    SPI_EnableFpga();
    SPI(0x00);
    status   = SPI(0); // cmd request
    cmd      = SPI(0);
    size     = SPI(0); //8 bits to be expanded
    addr     = SPI(0) << 8;
    addr    |= SPI(0);
    dsksync     = SPI(0) << 8;
    dsksync    |= SPI(0);
    SPI_DisableFpga();


    track = (uint8_t) addr & 0xFF;

    // end of disk check
    if (track >= drive->tracks)
      track = drive->tracks - 1;

    // workaround for Copy Lock in Wiz'n'Liz and North&South (might brake other games)
    if (dsksync == 0x0000 || dsksync == 0x8914 || dsksync == 0xA144)
      dsksync = 0x4489;

    // North&South: $A144
    // Wiz'n'Liz (Copy Lock): $8914
    // Prince of Persia: $4891
    // Commando: $A245

    /*if (FDD_DEBUG)*/
      /*DEBUG(2,"#%X:%04X", sector, dsklen);*/



    // some loaders stop dma if sector header isn't what they expect
    // because we don't check dma transfer count after sending a word
    // the track can be changed while we are sending the rest of the previous sector
    // in this case let's start transfer from the beginning
    if (track == drive->track) {
      // send sector if fpga is still asking for data - not required as checked on entry

         SPI_EnableFpga();
         SPI(0x50); // enable write
         FDD_SendSector(FDD_fBuf, sector, track, (uint8_t)(dsksync >> 8), (uint8_t)dsksync);
         SPI_DisableFpga();

         if (sector == LAST_SECTOR)
         {

          SPI_EnableFpga();
          SPI(0x51); // temp use ch1 for gap (index)

           // send gap
          i = 128;
          while (i--)
            SPI(0xAA);
          SPI_DisableFpga();
          //
          SPI_EnableFpga();
          SPI(0x50);
          // send gap
          i = GAP_SIZE-128;
          while (i--)
            SPI(0xAA);
          SPI_DisableFpga();
         }
    }

    // we are done accessing FPGA
    SPI_DisableFpga();

    // track has changed
    if (track != drive->track)
      break;

    // read request
    if (!(status & 0x08))
      break;

    sector++;
    if (sector < SECTOR_COUNT) {
      //all is good, do nothing
    } else {
      // go to the start of current track
      sector = 0;
      FF_Seek(drive->fSource, ((512*11) * drive->track), FF_SEEK_SET);
    }
    // remember current sector and cluster
    drive->sector_offset = sector;
  }
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
  /*SPI_EnableFpga();*/
  /*SPI(0x20);*/
  /*SPI(fdf[0].status | (fdf[1].status << 1) | (fdf[2].status << 2) | (fdf[3].status << 3));*/
  /*SPI_DisableFpga();*/
}

void FDD_Handle(void)
{


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

  /*uint8_t  status;*/
  /*uint8_t  cmd;*/
  /*uint16_t size;*/
  /*uint16_t addr;*/
  /*uint16_t user;*/

  /*uint8_t  sel;*/
  /*uint32_t addr_ts;*/

  /*SPI_EnableFpga();*/
  /*SPI(0x00);*/
  /*status   = SPI(0); // cmd request*/
  /*cmd      = SPI(0);*/
  /*size     = SPI(0); //8 bits to be expanded*/
  /*addr     = SPI(0) << 8;*/
  /*addr    |= SPI(0);*/
  /*user     = SPI(0) << 8;*/
  /*user    |= SPI(0);*/
  /*SPI_DisableFpga();*/

  /*// ack all commands*/
  /*SPI_EnableFpga();*/
  /*SPI(0x18);*/
  /*SPI_DisableFpga();*/

  /*sel  = status & 0x03;*/

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

  /*addr_ts = (SECTOR_COUNT * SECTOR_SIZE) * (addr & 0xFF) + (SECTOR_SIZE * (addr>>8));*/

  // temp
  /*fdf[sel].track = (uint8_t) (addr & 0xFF);*/
  /*ACTLED_ON;*/

  /*if (status & 0x4) { // write*/
  /*} else {*/
    /*DEBUG(1,"FDD:handle request drive:%02X cmd:%02X addr:%04X user:%08X",sel,cmd,addr,user);*/

    /*FDD_AmigaRead(&fdf[sel]);*/


    /*switch (cmd) {*/
      // amiga floppy header
      /*case 0x80: FDD_AmigaHeader(&df[sel], (addr & 0xFF), (addr>>8), user); break;*/
      // block transfer, 512 with address from track/sector
      /*case 0x81 : FDD_Read(&df[sel], addr_ts, 512); break;*/

    /*}*/
  /*}*/

  /*ACTLED_OFF;*/

  // transfer complete
  /*SPI_EnableFpga();*/
  /*SPI(0x19);*/
  /*SPI_DisableFpga();*/

}

void FDD_Insert_0x00(adfTYPE *drive)
{
  drive->tracks = 1;
}

void FDD_Insert_0x01(adfTYPE *drive)
{
  uint16_t tracks;
  tracks = drive->fSource->Filesize / (512*11);
  if (tracks > FD_MAX_TRACKS) {
    MSG_warning("UNSUPPORTED ADF SIZE!!! Too many tracks: %lu", tracks);
    tracks = FD_MAX_TRACKS;
  }
  drive->tracks = (uint16_t)tracks;
}

void FDD_Insert(uint8_t drive_number, char *path)
{
  uint8_t type;

  DEBUG(1,"attempting to inserting floppy <%d> : <%s> ", drive_number,path);

  if (drive_number < FD_MAX_NUM) {
    adfTYPE* drive = &fdf[drive_number];
    drive->status = 0;
    drive->name[0] = '\0';

    drive->fSource = FF_Open(pIoman, path, FF_MODE_READ, NULL);

    if (!drive->fSource) {
      MSG_warning("Insert Floppy:Could not open file.");
      return;
    }
    // do request read to find out FDD core request type
    type = (FDD_FileIO_GetStat() >> 4) & 0x0F;
    DEBUG(1,"request type <%d>", type);
    switch (type) {
      case 0x0: FDD_Insert_0x01(drive); break; // BODGE TO AMIGA
      case 0x1: FDD_Insert_0x01(drive); break;
      MSG_warning("Unknown FDD request type."); 
      return;
    }
    FileDisplayName(drive->name, MAX_DISPLAY_FILENAME, path);

    drive->status = FD_INSERTED;
    /*if (!(drive->fSource.Mode & FF_MODE_WRITE)) // read-only attribute*/
      /*drive->status |= FD_WRITABLE;*/

    DEBUG(1,"Inserted floppy: <%s>", drive->name);
    DEBUG(1,"file size   : %lu (%lu kB)", drive->fSource->Filesize, drive->fSource->Filesize >> 10);
    DEBUG(1,"drive tracks: %u", drive->tracks);
    DEBUG(1,"drive status: 0x%02X", drive->status);
  }
}

void FDD_Eject(uint8_t drive_number)
{
  DEBUG(1,"Ejecting floppy <%d>", drive_number);

  if (drive_number < FD_MAX_NUM) {
    adfTYPE* drive = &fdf[drive_number];
    FF_Close(drive->fSource);
    drive->status = 0;
  }
}

uint8_t FDD_Inserted(uint8_t drive_number)
{
  if (drive_number < FD_MAX_NUM) {
    return (fdf[drive_number].status & FD_INSERTED);
  } else
  return FALSE;
}

char* FDD_GetName(uint8_t drive_number)
{
  if (drive_number < FD_MAX_NUM) {
    return (fdf[drive_number].name);
  }
  return NULL; // mmmmm
}

void FDD_Init(void)
{
  DEBUG(1,"FDD:Init");
  uint32_t i;
  for (i=0; i<FD_MAX_NUM; i++) {
    fdf[i].status = 0; fdf[i].fSource = NULL;
    fdf[i].name[0] = '\0';
    fdf[i].tracks = 0;

  }
}
