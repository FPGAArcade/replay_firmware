/*
 WWW.FPGAArcade.COM

 REPLAY Retro Gaming Platform
 No Emulation No Compromise

 All rights reserved
 Mike Johnson Wolfgang Scherr

 SVN: $Id:

--------------------------------------------------------------------

 Redistribution and use in source and synthezised forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

 Redistributions in synthesized form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.

 Neither the name of the author nor the names of other contributors may
 be used to endorse or promote products derived from this software without
 specific prior written permission.

 THIS CODE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.

 You are responsible for any legal issues arising from your use of this code.

 The latest version of this file can be found at: www.FPGAArcade.com

 Email support@fpgaarcade.com
*/

#include "fileio_drv.h"
#include "hardware.h"
#include "messaging.h"

const uint8_t DRV01_DEBUG = 0;

// to do, tidy constants
#define ADF_MAX_TRACKS (83*2)

// all these in BYTES
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

#define DRV01_ADF_WRITE_LEN 540 // 512 + 28 WORDS from after 2nd sync to end of sector

typedef enum {
    XXX, // unsupported
    ADF,
    SCP
} drv01_format_t;

typedef struct
{
    uint8_t id[3];                              // "SCP"
    uint8_t version;                            // Version<<4|Revision
    uint8_t disk_type;                          // Class<<4|Sub class
    uint8_t number_of_revolutions;
    uint8_t start_track;
    uint8_t end_track;
    uint8_t flags;
    uint8_t cell_encoding;
    uint8_t resv_0;
    uint8_t resv_1;
    uint32_t checksum;
} drv01_scp_header_t;

typedef struct
{
    uint32_t index_time;
    uint32_t track_length;
    uint32_t track_offset;
} drv01_scp_track_tlo_t;

typedef struct
{
    uint8_t id[3];                          // "TRK"
    uint8_t track_number;
    drv01_scp_track_tlo_t tlo[5]; // one for each
} drv01_scp_track_header_t;

typedef struct
{
    drv01_format_t  format;
    //
    uint32_t file_size;
    uint16_t total_tracks;
    uint8_t  cur_sector;
    uint8_t  scp_start_track;
    uint8_t  scp_end_track;
    uint8_t  scp_revolutions;
    //
    uint8_t  scp_cur_track;
    uint32_t scp_cur_track_header_addr;    // current track header address in file. 0 = track does not exist
    //
    drv01_scp_track_header_t scp_cur_track_header;
    uint8_t  scp_cur_track_rev;
    uint32_t scp_cur_track_offset;  // current offset from start of track flux data
    //
} drv01_desc_t;


inline uint8_t MFMDecode(uint8_t* odd, uint8_t* even)
{

  uint8_t r  = ((*odd ) & 0x55) << 1;
          r |= ((*even) & 0x55);
  return r;
}

inline uint8_t MFMEncode(uint8_t d, uint8_t l)
{
  // strips out even bits and replaces with clock bits
  uint8_t r = d & 0x55;
  if (! ((r & 0x01) || (r & 0x04))) r |= 0x02; // clock bit 1
  if (! ((r & 0x04) || (r & 0x10))) r |= 0x08; // clock bit 3
  if (! ((r & 0x10) || (r & 0x40))) r |= 0x20; // clock bit 5
  if (! ((r & 0x40) || (l & 0x01))) r |= 0x80; // clock bit 7
  return r;
}


