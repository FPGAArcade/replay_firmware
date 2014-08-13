
// MCh is memory chanel <> DRAM

#include "fileio.h"
#include "hardware.h"
#include "messaging.h"


uint8_t FileIO_MCh_WaitStat(uint8_t mask, uint8_t wanted)
{
  uint8_t  stat;
  uint32_t timeout = Timer_Get(100);      // 100 ms timeout
  do {
    SPI_EnableFileIO();
    SPI(0x87); // do Read
    stat = SPI(0);
    SPI_DisableFileIO();

    if (Timer_Check(timeout)) {
      WARNING("MCh:Waitstat timeout.");
      return (1);
    }
  } while ((stat & mask) != wanted);
  return (0);
}

uint8_t FileIO_MCh_SendBuffer(uint8_t *pBuf, uint16_t buf_tx_size)
{
  DEBUG(3,"MCh:send buffer :%4x.",buf_tx_size);
  if (FileIO_MCh_WaitStat(0x02, 0)) // !HF
    return (1); // timeout

  SPI_EnableFileIO();
  SPI(0xB0);
  SPI_WriteBufferSingle(pBuf, buf_tx_size);
  SPI_DisableFileIO();

  return (0);
}

uint8_t FileIO_MCh_ReadBuffer(uint8_t *pBuf, uint16_t buf_tx_size)
{
  // we assume read has completed and FIFO contains complete transfer
  DEBUG(3,"MCh:read buffer :%4x.",buf_tx_size);
  SPI_EnableFileIO();
  SPI(0xA0); // should check status
  SPI_ReadBufferSingle(pBuf, buf_tx_size);
  SPI_DisableFileIO();
  return (0);
}

uint8_t FileIO_MCh_FileToMem(FF_FILE *pFile, uint32_t base, uint32_t size, uint32_t offset)
// this function sends given file to FPGA's memory
// base - memory base address (bits 23..16)
// size - memory size (bits 23..16)
{
  uint8_t  rc = 0;
  uint32_t remaining_size = size;
  unsigned long time;
  time = Timer_Get(0);

  DEBUG(3,"FPGA:Uploading file Addr:%8X Size:%8X.",base,size);
  FF_Seek(pFile, offset, FF_SEEK_SET);

  SPI_EnableFileIO();
  SPI(0x80); // set address
  SPI((uint8_t)(base));
  SPI((uint8_t)(base >> 8));
  SPI((uint8_t)(base >> 16));
  SPI((uint8_t)(base >> 24));
  SPI_DisableFileIO();

  SPI_EnableFileIO();
  SPI(0x81); // set direction
  SPI(0x00); // write
  SPI_DisableFileIO();

  while (remaining_size) {
    #if (FILEIO_MEMBUF_SIZE>FILEBUF_SIZE)
      #error "FILEIO_MEMBUF_SIZE>FILEBUF_SIZE !"
    #endif
    uint8_t fBuf[FILEBUF_SIZE];
    uint8_t *wPtr;
    uint32_t buf_tx_size = FILEBUF_SIZE;
    uint32_t bytes_read;
    uint16_t fpgabuf_size=FILEIO_MEMBUF_SIZE;

    // read data sector from memory card
    bytes_read = FF_Read(pFile, FILEBUF_SIZE, 1, fBuf);
    if (bytes_read == 0) break; // catch 0 len file error

    // clip to smallest of file and transfer length
    if (remaining_size < buf_tx_size) buf_tx_size = remaining_size;
    if (bytes_read     < buf_tx_size) buf_tx_size = bytes_read;
    remaining_size -= buf_tx_size;

    // send file buffer
    wPtr = &(fBuf[0]);
    while(buf_tx_size) {
      if (FileIO_MCh_WaitStat(0x01, 0)) // wait for finish, it is a little faster doing that way
        return(1);
      if (buf_tx_size < fpgabuf_size) fpgabuf_size = buf_tx_size;
      rc |= FileIO_MCh_SendBuffer(wPtr, fpgabuf_size);
      wPtr += fpgabuf_size;
      buf_tx_size -= fpgabuf_size;
    }
  }

  if (FileIO_MCh_WaitStat(0x01, 0)) // wait for finish (final)
    return(1);
  time = Timer_Get(0)-time;
  DEBUG(1,"Upload done in %d ms.", (uint32_t) (time >> 20));

  if (remaining_size != 0) {
    WARNING("MCh: Sent file truncated. Requested :%8X Sent :%8X.",
                                               size, size - remaining_size);
    rc = 2;
  } else
    DEBUG(2,"MCh:File uploaded.");

  return(rc) ;// no error
}

