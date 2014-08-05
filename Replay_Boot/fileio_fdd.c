
#include "hardware.h"
#include "config.h"
#include "messaging.h"
#include "fileio_fdd.h"


const uint8_t FDD_DEBUG = 0;

extern FF_IOMAN *pIoman;
uint8_t cha_driver_type = 0; // temp

fddTYPE fdf[CHA_MAX_NUM];        // drive 0 information structure

// localized buffer for this module, in case of cache
uint8_t  FDD_fBuf[FDD_BUF_SIZE];

//
// FILEIO (to be moved)
//

uint8_t FDD_FileIO_GetStat(void)
{
  uint8_t stat;
  SPI_EnableFpga();
  SPI(FILEIO_FD_STAT_R);
  stat = SPI(0);
  SPI_DisableFpga();
  return stat;
}

void FDD_FileIO_WriteStat(uint8_t stat)
{
  SPI_EnableFpga();
  SPI(FILEIO_FD_STAT_W);
  SPI(stat);
  SPI_DisableFpga();
}

/*{{{*/
/*void FDD_AmigaHeader(fddTYPE *drive,  uint8_t track, uint8_t sector, uint16_t dsksync)*/
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
/*void FDD_Read(fddTYPE *drive, uint32_t offset, uint16_t size)*/
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

/*{{{*/
void FDD_SendAmigaSector(uint8_t *pData, uint8_t sector, uint8_t track, uint8_t dsksynch, uint8_t dsksyncl)
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
/*}}}*/

/*{{{*/
/*void FDD_AmigaRead(fddTYPE *drive)*/
/*{ // track number is updated in drive struct before calling this function*/
  /*uint8_t  sector;*/
  /*uint8_t  track;*/
 /*//uint16_t dsklen;*/
  /*uint32_t offset;*/

  /*uint8_t  status;*/
  /*uint8_t  cmd;*/
  /*uint16_t size;*/
  /*uint16_t addr;*/
  /*uint16_t dsksync; //user*/
  /*uint16_t i;*/

  /*if (drive->track >= drive->tracks) {*/
    /*ERROR("Illegal track %u read!", drive->track);*/
    /*drive->track = drive->tracks - 1;*/
  /*}*/

  /*// display track number: cylinder & head*/
  /*if (FDD_DEBUG)*/
    /*DEBUG(1,"FDD_ReadTrack #%u", drive->track);*/

  /*offset = (512*11) * drive->track;*/

  /*if (drive->track != drive->track_prev) {*/
    /*// track step or track 0, start at beginning of track*/
    /*drive->track_prev = drive->track;*/
    /*sector = 0;*/
    /*drive->sector_offset = sector;*/
    /*FF_Seek(drive->fSource, offset, FF_SEEK_SET);*/
  /*} else {*/
    /*// same track, start at next sector in track*/
    /*sector = drive->sector_offset;*/
    /*FF_Seek(drive->fSource, offset + (sector<<9), FF_SEEK_SET);*/
  /*}*/
  /*while (1) {*/
    /*// note read moves on file pointer automatically*/
    /*FF_Read(drive->fSource, FDD_BUF_SIZE, 1, FDD_fBuf);*/

    /*SPI_EnableFpga();*/
    /*SPI(0x00);*/
    /*status   = SPI(0); // cmd request*/
    /*cmd      = SPI(0);*/
    /*size     = SPI(0); //8 bits to be expanded*/
    /*addr     = SPI(0) << 8;*/
    /*addr    |= SPI(0);*/
    /*dsksync     = SPI(0) << 8;*/
    /*dsksync    |= SPI(0);*/
    /*SPI_DisableFpga();*/


    /*track = (uint8_t) addr & 0xFF;*/

    /*// end of disk check*/
    /*if (track >= drive->tracks)*/
      /*track = drive->tracks - 1;*/

    /*// workaround for Copy Lock in Wiz'n'Liz and North&South (might brake other games)*/
    /*if (dsksync == 0x0000 || dsksync == 0x8914 || dsksync == 0xA144)*/
      /*dsksync = 0x4489;*/

    /*// North&South: $A144*/
    /*// Wiz'n'Liz (Copy Lock): $8914*/
    /*// Prince of Persia: $4891*/
    /*// Commando: $A245*/

    /*if (FDD_DEBUG)*/
      /*DEBUG(2,"#%X:%04X", sector, dsklen);*/



    /*// some loaders stop dma if sector header isn't what they expect*/
    /*// because we don't check dma transfer count after sending a word*/
    /*// the track can be changed while we are sending the rest of the previous sector*/
    /*// in this case let's start transfer from the beginning*/
    /*if (track == drive->track) {*/
      /*// send sector if fpga is still asking for data - not required as checked on entry*/

         /*SPI_EnableFpga();*/
         /*SPI(0x50); // enable write*/
         /*FDD_SendSector(FDD_fBuf, sector, track, (uint8_t)(dsksync >> 8), (uint8_t)dsksync);*/
         /*SPI_DisableFpga();*/

         /*if (sector == LAST_SECTOR)*/
         /*{*/

          /*SPI_EnableFpga();*/
          /*SPI(0x51); // temp use ch1 for gap (index)*/

           /*// send gap*/
          /*i = 128;*/
          /*while (i--)*/
            /*SPI(0xAA);*/
          /*SPI_DisableFpga();*/
          /*//*/
          /*SPI_EnableFpga();*/
          /*SPI(0x50);*/
          /*// send gap*/
          /*i = GAP_SIZE-128;*/
          /*while (i--)*/
            /*SPI(0xAA);*/
          /*SPI_DisableFpga();*/
         /*}*/
    /*}*/

    /*// we are done accessing FPGA*/
    /*SPI_DisableFpga();*/

    /*// track has changed*/
    /*if (track != drive->track)*/
      /*break;*/

    /*// read request*/
    /*if (!(status & 0x08))*/
      /*break;*/

    /*sector++;*/
    /*if (sector < SECTOR_COUNT) {*/
      /*//all is good, do nothing*/
    /*} else {*/
      /*// go to the start of current track*/
      /*sector = 0;*/
      /*FF_Seek(drive->fSource, ((512*11) * drive->track), FF_SEEK_SET);*/
    /*}*/
    /*// remember current sector and cluster*/
    /*drive->sector_offset = sector;*/
  /*}*/