void FileIO_Drv01_Amiga_SendHeader(uint8_t* pBuffer, uint8_t sector, uint8_t track, uint8_t dsksynch, uint8_t dsksyncl)
{
  //DumpBuffer(pData, 64);

  uint8_t checksum[4];
  uint16_t i;
  uint8_t x;
  uint8_t *p;

  // preamble
  rSPI(0xAA);
  rSPI(0xAA);
  rSPI(0xAA);
  rSPI(0xAA);

  // synchronization
  rSPI(dsksynch);
  rSPI(dsksyncl);
  rSPI(dsksynch);
  rSPI(dsksyncl);

  // odd bits of header
  x = MFMEncode(0xFF, dsksyncl);
  checksum[0] = x; rSPI(x);

  x = MFMEncode(track >> 1, x);
  checksum[1] = x; rSPI(x);

  x = MFMEncode(sector >> 1, x);
  checksum[2] = x; rSPI(x);

  x = MFMEncode((11 - sector) >> 1, x);
  checksum[3] = x; rSPI(x);

  // even bits of header
  x = MFMEncode(0xFF, x);
  checksum[0] ^= x; rSPI(x);

  x = MFMEncode(track, x);
  checksum[1] ^= x; rSPI(x);

  x = MFMEncode(sector, x);
  checksum[2] ^= x; rSPI(x);

  x = MFMEncode((11 - sector), x);
  checksum[3] ^= x; rSPI(x);

  // first clock bit here may be wrong ...
  // sector label and reserved area (changes nothing to checksum)
  i = 0x20;
  while (i--)
    rSPI(0xAA);

  // send header checksum
  rSPI(0xAA);
  rSPI(0xAA);
  rSPI(0xAA);
  rSPI(0xAA);
  rSPI(checksum[0] | 0xAA);
  rSPI(checksum[1] | 0xAA);
  rSPI(checksum[2] | 0xAA);
  rSPI(checksum[3] | 0xAA);

  // calculate data checksum
  checksum[0] = 0;
  checksum[1] = 0;
  checksum[2] = 0;
  checksum[3] = 0;

  p = pBuffer;
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
  rSPI(0xAA);
  rSPI(0xAA);
  rSPI(0xAA);
  rSPI(0xAA);
  rSPI(checksum[0] | 0xAA);
  rSPI(checksum[1] | 0xAA);
  rSPI(checksum[2] | 0xAA);
  rSPI(checksum[3] | 0xAA);

}

