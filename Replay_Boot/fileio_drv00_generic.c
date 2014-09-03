
#include "fileio_drv.h"
#include "hardware.h"
#include "messaging.h"

const uint8_t DRV00_DEBUG = 0;

#define DRV00_STAT_REQ_ACK             0x01
#define DRV00_STAT_TRANS_ACK_OK        0x02
#define DRV00_STAT_TRANS_ACK_SEEK_ERR  0x12
#define DRV00_STAT_TRANS_ACK_TRUNC_ERR 0x22
#define DRV00_STAT_TRANS_ACK_ABORT_ERR 0x42

#define DRV00_BUF_SIZE 512

typedef struct
{
    uint32_t file_size;
} drv00_desc_t;

void FileIO_Drv00_Process(uint8_t ch, fch_t handle[2][FCH_MAX_NUM], uint8_t status) // amiga
{
  // file buffer
  uint8_t  fbuf[DRV00_BUF_SIZE];

  uint8_t  dir          = 0;
  uint8_t  drive_number = 0;
  uint16_t size = 0;
  uint16_t cur_size = 0;
  uint32_t addr = 0;
  uint32_t act_size = 0;

  uint8_t  goes = FILEIO_PROCESS_LIMIT; // limit number of requests.

  do {
    dir          = (status >> 2) & 0x01; // high is write
    drive_number = (status >> 4) & 0x03;

    // validate request
    if (!FileIO_FCh_GetInserted(ch, drive_number)) {
      DEBUG(1,"Drv00:Process Ch:%d Drive:%d not mounted", ch, drive_number);
      FileIO_FCh_WriteStat(ch, DRV00_STAT_REQ_ACK); // ack
      FileIO_FCh_WriteStat(ch, DRV00_STAT_TRANS_ACK_ABORT_ERR); // err
      return;
    }

    fch_t* pDrive = (fch_t*) &handle[ch][drive_number]; // get base

    SPI_EnableFileIO();
    SPI(FCH_CMD(ch,FILEIO_FCH_CMD_CMD_R | 0x0));
    SPI(0x00); // dummy
    size  =  SPI(0);
    size |= (SPI(0) << 8);
    addr  =  SPI(0);
    addr |= (SPI(0) << 8);
    addr |= (SPI(0) << 16);
    addr |= (SPI(0) << 24);
    SPI_DisableFileIO();

    FileIO_FCh_WriteStat(ch, DRV00_STAT_REQ_ACK); // ack

    if (size > 0x2000) {
      DEBUG(1,"Drv00:warning large size request:%04X",size);
    }

    if (FF_Seek(pDrive->fSource, addr, FF_SEEK_SET)) {
      WARNING("Drv00:Seek error");
      FileIO_FCh_WriteStat(ch, DRV00_STAT_TRANS_ACK_SEEK_ERR);
      return;
    }

    while (size) {
      cur_size = size;
      if (cur_size > DRV00_BUF_SIZE) cur_size = DRV00_BUF_SIZE;

      if (DRV00_DEBUG)
        DEBUG(1,"Drv00:Process Ch%d Drive:%02X Addr:%08X Size:%04X",ch,drive_number,addr,size);

      // align to 512 byte boundaries if possible
      uint32_t addr_offset = addr & 0x1FF;
      if (addr_offset != 0) {
        if (DRV00_DEBUG) DEBUG(1,"Drv00:non-aligned:%08X",addr);
        addr_offset = 0x200 - addr_offset;
        if (cur_size > addr_offset) {
          cur_size = addr_offset;
          if (DRV00_DEBUG) DEBUG(1,"Drv00:new size:%04X",cur_size);
        }
      }

      if (dir) { // write
        // request should not be asserted if data is not ready
        // write will fail if read only
        if (pDrive->status & FILEIO_STAT_READONLY_OR_PROTECTED) {
          WARNING("Drv00:W Read only disk!");
          FileIO_FCh_WriteStat(ch, DRV00_STAT_TRANS_ACK_TRUNC_ERR); // truncated
          return;
        }

        SPI_EnableFileIO();
        SPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_R));
        SPI_ReadBufferSingle(fbuf, cur_size);
        SPI_DisableFileIO();
        /*DumpBuffer(FDD_fBuf,cur_size);*/

        act_size = FF_Write(pDrive->fSource, cur_size, 1, fbuf);
        if (act_size != cur_size) {
          WARNING("Drv00:!! Write Fail!!");
          FileIO_FCh_WriteStat(ch, DRV00_STAT_TRANS_ACK_TRUNC_ERR); // truncated
          return;
        }
      } else {
        // enough faffing, do the read
        act_size = FF_Read(pDrive->fSource, cur_size, 1, fbuf);
        if (DRV00_DEBUG) DEBUG(1,"Drv00:bytes read:%04X",act_size);
        /*DumpBuffer(FDD_fBuf,cur_size);*/

        SPI_EnableFileIO();
        SPI(FCH_CMD(ch,FILEIO_FCH_CMD_FIFO_W));
        SPI_WriteBufferSingle(fbuf, act_size);
        SPI_DisableFileIO();

        if (act_size != cur_size) {
          FileIO_FCh_WriteStat(ch, DRV00_STAT_TRANS_ACK_TRUNC_ERR); // truncated
          return;
        }
      }

      addr += cur_size;
      size -= cur_size;

      // check to see if we can send/rx more
      // NOTE, this stalls the GUI so not recommeded to use the flow control here
      if (size) { // going again
        if (dir) { // write to arm
          if (FileIO_FCh_WaitStat(ch, FILEIO_REQ_OK_TO_ARM, FILEIO_REQ_OK_TO_ARM)) {
            FileIO_FCh_WriteStat(ch, DRV00_STAT_TRANS_ACK_ABORT_ERR); // err
            return;
          }
        }
        else {
          if (FileIO_FCh_WaitStat(ch, FILEIO_REQ_OK_FM_ARM, FILEIO_REQ_OK_FM_ARM)) {
            FileIO_FCh_WriteStat(ch, DRV00_STAT_TRANS_ACK_ABORT_ERR); // err
            return;
          }
        }
      } // end of wait

    }  // end of transfer loop

    // signal transfer done
    FileIO_FCh_WriteStat(ch, DRV00_STAT_TRANS_ACK_OK); // ok
    // any more to do?
    status = FileIO_FCh_GetStat(ch);
    goes --;
  } while ((status & FILEIO_REQ_ACT) && goes);
}

uint8_t FileIO_Drv00_InsertInit(uint8_t ch, uint8_t drive_number, fch_t *pDrive, char *ext)
{
  DEBUG(1,"Drv00:InsertInit");

  pDrive->pDesc = calloc(1, sizeof(drv00_desc_t)); // 0 everything
  drv00_desc_t* pDesc = pDrive->pDesc;

  pDesc->file_size =  pDrive->fSource->Filesize;

  // select drive
  SPI_EnableFileIO();
  SPI(FCH_CMD(ch,FILEIO_FCH_CMD_CMD_W | 0x0));
  SPI(drive_number);
  SPI_DisableFileIO();

  // write file size
  SPI_EnableFileIO();
  SPI(FCH_CMD(ch,FILEIO_FCH_CMD_CMD_W | 0x4));
  SPI((uint8_t)(pDesc->file_size      ));
  SPI((uint8_t)(pDesc->file_size >>  8));
  SPI((uint8_t)(pDesc->file_size >> 16));
  SPI((uint8_t)(pDesc->file_size >> 24));
  SPI_DisableFileIO();

  DEBUG(1,"Drv00:Size   : %lu (%lu kB)", pDesc->file_size, pDesc->file_size);

  return (0);
}