/*}*/
/*}}}*/

/*{{{*/
/*uint8_t FDD_FindSync(fddTYPE *drive)*/
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
/*void FDD_WriteTrack(fddTYPE *drive)*/
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

uint8_t FDD_WaitStat(uint8_t mask, uint8_t wanted)
{
  uint8_t  stat;
  uint32_t timeout = Timer_Get(500);      // 500 ms timeout
  do {
    stat = FDD_FileIO_GetStat();

    if (Timer_Check(timeout)) {
      WARNING("FDD_WaitStat:Waitstat timeout.");
      return (1);
    }
  } while ((stat & mask) != wanted);
  return (0);
}

void FDD_Handle_0x00(uint8_t status) // generic
{
  uint8_t  dir  = (status >> 2) & 0x01; // high is write
  uint8_t  chan = (status >> 4) & 0x03;

  uint16_t size = 0;
  uint16_t cur_size = 0;
  uint32_t addr = 0;
  fddTYPE* drive = NULL;
  uint32_t act_size = 0;

  SPI_EnableFpga();
  SPI(FILEIO_FD_CMD_R | 0x0);
  SPI(0x00); // dummy
  size  =  SPI(0);
  size |= (SPI(0) << 8);
  addr  =  SPI(0);
  addr |= (SPI(0) << 8);
  addr |= (SPI(0) << 16);
  addr |= (SPI(0) << 24);
  SPI_DisableFpga();

  FDD_FileIO_WriteStat(FD_STAT_REQ_ACK); // ack

  // move to validate chan function
  // point to drive struct
  if (!FDD_Inserted(chan)) {
    DEBUG(1,"FDD:request chan %d not mounted", chan);
    FDD_FileIO_WriteStat(FD_STAT_TRANS_ACK_ABORT_ERR); // err
    return;
  }
  drive = &fdf[chan];

  if (!drive->fSource) {
    DEBUG(1,"FDD:request chan %d not open", chan);
    FDD_FileIO_WriteStat(FD_STAT_TRANS_ACK_ABORT_ERR); // err
    return;
  }
  // end validate chan
  if (size > 0x2000) {
    DEBUG(1,"FDD:warning large size request:%04X",size);
  }

  if (FF_Seek(drive->fSource, addr, FF_SEEK_SET)) {
    DEBUG(1,"FDD:seek error");
    FDD_FileIO_WriteStat(FD_STAT_TRANS_ACK_SEEK_ERR); // err
    return;
  }

  while (size) {
    cur_size = size;
    if (cur_size > FDD_BUF_SIZE) cur_size = FDD_BUF_SIZE;

    /*DEBUG(1,"FDD:handle Chan:%02X Addr:%08X Size:%04X",chan,addr,size);*/

    // align to 512 byte boundaries if possible
    uint32_t addr_offset = addr & 0x1FF;
    if (addr_offset != 0) {
      /*DEBUG(1,"FDD:non-aligned:%08X",addr);*/
      addr_offset = 0x200 - addr_offset;
      if (cur_size > addr_offset) {
        cur_size = addr_offset;
        /*DEBUG(1,"FDD:new size:%04X",cur_size);*/
      }
    }

    if (dir) { // write
      // request should not be asserted if data is not ready

      SPI_EnableFpga();
      SPI(FILEIO_FD_FIFO_R);
      SPI_ReadBufferSingle(FDD_fBuf, cur_size);
      SPI_DisableFpga();

      /*DumpBuffer(FDD_fBuf,cur_size);*/

      act_size = FF_Write(drive->fSource, cur_size, 1, FDD_fBuf);
      if (act_size != cur_size) {
        DEBUG(1,"FDD:!! Write Fail!!");
        FDD_FileIO_WriteStat(FD_STAT_TRANS_ACK_ABORT_ERR); // truncated
        return;
      }

    } else {

      // enough faffing, do the read
      act_size = FF_Read(drive->fSource, cur_size, 1, FDD_fBuf);
      /*DEBUG(1,"FDD:bytes read:%04X",act_size);*/
      /*DumpBuffer(FDD_fBuf,cur_size);*/

      SPI_EnableFpga();
      SPI(FILEIO_FD_FIFO_W);
      SPI_WriteBufferSingle(FDD_fBuf, act_size);
      SPI_DisableFpga();

      if (act_size != cur_size) {
        FDD_FileIO_WriteStat(FD_STAT_TRANS_ACK_TRUNC_ERR); // truncated
        return;
      }
    }

    addr += cur_size;
    size -= cur_size;

    // check to see if we can send/rx more
    // NOTE, this stalls the GUI so not recommeded to use the flow control here
    if (size) { // going again
      if (dir) { // write to arm
        if (FDD_WaitStat(FILEIO_FD_REQ_OK_TO_ARM, FILEIO_FD_REQ_OK_TO_ARM)) {
          FDD_FileIO_WriteStat(FD_STAT_TRANS_ACK_ABORT_ERR); // err
          return;
        }
      }
      else {
        if (FDD_WaitStat(FILEIO_FD_REQ_OK_FM_ARM, FILEIO_FD_REQ_OK_FM_ARM)) {
          FDD_FileIO_WriteStat(FD_STAT_TRANS_ACK_ABORT_ERR); // err
          return;
        }
      }
    }
    // end of wait
  }
  FDD_FileIO_WriteStat(FD_STAT_TRANS_ACK_OK); // ok
}