void FileIO_Drv01_ADF_Write(uint8_t ch, fch_t* pDrive, uint8_t* pBuffer, uint8_t* write_state)
{
  uint8_t rxbuf[DRV01_ADF_WRITE_LEN*2]; // this is in addition to pBuffer. Could optimize

  uint8_t  track   = 0;
  uint16_t dsksync = 0;
  uint16_t rx      = 0;
  uint16_t reqsize = 0; // -1
  uint32_t offset  = 0;
  uint32_t i       = 0;
  uint32_t j       = 0;

  uint8_t p[4];  // param
  uint8_t cs[4]; // checksum

  drv01_desc_t* pDesc = pDrive->pDesc;


  SPI_EnableFileIO();
  rSPI(FCH_CMD(ch,FILEIO_FCH_CMD_CMD_R | 0x0));
  rSPI(0x00); // dummy
             rSPI(0);  // spare
  track    = rSPI(0);  //
  dsksync  = rSPI(0) << 8;
  dsksync |= rSPI(0);
  reqsize  = rSPI(0) << 8;
  reqsize |= rSPI(0);
  SPI_DisableFileIO();

  reqsize++; // add one as reqsize is -1

  FileIO_FCh_WriteStat(ch, DRV01_STAT_REQ_ACK); // ack

  if (DRV01_DEBUG)
    DEBUG(1,"Drv01:Process Write Ch%u Dsksync:%04X Track:%u ReqSize:%u",ch, dsksync,track, reqsize);

  // add check for write protect (filesys will bounce it)

  uint8_t sync = 0;
  uint32_t count = 0;
  switch (*write_state) {
    case 0 : // find sync word
      SPI_EnableFileIO();
      rSPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_R));
      do {
        rx  = rSPI(0) << 8;
        rx |= rSPI(0);
        count ++;
        reqsize --; // here for debug
        if (rx == 0x4489) sync = 1;
      } while (reqsize && !sync);
      SPI_DisableFileIO();
      //
      if (sync) *write_state = 1;
      // store track?

      if (DRV01_DEBUG) DEBUG(1,"Drv01:sync %u count %u",sync,count);
      break;

    // 25 words for header (2nd dskync to end header cs)
    // 29 words for all header (2nd dskync to end data cs)
    // x200 for sector

    case 1 : // check header

      if (reqsize < DRV01_ADF_WRITE_LEN) { // note, in WORDs 1024+58=1082 bytes
        if (DRV01_DEBUG) DEBUG(1,"Drv01:Write underrun");
        *write_state = 0;
        break;
      }

      // sanity check, look for second sync word
      SPI_EnableFileIO();
      rSPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_R));
      rx  = rSPI(0) << 8; rx |= rSPI(0);
      SPI_DisableFileIO();

      if (rx != 0x4489) {
        WARNING("Drv01:W 2nd sync word missing");
        *write_state = 0;
        break;
      }

      // read buffer
      SPI_EnableFileIO();
      rSPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_R));
      SPI_ReadBufferSingle(rxbuf, DRV01_ADF_WRITE_LEN * 2); // *2 to get bytes
      SPI_DisableFileIO();
      /*DumpBuffer(rxbuf,DRV01_ADF_WRITE_LEN * 2);*/

      // note, start of rxbuf is offset 0x08 in the header
      // extract sector/track params
      for (i=0; i<4; ++i) p[i] = MFMDecode(rxbuf+i, rxbuf+i+4);
      if (DRV01_DEBUG) DEBUG(1,"Drv01:Write param %u.%u.%u.%u",p[0], p[1], p[2], p[3]);

      // p[0] always 0xFF, p[1] track, p[2] sector (0-10) p[3] number to gap (1-11)
      if ((p[0] != 0xFF) || (p[1] >= pDesc->total_tracks) || (p[2] > 10) || (p[3] > 11) || (p[3] ==0)) {
        WARNING("Drv01:W Param err %u.%u.%u.%u",p[0], p[1], p[2], p[3]);
        *write_state = 0;
        break;
      }
      if (p[1] != track)  {
        WARNING("Drv01:W Track param mismatch");
        *write_state = 0;
        break;
      }

      // sector size hard coded as 512 bytes
      offset  = (512*11) * track;
      offset += (p[2]<<9);

      // check header checksum
      for (i=0; i<4; ++i) cs[i] = 0;

      for (j=0; j<10; ++j) {
        for (i=0; i<4; ++i) {
          cs[i] ^= rxbuf[i+j*4];
        }
      }
      for (i=0; i<4; ++i) cs[i] &= 0x55;
      // check header checksum
      for (i=0; i<4; ++i) p[i] = MFMDecode(rxbuf+i+0x28, rxbuf+i+0x2C);

      if (p[0] != cs[0] || p[1] != cs[1] || p[2] != cs[2] || p[3] != cs[3]) {
        WARNING("Drv01:W Header checksum error");
        *write_state = 0;
        break;
      };

      // check data checksum
      for (i=0; i<4; ++i) cs[i] = 0;

      for (j=0; j<0x100; ++j) {
        for (i=0; i<4; ++i) {
          cs[i] ^= rxbuf[0x38+i+j*4];
        }
      }
      for (i=0; i<4; ++i) cs[i] &= 0x55;

      // get data checksum
      for (i=0; i<4; ++i) p[i] = MFMDecode(rxbuf+i+0x30, rxbuf+i+0x34);

      if (p[0] != cs[0] || p[1] != cs[1] || p[2] != cs[2] || p[3] != cs[3]) {
        WARNING("Drv01:W Data checksum error");
        *write_state = 0;
        break;
      };

      if (pDrive->status & FILEIO_STAT_READONLY_OR_PROTECTED) {
        WARNING("Drv01:W Read only disk!");
        *write_state = 0;
        break;
      }

      // decode buffer
      for (i=0; i<0x200; ++i) rxbuf[i+0x38] = MFMDecode(rxbuf+i+0x38, rxbuf+i+0x238);
      /*DumpBuffer(rxbuf+0x38,0x200);*/

      if (FF_Seek(pDrive->fSource, offset, FF_SEEK_SET)) {
        WARNING("Drv01:Seek error");
        *write_state = 0;
        break;
      }

      // write it
      if (FF_Write(pDrive->fSource, 0x200, 1, rxbuf+0x38) != 0x200) {
        WARNING("Drv01:!! Write Fail!!");
      }
      // return to seek mode
      *write_state = 0;
      break;

    default :
      *write_state = 0;
  }

  // signal transfer done
  FileIO_FCh_WriteStat(ch, DRV01_STAT_TRANS_ACK_OK); // no error reporting
}

