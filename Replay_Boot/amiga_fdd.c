
#include "hardware.h"
#include "config.h"
#include "messaging.h"
#include "amiga_fdd.h"

const uint8_t FDD_DEBUG = 0;

uint8_t drives = 0;   // number of active drives reported by FPGA (may change only during reset)
adfTYPE df[4];        // drive 0 information structure

extern uint8_t *pFileBuf;

//
// READ
//

// sends the data in the sector buffer to the FPGA, translated into an Amiga floppy format sector
// note that we do not insert clock bits because they will be stripped by the Amiga software anyway
void FDD_SendSector(uint8_t *pData, uint8_t sector, uint8_t track, uint8_t dsksynch, uint8_t dsksyncl)
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

void FDD_SendGap(void)
{
  uint16_t i = GAP_SIZE;
  while (i--)
    SPI(0xAA);
}

// read a track from disk
void FDD_ReadTrack(adfTYPE *drive)
{ // track number is updated in drive struct before calling this function

  uint8_t  sector;
  uint8_t  status;
  uint8_t  track;
  uint16_t dsksync;
  uint16_t dsklen;
  uint32_t offset;

  if (drive->track >= drive->tracks) {
    ERROR("Illegal track %u read!", drive->track);
    drive->track = drive->tracks - 1;
  }

  // display track number: cylinder & head
  if (FDD_DEBUG)
    DEBUG(1,"#%u", drive->track);

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

  SPI_EnableFpga();
  status   = SPI(0); // read request signal
  track    = SPI(0); // track number (cylinder & head)
  dsksync  = SPI(0) << 8; // disk sync high byte
  dsksync |= SPI(0); // disk sync low byte
  dsklen   = SPI(0) << 8 & 0x3F00; // msb of mfm words to transfer
  dsklen  |= SPI(0); // lsb of mfm words to transfer
  SPI_DisableFpga();

  if (track >= drive->tracks)
    track = drive->tracks - 1;

  if (FDD_DEBUG)
    DEBUG(1,"#(%u,%04X)", status >> 6, dsksync);

  while (1) {
    // note read moves on file pointer automatically
    FF_Read(drive->fSource, FS_FILEBUF_SIZE, 1, pFileBuf);

    SPI_EnableFpga();

    // check if FPGA is still asking for data
    status   = SPI(0); // read request signal
    track    = SPI(0); // track number (cylinder & head)
    dsksync  = SPI(0) << 8; // disk sync high byte
    dsksync |= SPI(0); // disk sync low byte
    dsklen   = SPI(0) << 8 & 0x3F00; // msb of mfm words to transfer
    dsklen  |= SPI(0); // lsb of mfm words to transfer

    if (track >= drive->tracks)
      track = drive->tracks - 1;

    // workaround for Copy Lock in Wiz'n'Liz and North&South (might brake other games)
    if (dsksync == 0x0000 || dsksync == 0x8914 || dsksync == 0xA144)
      dsksync = 0x4489;

    // North&South: $A144
    // Wiz'n'Liz (Copy Lock): $8914
    // Prince of Persia: $4891
    // Commando: $A245

    if (FDD_DEBUG)
      DEBUG(2,"#%X:%04X", sector, dsklen);

    // some loaders stop dma if sector header isn't what they expect
    // because we don't check dma transfer count after sending a word
    // the track can be changed while we are sending the rest of the previous sector
    // in this case let's start transfer from the beginning
    if (track == drive->track) {
      // send sector if fpga is still asking for data
       if (status & CMD_RDTRK) {
         FDD_SendSector(pFileBuf, sector, track, (uint8_t)(dsksync >> 8), (uint8_t)dsksync);
         if (sector == LAST_SECTOR)
           FDD_SendGap();
       }
    }

    // we are done accessing FPGA
    SPI_DisableFpga();

    // track has changed
    if (track != drive->track)
      break;

    // read dma request
    if (!(status & CMD_RDTRK))
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

uint8_t FDD_FindSync(adfTYPE *drive)
// reads data from fifo till it finds sync word or fifo is empty and dma inactive (so no more data is expected)
{
  uint8_t  c1, c2, c3, c4;
  uint16_t n;

  while (1) {
    SPI_EnableFpga();
    c1 = SPI(0); // write request signal
    c2 = SPI(0); // track number (cylinder & head)
    if (!(c1 & CMD_WRTRK))
      break;
    if (c2 != drive->track)
      break;
    SPI(0); // disk sync high byte
    SPI(0); // disk sync low byte
    c3 = SPI(0) & 0xBF; // msb of mfm words to transfer
    c4 = SPI(0);        // lsb of mfm words to transfer

    if (c3 == 0 && c4 == 0)
      break;

    n = ((c3 & 0x3F) << 8) + c4;

    while (n--) {
      c3 = SPI(0);
      c4 = SPI(0);
      if (c3 == 0x44 && c4 == 0x89) {
        SPI_DisableFpga();
        if (FDD_DEBUG)
          DEBUG(1,"#SYNC");
        return 1;
      }
    }
    SPI_DisableFpga();
  }
  SPI_DisableFpga();
  return 0;
}

uint8_t FDD_GetHeader(uint8_t *pTrack, uint8_t *pSector)
// this function reads data from fifo till it finds sync word or dma is inactive
{
  uint8_t c, c1, c2, c3, c4;
  uint8_t i;
  uint8_t checksum[4];
  uint8_t error = 0;

  while (1) {
    SPI_EnableFpga();
    c1 = SPI(0); // write request signal
    c2 = SPI(0); // track number (cylinder & head)
    if (!(c1 & CMD_WRTRK))
        break;
    SPI(0); // disk sync high byte
    SPI(0); // disk sync low byte
    c3 = SPI(0); // msb of mfm words to transfer
    c4 = SPI(0); // lsb of mfm words to transfer

    if ((c3 & 0x3F) != 0 || c4 > 24) {// remaining header data is 25 mfm words
      c1 = SPI(0); // second sync lsb
      c2 = SPI(0); // second sync msb
      if (c1 != 0x44 || c2 != 0x89) {
        error = 21;
        ERROR("Second sync word missing!");
        break;
      }

      c = SPI(0);
      checksum[0] = c;
      c1 = (c & 0x55) << 1;
      c = SPI(0);
      checksum[1] = c;
      c2 = (c & 0x55) << 1;
      c = SPI(0);
      checksum[2] = c;
      c3 = (c & 0x55) << 1;
      c = SPI(0);
      checksum[3] = c;
      c4 = (c & 0x55) << 1;

      c = SPI(0);
      checksum[0] ^= c;
      c1 |= c & 0x55;
      c = SPI(0);
      checksum[1] ^= c;
      c2 |= c & 0x55;
      c = SPI(0);
      checksum[2] ^= c;
      c3 |= c & 0x55;
      c = SPI(0);
      checksum[3] ^= c;
      c4 |= c & 0x55;

      if (c1 != 0xFF) // always 0xFF
         error = 22;
      else if (c2 > 159) // Track number (0-159)
         error = 23;
      else if (c3 > 10) // Sector number (0-10)
         error = 24;
      else if (c4 > 11 || c4 == 0) // Number of sectors to gap (1-11)
         error = 25;

      if (error) {
        ERROR("Wrong header: %u.%u.%u.%u", c1, c2, c3, c4);
        break;
      }
      *pTrack = c2;
      *pSector = c3;

      for (i = 0; i < 8; i++) {
        checksum[0] ^= SPI(0);
        checksum[1] ^= SPI(0);
        checksum[2] ^= SPI(0);
        checksum[3] ^= SPI(0);
      }
      checksum[0] &= 0x55;
      checksum[1] &= 0x55;
      checksum[2] &= 0x55;
      checksum[3] &= 0x55;

      c1 = (SPI(0) & 0x55) << 1;
      c2 = (SPI(0) & 0x55) << 1;
      c3 = (SPI(0) & 0x55) << 1;
      c4 = (SPI(0) & 0x55) << 1;

      c1 |= SPI(0) & 0x55;
      c2 |= SPI(0) & 0x55;
      c3 |= SPI(0) & 0x55;
      c4 |= SPI(0) & 0x55;

      if (c1 != checksum[0] || c2 != checksum[1] || c3 != checksum[2] || c4 != checksum[3]) {
        error = 26;
         break;
      }

      SPI_DisableFpga();
      return 1;
    } else if ((c3 & 0x80) == 0) {
      // not enough data for header and write dma is not active
      error = 20;
      break;
    }

    SPI_DisableFpga();
  }
  SPI_DisableFpga();

  if (error) {
    ERROR("GetHeader: error %u", error);
  }

  return 0;
}

uint8_t FDD_GetData(void)
{
  uint8_t  c, c1, c2, c3, c4;
  uint8_t  i;
  uint8_t  *p;
  uint16_t n;
  uint8_t  checksum[4];

  uint8_t error = 0;
  while (1) {
    SPI_EnableFpga();
    c1 = SPI(0); // write request signal
    c2 = SPI(0); // track number (cylinder & head)
    if (!(c1 & CMD_WRTRK))
      break;
    SPI(0); // disk sync high byte
    SPI(0); // disk sync low byte
    c3 = SPI(0); // msb of mfm words to transfer
    c4 = SPI(0); // lsb of mfm words to transfer

    n = ((c3 & 0x3F) << 8) + c4;

    if (n >= 0x204) {
      c1 = (SPI(0) & 0x55) << 1;
      c2 = (SPI(0) & 0x55) << 1;
      c3 = (SPI(0) & 0x55) << 1;
      c4 = (SPI(0) & 0x55) << 1;

      c1 |= SPI(0) & 0x55;
      c2 |= SPI(0) & 0x55;
      c3 |= SPI(0) & 0x55;
      c4 |= SPI(0) & 0x55;

      checksum[0] = 0;
      checksum[1] = 0;
      checksum[2] = 0;
      checksum[3] = 0;

      // odd bits of data field
      i = 128;
      p = pFileBuf;
      do {
        c = SPI(0);
        checksum[0] ^= c;
        *p++ = (c & 0x55) << 1;
        c = SPI(0);
        checksum[1] ^= c;
        *p++ = (c & 0x55) << 1;
        c = SPI(0);
        checksum[2] ^= c;
        *p++ = (c & 0x55) << 1;
        c = SPI(0);
        checksum[3] ^= c;
        *p++ = (c & 0x55) << 1;
      }
      while (--i);

      // even bits of data field
      i = 128;
      p = pFileBuf;
      do {
        c = SPI(0);
        checksum[0] ^= c;
        *p++ |= c & 0x55;
        c = SPI(0);
        checksum[1] ^= c;
        *p++ |= c & 0x55;
        c = SPI(0);
        checksum[2] ^= c;
        *p++ |= c & 0x55;
        c = SPI(0);
        checksum[3] ^= c;
        *p++ |= c & 0x55;
      }

      while (--i);
       checksum[0] &= 0x55;
       checksum[1] &= 0x55;
       checksum[2] &= 0x55;
       checksum[3] &= 0x55;

       if (c1 != checksum[0] || c2 != checksum[1] || c3 != checksum[2] || c4 != checksum[3]) {
         error = 29;
         break;
       }

       SPI_DisableFpga();
       return 1;
     } else if ((c3 & 0x80) == 0) {
       // not enough data in fifo and write dma is not active
       error = 28;
       break;
     }

    SPI_DisableFpga();
  }
  SPI_DisableFpga();

  if (error) {
    ERROR("GetData: error %u", error);
  }

  return 0;
}

void FDD_WriteTrack(adfTYPE *drive)
{

  uint8_t rx_track  = 0;
  uint8_t rx_sector = 0;
  uint8_t sector;

  uint8_t error = 0;

  // setting file pointer to begining of current track
  uint32_t offset = (512*11) * drive->track;
  drive->track_prev = drive->track + 1;  // just to force next read from the start of current track*/
  drive->sector_offset = 0;
  sector = 0;

  FF_Seek(drive->fSource, offset, FF_SEEK_SET);

  if (FDD_DEBUG)
    DEBUG(1,"*%u", drive->track);

  while (FDD_FindSync(drive)) {
    if (FDD_GetHeader(&rx_track, &rx_sector)) {
      if (rx_track == drive->track) {
        while (sector != rx_sector) {

          if (sector < rx_sector) {
            // next sector
            FF_Seek(drive->fSource, 512, FF_SEEK_CUR);
            sector++;
          } else {
            // set to start of track
            offset = (512*11) * drive->track;
            drive->sector_offset = 0;
            sector = 0;
            FF_Seek(drive->fSource, offset, FF_SEEK_SET);
          }
        }

        if (FDD_GetData()) {
          if (drive->status & DSK_WRITABLE)
            FF_Write(drive->fSource, FS_FILEBUF_SIZE, 1, pFileBuf);
          else {
            error = 30;
            ERROR("Write attempt to protected disk!");
          }
        }
      } else {
        error = 27; //track number reported in sector header is not the same as current drive track
      }
    }

    if (error) {
      ERROR("WriteTrack: error %u", error);
    }
  }
}

void FDD_UpdateDriveStatus(void)
{
  SPI_EnableFpga();
  SPI(0x10);
  SPI(df[0].status | (df[1].status << 1) | (df[2].status << 2) | (df[3].status << 3));
  SPI_DisableFpga();
}

void FDD_Handle(uint8_t c1, uint8_t c2)
{
  uint8_t sel;
  drives = (c1 >> 4) & 0x03; // number of active floppy drives

  if (c1 & CMD_RDTRK) {
    ACTLED_ON;
    sel = (c1 >> 6) & 0x03;
    df[sel].track = c2;
    FDD_ReadTrack(&df[sel]);
    ACTLED_OFF;
  } else if (c1 & CMD_WRTRK) {
    ACTLED_ON;
    sel = (c1 >> 6) & 0x03;
    df[sel].track = c2;
    FDD_WriteTrack(&df[sel]);
    ACTLED_OFF;
  }
}

void FDD_Init(void)
{
  uint32_t i;
  for (i=0; i<4; i++) {
    df[i].status = 0; df[i].fSource = NULL;
  }
}