void FDD_Handle_0x01(uint8_t status) // amiga
{
  uint8_t  dir     = (status >> 2) & 0x01; // high is write
  uint8_t  chan    = (status >> 4) & 0x03;

  uint8_t  sector  = 0;
  uint8_t  track   = 0;
  uint16_t dsksync = 0;
  uint32_t offset  = 0;
  uint32_t i       = 0;

  fddTYPE* drive   = NULL;

  if (FDD_DEBUG)
    DEBUG(1,"FDD:Amiga handler");

  do {

  SPI_EnableFpga();
  SPI(FILEIO_FD_CMD_R | 0x0);
  SPI(0x00); // dummy

  SPI(0);  // spare
  track    = SPI(0);
  dsksync  = SPI(0) << 8;
  dsksync |= SPI(0);
  SPI_DisableFpga();


  // move drive validation out of request loop
  if (!FDD_Inserted(chan)) {
    DEBUG(1,"FDD:request chan %d not mounted", chan);
    FDD_FileIO_WriteStat(FD_STAT_TRANS_ACK_ABORT_ERR); // err
    return;
  }
  drive = &fdf[chan];

  if (!drive->fSource) {
    DEBUG(1,"FDD:request chan %d not open", chan);
    FDD_FileIO_WriteStat(FD_STAT_TRANS_ACK_ABORT_ERR); // err
    return;
  }

  if (track >= drive->tracks) {
    ERROR("Illegal track %u read!", drive->track);
    track = drive->tracks - 1;
  }

  sector = drive->cur_sector;
  if (drive->cur_sector == LAST_SECTOR)
    drive->cur_sector=0;
  else
    drive->cur_sector++;

  FDD_FileIO_WriteStat(FD_STAT_REQ_ACK); // ack

  // workaround for Copy Lock in Wiz'n'Liz and North&South (might brake other games)
  if (dsksync == 0x0000 || dsksync == 0x8914 || dsksync == 0xA144)
    dsksync = 0x4489;

  // North&South: $A144
  // Wiz'n'Liz (Copy Lock): $8914
  // Prince of Persia: $4891
  // Commando: $A245

  if (FDD_DEBUG)
    DEBUG(1,"FDD:handle Chan:%02X Dsksync:%04X Track:%02X Sector:%01X",chan,dsksync,track,sector);

  // sector size hard coded as 512 bytes
  offset  = (512*11) * track;
  offset += (sector<<9);

  if (FF_Seek(drive->fSource, offset, FF_SEEK_SET)) {
    DEBUG(1,"FDD:seek error");
    FDD_FileIO_WriteStat(FD_STAT_TRANS_ACK_SEEK_ERR); // err
    return;
  }
  FF_Read(drive->fSource, 512, 1, FDD_fBuf);

  // send sector
  SPI_EnableFpga();
  SPI(FILEIO_FD_FIFO_W);
  FDD_SendAmigaSector(FDD_fBuf, sector, track, (uint8_t)(dsksync >> 8), (uint8_t)dsksync);
  SPI_DisableFpga();

  if (sector == LAST_SECTOR)
  {

    // send gap

    SPI_EnableFpga();
    SPI(FILEIO_FD_FIFO_W | 0x1);
    SPI(0xAA);
    SPI(0xAA);
    SPI_DisableFpga();

    SPI_EnableFpga();
    SPI(FILEIO_FD_FIFO_W);
    i = GAP_SIZE-2;
    while (i--)
      SPI(0xAA);
    SPI_DisableFpga();
  }

  FDD_FileIO_WriteStat(FD_STAT_TRANS_ACK_OK); // ok

  status = FDD_FileIO_GetStat();
  } while (status & FILEIO_FD_REQ_ACT);

}