void FileIO_Drv01_ADF_Read(uint8_t ch, fch_t* pDrive, uint8_t* pBuffer)
{
  uint8_t  sector  = 0;
  uint8_t  track   = 0;
  uint16_t dsksync = 0;
  uint32_t offset  = 0;
  uint32_t i       = 0;

  drv01_desc_t* pDesc = pDrive->pDesc;

  SPI_EnableFileIO();
  rSPI(FCH_CMD(ch,FILEIO_FCH_CMD_CMD_R | 0x0));
  rSPI(0x00); // dummy

             rSPI(0);  // spare
  track    = rSPI(0);
  dsksync  = rSPI(0) << 8;
  dsksync |= rSPI(0);
  SPI_DisableFileIO();

  FileIO_FCh_WriteStat(ch, DRV01_STAT_REQ_ACK); // ack

  if (track >= pDesc->total_tracks) {
    DEBUG(0,"Illegal track %u read!", track); // no warning, happens quite a bit
    track = pDesc->total_tracks - 1;
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
    DEBUG(1,"Drv01:Process ADF Read Ch%u Dsksync:%04X Track:%u Sector:%01X",ch, dsksync,track,sector);

  // sector size hard coded as 512 bytes
  offset  = (512*11) * track;
  offset += (sector<<9);

  if (FF_Seek(pDrive->fSource, offset, FF_SEEK_SET)) {
    DEBUG(1,"Drv01:seek error");
    FileIO_FCh_WriteStat(ch, DRV01_STAT_TRANS_ACK_SEEK_ERR); // err
    return;
  }
  FF_Read(pDrive->fSource, 512, 1, pBuffer);

  // send sector
  SPI_EnableFileIO();
  rSPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_W));
  FileIO_Drv01_Amiga_SendHeader(pBuffer, sector, track, (uint8_t)(dsksync >> 8), (uint8_t)dsksync);
  SPI_DisableFileIO();

  // odd bits of data
  SPI_EnableFileIO();
  rSPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_W | 0x2));
  SPI_WriteBufferSingle(pBuffer, DATA_SIZE / 2);
  SPI_DisableFileIO();

  // even bits of data field
  SPI_EnableFileIO();
  rSPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_W | 0x3));
  SPI_WriteBufferSingle(pBuffer, DATA_SIZE / 2);
  SPI_DisableFileIO();

  // odd bits of data field
  /*i = DATA_SIZE / 2;*/
  /*p = pBuffer;*/
  /*while (i--)*/
    /*rSPI(*p++ >> 1 | 0xAA);*/

  // even bits of data field
  /*i = DATA_SIZE / 2;*/
  /*p = pBuffer;*/
  /*while (i--)*/
    /*rSPI(*p++ | 0xAA);*/

  SPI_DisableFileIO();

  if (sector == LAST_SECTOR)
  {
    // send gap
    SPI_EnableFileIO();
    rSPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_W | 0x1));
    rSPI(0xAA);
    rSPI(0xAA);
    SPI_DisableFileIO();

    SPI_EnableFileIO();
    rSPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_W));
    i = (GAP_SIZE/2) - 1;
    while (i--)
      rSPI(0xAA);
      rSPI(0xAA);
    SPI_DisableFileIO();
  }
  // signal transfer done
  FileIO_FCh_WriteStat(ch, DRV01_STAT_TRANS_ACK_OK); // ok
}