uint8_t FileIO_MCh_FileToMemVerify(FF_FILE *pFile, uint32_t base, uint32_t size, uint32_t offset)
{
  // for debug
  uint8_t  rc = 0;
  uint32_t remaining_size = size;
  unsigned long time;
  time = Timer_Get(0);

  DEBUG(2,"MCh:Verifying Addr:%8X Size:%8X.",base,size);
  FF_Seek(pFile, offset, FF_SEEK_SET);

  SPI_EnableFileIO();
  SPI(0x80); // set address
  SPI((uint8_t)(base));
  SPI((uint8_t)(base >> 8));
  SPI((uint8_t)(base >> 16));
  SPI((uint8_t)(base >> 24));
  SPI_DisableFileIO();

  SPI_EnableFileIO();
  SPI(0x81); // set direction
  SPI(0x80); // read
  SPI_DisableFileIO();

  while (remaining_size) {
    #if (FILEIO_MEMBUF_SIZE>FILEBUF_SIZE)
      #error "FILEIO_MEMBUF_SIZE>FILEBUF_SIZE !"
    #endif
    uint8_t fBuf[FILEBUF_SIZE];
    uint8_t tBuf[FILEBUF_SIZE];
    uint8_t *wPtr;
    uint32_t buf_tx_size = FILEBUF_SIZE;
    uint32_t bytes_read;
    uint16_t fpgabuf_size=FILEIO_MEMBUF_SIZE;
    uint32_t fpga_read_left;

    // read data sector from memory card
    bytes_read = FF_Read(pFile, FILEBUF_SIZE, 1, fBuf);
    if (bytes_read == 0) break;

    // clip to smallest of file and transfer length
    if (remaining_size < buf_tx_size) buf_tx_size = remaining_size;
    if (bytes_read     < buf_tx_size) buf_tx_size = bytes_read;

    // read same sector from FPGA
    wPtr=&(tBuf[0]);
    fpga_read_left=buf_tx_size;
    while (fpga_read_left) {
      if (fpga_read_left < fpgabuf_size) fpgabuf_size = fpga_read_left;
      SPI_EnableFileIO();
      SPI(0x84); // do Read
      SPI((uint8_t)( fpgabuf_size - 1));
      SPI((uint8_t)((fpgabuf_size - 1) >> 8));
      SPI_DisableFileIO();
      if (FileIO_MCh_WaitStat(0x04, 0)) { // wait for read finish
        return(1);
      }
      FileIO_MCh_ReadBuffer(wPtr, fpgabuf_size);
      wPtr+=fpgabuf_size;
      fpga_read_left-=fpgabuf_size;
    }

    // compare
    if (memcmp(fBuf,tBuf, buf_tx_size)) {
      WARNING("!!Compare fail!! Block Addr:%8X", base);

      DEBUG(2,"Source:", base);
      DumpBuffer(fBuf,buf_tx_size);
      DEBUG(2,"Memory:", base);
      DumpBuffer(tBuf,buf_tx_size);

      rc = 1;
      break;
    }
    base += buf_tx_size;
    remaining_size -= buf_tx_size;
  }

  time = Timer_Get(0)-time;
  DEBUG(1,"Verify done in %d ms.", (uint32_t) (time >> 20));

  if (!rc) DEBUG(2,"MCh:File verified complete.");

  return(rc) ;
}