void FDD_Handle(void)
{
  uint8_t status = FDD_FileIO_GetStat();

  if (status & FILEIO_FD_REQ_ACT) { // FF request, move to header
    ACTLED_ON;

    switch (cha_driver_type) {
      case 0x0: // generic
        FDD_Handle_0x00(status);
        break;
      case 0x1: // amiga
        FDD_Handle_0x01(status);
        break;
      MSG_warning("Unknown FDD request type.");
    } // end of case

  }
  ACTLED_OFF;
}

void FDD_UpdateDriveStatus(void)
{
  uint8_t status = 0;
  for (int i=0; i<CHA_MAX_NUM; ++i) {
    if (fdf[i].status & FD_INSERTED) status |= (0x01<<i);
    if (fdf[i].status & FD_WRITABLE) status |= (0x10<<i);
  }
  DEBUG(1,"FDD:update status %02X",status);
  OSD_ConfigSendFileIO_CHA(status);
}

//
// Generic
//
void FDD_InsertParse_Generic(fddTYPE *drive)
{
  drive->tracks = 1;
}

void FDD_InsertParse_ADF(fddTYPE *drive)
{
  uint16_t tracks;
  tracks = drive->fSource->Filesize / (512*11);
  if (tracks > FD_MAX_TRACKS) {
    MSG_warning("UNSUPPORTED ADF SIZE!!! Too many tracks: %lu", tracks);
    tracks = FD_MAX_TRACKS;
  }
  drive->tracks = (uint16_t)tracks;
}