void FileIO_Drv01_SCP_Read(uint8_t ch, fch_t *pDrive, uint8_t *pBuffer)
{
  uint8_t  track  = 0;
  uint32_t offset = 0;
  uint32_t cur_track_len = 0;
  uint32_t trans_len = 0;

  drv01_desc_t* pDesc = pDrive->pDesc;

  SPI_EnableFileIO();
  rSPI(FCH_CMD(ch,FILEIO_FCH_CMD_CMD_R | 0x0));
  rSPI(0x00); // dummy

             rSPI(0);  // spare
  track    = rSPI(0);
  SPI_DisableFileIO();
  FileIO_FCh_WriteStat(ch, DRV01_STAT_REQ_ACK); // ack

  if (DRV01_DEBUG)
    DEBUG(1,"Drv01:Process SCP Read Ch%u Track:%u",ch, track);

  if (track != pDesc->scp_cur_track) { // step

    pDesc->scp_cur_track = track; // store received track value
    // legalize
    if        (track > pDesc->scp_end_track) {
      pDesc->scp_cur_track_header_addr = 0;
    } else if (track < pDesc->scp_start_track) {
      pDesc->scp_cur_track_header_addr = 0;
    } else {
      // get track header offset
      FF_Seek(pDrive->fSource, 0x0010 + (track - pDesc->scp_start_track)*4, FF_SEEK_SET);
      FF_Read(pDrive->fSource, 4, 1, (uint8_t*)&offset);

      pDesc->scp_cur_track_header_addr = offset; // 0 if track skipped

      if (offset !=0) {
        // read track header
        FF_Seek(pDrive->fSource, offset, FF_SEEK_SET);
        FF_Read(pDrive->fSource, sizeof(drv01_scp_track_header_t), 1, (uint8_t*)&pDesc->scp_cur_track_header);
        /*DumpBuffer((uint8_t*)&pDesc->scp_cur_track_header, sizeof(drv01_scp_track_header_t));*/

        if (_strncmp((char*)&pDesc->scp_cur_track_header.id,"TRK",3)) {
          WARNING("Bad TRK header");
          pDesc->scp_cur_track_header_addr = 0;
        }

        if (pDesc->scp_cur_track_header.track_number != track) {
          WARNING("TRK header does not match current track");
          pDesc->scp_cur_track_header_addr = 0;
        }

        // ok, all good
        pDesc->scp_cur_track_rev    = 0;
        // stay at approx offset. Check if we are off the end of new track length
        if (pDesc->scp_cur_track_offset > pDesc->scp_cur_track_header.tlo[0].track_length)
          pDesc->scp_cur_track_offset = 0; // start at begining of track
      }
    }
  }
  // assume offset is valid
  if (pDesc->scp_cur_track_header_addr == 0) {
    /*DEBUG(1,"on duff track");*/
    // do nothing
  } else {
    /*DEBUG(1,"cur revolution %d", pDesc->scp_cur_track_rev + 1);*/
    /*DEBUG(1,"cur offset       %08X", pDesc->scp_cur_track_offset);*/
    /*DEBUG(1,"cur index time   %08X", pDesc->scp_cur_track_header.tlo[pDesc->scp_cur_track_rev].index_time);*/
    /*DEBUG(1,"cur track length %08X", pDesc->scp_cur_track_header.tlo[pDesc->scp_cur_track_rev].track_length);*/
    /*DEBUG(1,"cur track offset %08X", pDesc->scp_cur_track_header.tlo[pDesc->scp_cur_track_rev].track_offset);*/

    if (pDesc->scp_cur_track_offset == 0) {
      // send sync marker
      SPI_EnableFileIO();
      rSPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_W | 0x1));
      rSPI(0x00); // very short
      rSPI(0x04);
      SPI_DisableFileIO();
    }

    cur_track_len = pDesc->scp_cur_track_header.tlo[pDesc->scp_cur_track_rev].track_length; // in words
    trans_len     = cur_track_len - pDesc->scp_cur_track_offset; // words

    if (trans_len > 1024) trans_len = 1024;
    trans_len = trans_len << 1; // words to bytes
    // max burst is 2048 bytes. Note, bufer size is only 512 used for alignment

    // offset in file
    offset  = pDesc->scp_cur_track_header_addr + pDesc->scp_cur_track_header.tlo[pDesc->scp_cur_track_rev].track_offset;
    offset += (pDesc->scp_cur_track_offset << 1); // words to bytes

    // this seek will (in most cases) not move the file pointer
    FF_Seek(pDrive->fSource, offset, FF_SEEK_SET);

    // align to 512 byte boundaries if possible
    uint32_t offset_sub    = offset & 0x1FF; // aligned?
    uint32_t trans_len_sub = trans_len & 0x1FF; // aligned?

    if ((offset_sub != 0) || (trans_len_sub !=0)) {
      offset_sub = 0x200 - offset_sub;
      if (trans_len > offset_sub) {  // < 512
        trans_len = offset_sub;
      }
      trans_len &= 0xFFFE;

      FF_Read(pDrive->fSource, trans_len, 1, pBuffer);
      SPI_EnableFileIO();
      rSPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_W));
      SPI_WriteBufferSingle(pBuffer,trans_len);
      SPI_DisableFileIO();
    } else { // aligned
      FileIo_FCh_FileReadSendDirect(ch, pDrive, trans_len);
    }
    // check end of track
    pDesc->scp_cur_track_offset += (trans_len >> 1); // words
    if (pDesc->scp_cur_track_offset >= cur_track_len) {
      // end of track
      pDesc->scp_cur_track_offset =0;
      // inc revolution
      pDesc->scp_cur_track_rev++;
      if (pDesc->scp_cur_track_rev == pDesc->scp_revolutions)
        pDesc->scp_cur_track_rev = 0;
    };
  }
  // signal transfer done
  FileIO_FCh_WriteStat(ch, DRV01_STAT_TRANS_ACK_OK); // ok
}