uint8_t FileIO_MCh_MemToFile(FF_FILE *pFile, uint32_t base, uint32_t size, uint32_t offset)
{
  // for debug
  uint8_t  rc = 0;
  uint32_t remaining_size = size;
  unsigned long time;
  time = Timer_Get(0);

  //DEBUG(1,"FPGA_MemToFile(%x,%x,%x,%x)",pFile,base,size,offset);

  DEBUG(2,"MCh:Store Addr:%8X Size:%8X.",base,size);
  if (offset) FF_Seek(pFile, offset, FF_SEEK_SET);

  SPI_EnableFileIO();
  SPI(0x80); // set address
  SPI((uint8_t)(base));
  SPI((uint8_t)(base >> 8));
  SPI((uint8_t)(base >> 16));
  SPI((uint8_t)(base >> 24));
  SPI_DisableFileIO();

  SPI_EnableFileIO();
  SPI(0x81); // set direction
  SPI(0x80); // read
  SPI_DisableFileIO();

  while (remaining_size) {
    #if (FILEIO_MEMBUF_SIZE>FILEBUF_SIZE)
      #error "FILEIO_MEMBUF_SIZE>FILEBUF_SIZE !"
    #endif
    uint8_t tBuf[FILEBUF_SIZE];
    uint8_t *wPtr;
    uint32_t buf_tx_size = FILEBUF_SIZE;
    uint32_t bytes_written;
    uint16_t fpgabuf_size=FILEIO_MEMBUF_SIZE;
    uint32_t fpga_read_left;

    // clip to smallest of file and transfer length
    if (remaining_size < buf_tx_size) buf_tx_size = remaining_size;

    // read sector from FPGA
    wPtr=&(tBuf[0]);
    fpga_read_left=buf_tx_size;
    while (fpga_read_left) {
      if (fpga_read_left < fpgabuf_size) fpgabuf_size = fpga_read_left;
      SPI_EnableFileIO();
      SPI(0x84); // do Read
      SPI((uint8_t)( fpgabuf_size - 1));
      SPI((uint8_t)((fpgabuf_size - 1) >> 8));
      SPI_DisableFileIO();
      if (FileIO_MCh_WaitStat(0x04, 0)) { // wait for read finish
        return(1);
      }
      FileIO_MCh_ReadBuffer(wPtr, fpgabuf_size);
      wPtr+=fpgabuf_size;
      fpga_read_left-=fpgabuf_size;
    }

    // write data sector to memory card
    bytes_written = FF_Write(pFile, buf_tx_size, 1, tBuf);
    if (bytes_written == 0) break;

    // go on...
    remaining_size -= buf_tx_size;
  }

  time = Timer_Get(0)-time;
  DEBUG(1,"Save done in %d ms.", (uint32_t) (time >> 20));

  if (!rc) DEBUG(2,"MCh:File written.");

  return(rc) ;
}

uint8_t FileIO_MCh_BufToMem(uint8_t *pBuf, uint32_t base, uint32_t size)
// base - memory base address
// size - must be <=FILEIO_MEMBUF_SIZE and even
{
  uint32_t buf_tx_size = size;
  DEBUG(3,"MCh:BufToMem Addr:%8X.",base);
  SPI_EnableFileIO();
  SPI(0x80); // set address
  SPI((uint8_t)(base));
  SPI((uint8_t)(base >> 8));
  SPI((uint8_t)(base >> 16));
  SPI((uint8_t)(base >> 24));
  SPI_DisableFileIO();

  SPI_EnableFileIO();
  SPI(0x81); // set direction
  SPI(0x00); // write
  SPI_DisableFileIO();

  if (size>FILEIO_MEMBUF_SIZE) { // arb limit while debugging
    buf_tx_size = FILEIO_MEMBUF_SIZE;
    WARNING("MCh:Max BufToMem size is %d bytes.",FILEIO_MEMBUF_SIZE);
  }
  FileIO_MCh_SendBuffer(pBuf, buf_tx_size);

  if (FileIO_MCh_WaitStat(0x01, 0)) // wait for finish
    return(1);

  return 0 ;// no error
}

uint8_t FileIO_MCh_MemToBuf(uint8_t *pBuf, uint32_t base, uint32_t size)
// base - memory base address
// size - must be <=FILEIO_MEMBUF_SIZE and even
{
  uint32_t buf_tx_size = size;

  SPI_EnableFileIO();
  SPI(0x80); // set address
  SPI((uint8_t)(base));
  SPI((uint8_t)(base >> 8));
  SPI((uint8_t)(base >> 16));
  SPI((uint8_t)(base >> 24));
  SPI_DisableFileIO();

  SPI_EnableFileIO();
  SPI(0x81); // set direction
  SPI(0x80); // read
  SPI_DisableFileIO();

  if (size>FILEIO_MEMBUF_SIZE) { // arb limit while debugging
    buf_tx_size = FILEIO_MEMBUF_SIZE;
    WARNING("MCh:Max MemToBuf size is %d bytes.",FILEIO_MEMBUF_SIZE);
  }
  SPI_EnableFileIO();
  SPI(0x84); // do Read
  SPI((uint8_t)( buf_tx_size - 1)     );
  SPI((uint8_t)((buf_tx_size - 1) >> 8));
  SPI_DisableFileIO();

  if (FileIO_MCh_WaitStat(0x04, 0)) // wait for read finish
    return(1);

  FileIO_MCh_ReadBuffer(pBuf, buf_tx_size);

  return (0) ;// no error
}