//
// Core setup
//
void FDD_InsertInit_0x00(uint8_t drive_number)
{
  fddTYPE* drive = &fdf[drive_number];
  uint32_t size = drive->fSource->Filesize;

  SPI_EnableFpga();
  SPI(FILEIO_FD_STAT_W);
  SPI(0x00); // clear status
  SPI_DisableFpga();

  // select drive
  SPI_EnableFpga();
  SPI(FILEIO_FD_CMD_W | 0x0);
  SPI(drive_number);
  SPI_DisableFpga();

  // write file size
  SPI_EnableFpga();
  SPI(FILEIO_FD_CMD_W | 0x4);
  SPI((uint8_t)(size      ));
  SPI((uint8_t)(size >>  8));
  SPI((uint8_t)(size >> 16));
  SPI((uint8_t)(size >> 24));
  SPI_DisableFpga();

}

void FDD_InsertInit_0x01(uint8_t drive_number)
{

}

void FDD_Insert(uint8_t drive_number, char *path)
{
  DEBUG(1,"attempting to insert floppy <%d> : <%s> ", drive_number,path);

  if (drive_number < CHA_MAX_NUM) {
    fddTYPE* drive = &fdf[drive_number];
    drive->status = 0;
    drive->name[0] = '\0';

    drive->status |= FD_WRITABLE;
    drive->cur_sector = 0;

    // add check for file attributes first, open read only if necessary
    /*drive->fSource = FF_Open(pIoman, path, FF_MODE_READ | FF_MODE_WRITE, NULL); // will not open if file is read only*/
    drive->fSource = FF_Open(pIoman, path, FF_MODE_READ , NULL); // will not open if file is read only
    if (!drive->fSource) {
      MSG_warning("Insert Floppy:Could not open file.");
      return;
    }
    // parse depending on file type
    char* pFile_ext = GetExtension(path);
    if (strnicmp(pFile_ext, "ADF",3) == 0) {
      // adf
      FDD_InsertParse_ADF(drive);
    } else {
      // generic
      FDD_InsertParse_Generic(drive);
    }

    SPI_EnableFpga();
    SPI(FILEIO_FD_STAT_W);
    SPI(0x00); // clear status
    SPI_DisableFpga();

    // do request read to find out FDD core request type
    // any data to send to core?
    DEBUG(1,"driver type <%d>", cha_driver_type);
    switch (cha_driver_type) {
      case 0x0: FDD_InsertInit_0x00(drive_number); break;
      case 0x1: FDD_InsertInit_0x01(drive_number); break;
      MSG_warning("Unknown FDD request type.");
      return;
    }
    FileDisplayName(drive->name, MAX_DISPLAY_FILENAME, path);

    drive->status |= FD_INSERTED;
    FDD_UpdateDriveStatus();

    DEBUG(1,"Inserted floppy: <%s>", drive->name);
    DEBUG(1,"file size   : %lu (%lu kB)", drive->fSource->Filesize, drive->fSource->Filesize >> 10);
    DEBUG(1,"drive tracks: %u", drive->tracks);
    DEBUG(1,"drive status: 0x%02X", drive->status);
  }
}

void FDD_Eject(uint8_t drive_number)
{
  DEBUG(1,"Ejecting floppy <%d>", drive_number);

  if (drive_number < CHA_MAX_NUM) {
    fddTYPE* drive = &fdf[drive_number];
    FF_Close(drive->fSource);
    drive->status = 0;
  }
  FDD_UpdateDriveStatus();
}

uint8_t FDD_Inserted(uint8_t drive_number)
{
  if (drive_number < CHA_MAX_NUM) {
    return (fdf[drive_number].status & FD_INSERTED);
  } else
  return FALSE;
}

char* FDD_GetName(uint8_t drive_number)
{
  if (drive_number < CHA_MAX_NUM) {
    return (fdf[drive_number].name);
  }
  return null_string; // in stringlight.c
}

void FDD_SetDriver(uint8_t type)
{
  cha_driver_type = type;
}

void FDD_Init(void)
{
  DEBUG(1,"FDD:Init");
  uint32_t i;
  for (i=0; i<CHA_MAX_NUM; i++) {
    fdf[i].status = 0; fdf[i].fSource = NULL;
    fdf[i].name[0] = '\0';
    fdf[i].tracks = 0;
    fdf[i].cur_sector = 0;
  }
}