void FileIO_Drv01_Process(uint8_t ch, fch_t handle[2][FCH_MAX_NUM], uint8_t status) // amiga
{
  // add format check...

  // file buffer
  uint8_t  fbuf[512];

  uint8_t  dir          = 0;
  uint8_t  drive_number = 0;

  uint8_t  goes = FILEIO_PROCESS_LIMIT; // limit number of requests.

  // We don't want to hang around here waiting for data, so we must remember some state
  static   uint8_t write_state = 0; // yuk
  static   uint8_t write_drive = 0; // yuk


  do {
    dir          = (status >> 2) & 0x01; // high is write
    drive_number = (status >> 4) & 0x03;

    // validate request
    if (!FileIO_FCh_GetInserted(ch, drive_number)) {
      DEBUG(1,"Drv01:Process Ch:%u Drive:%u not mounted", ch, drive_number);
      FileIO_FCh_WriteStat(ch, DRV01_STAT_REQ_ACK); // ack
      FileIO_FCh_WriteStat(ch, DRV01_STAT_TRANS_ACK_ABORT_ERR); // err
      return;
    }

    fch_t* pDrive = (fch_t*) &handle[ch][drive_number]; // get base
    drv01_desc_t* pDesc = pDrive->pDesc;

    if (dir) { // write
      if (drive_number != write_drive) // selected drive has changed
        write_state = 0;

      if (pDesc->format == (drv01_format_t)ADF)
        FileIO_Drv01_ADF_Write(ch, pDrive, fbuf, &write_state);

      write_drive = drive_number;

    } else { // read
      if (pDesc->format == (drv01_format_t)ADF)
        FileIO_Drv01_ADF_Read(ch, pDrive, fbuf);

      if (pDesc->format == (drv01_format_t)SCP)
        FileIO_Drv01_SCP_Read(ch, pDrive, fbuf);

      write_state = 0; // reset write state if we did a read
    }

    // any more to do?
    status = FileIO_FCh_GetStat(ch);
    goes --;
  } while ((status & FILEIO_REQ_ACT) && goes);

}

uint8_t FileIO_Drv01_InsertInit(uint8_t ch, uint8_t drive_number, fch_t *pDrive, char *ext)
{
  drv01_scp_header_t scp_header;

  DEBUG(1,"Drv01:InsertInit");

  pDrive->pDesc = calloc(1, sizeof(drv01_desc_t)); // 0 everything
  if (pDrive->pDesc == NULL) {
    WARNING("Drv01:Failed to allocate memory.");
    return (1);
  }

  drv01_desc_t* pDesc = pDrive->pDesc;

  pDesc->file_size = pDrive->fSource->Filesize;
  pDesc->format    = (drv01_format_t)XXX;

  if (strnicmp(ext, "ADF",3) == 0) {
    //
    pDesc->format    = (drv01_format_t)ADF;

    //
    uint16_t total_tracks = pDesc->file_size / (512*11);
    if (total_tracks > ADF_MAX_TRACKS) {
      WARNING("Bad ADF size!!!");
      WARNING("Too many tracks: %u", total_tracks);
      total_tracks = ADF_MAX_TRACKS;
    }
    pDesc->total_tracks = total_tracks;

  } else if (strnicmp(ext, "SCP",3) == 0) {
    pDesc->format    = (drv01_format_t)SCP;

    FF_Seek(pDrive->fSource, 0, FF_SEEK_SET);
    FF_Read(pDrive->fSource, sizeof(scp_header), 1, (uint8_t*)&scp_header);

    if (_strncmp((char*)&scp_header.id,"SCP",3)) {
      WARNING("Bad SCP header");
      return (1);
    }

    DEBUG(1,"Drv01:SCP header:");
    DEBUG(1,"Drv01:Version       : 0x%02X",scp_header.version);
    DEBUG(1,"Drv01:Disk type     : 0x%02X",scp_header.disk_type);
    DEBUG(1,"Drv01:Start track   : %d",scp_header.start_track);
    DEBUG(1,"Drv01:End track     : %d",scp_header.end_track);
    DEBUG(1,"Drv01:#Revolution(s): %d",scp_header.number_of_revolutions);
    DEBUG(1,"Drv01:Flags         : 0x%02X",scp_header.flags);
    DEBUG(1,"Drv01:Cell encoding : 0x%02X",scp_header.cell_encoding);

    if ( (scp_header.cell_encoding !=0) ||
         (scp_header.number_of_revolutions < 1) ||
         (scp_header.number_of_revolutions > 5) ||
         (scp_header.end_track < scp_header.start_track) ) {
      WARNING("Unsupported SCP file");
      return (1);
    }

    pDesc->total_tracks    = 1 + scp_header.end_track - scp_header.start_track;
    pDesc->scp_start_track = scp_header.start_track;
    pDesc->scp_end_track   = scp_header.end_track;
    pDesc->scp_revolutions = scp_header.number_of_revolutions;

    pDesc->scp_cur_track = 255; // illegal
    pDesc->scp_cur_track_header_addr = 0;
    pDesc->scp_cur_track_rev    = 0;
    pDesc->scp_cur_track_offset = 0;

    pDrive->status |= FILEIO_STAT_READONLY; // set readonly

  } else {
    WARNING("Drv01:Unsupported format.");
    return (1);
  }

  // send mode, 0 for ADF, 1 for SCP
  SPI_EnableFileIO();
  rSPI(FCH_CMD(ch,FILEIO_FCH_CMD_CMD_W));
  rSPI(drive_number);

  switch (pDesc->format) {
    case ADF : rSPI(0x00); break;
    case SCP : rSPI(0x01); break;
    default  : rSPI(0x00);
  }
  SPI_DisableFileIO();

  //
  DEBUG(1,"Drv01:Size   : %lu (%lu kB)", pDesc->file_size, pDesc->file_size);
  DEBUG(1,"Drv01:Tracks : %u", pDesc->total_tracks);

  return (0);
}

