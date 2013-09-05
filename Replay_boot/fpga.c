//
// Support routines for the FPGA including file transfer, system control and DRAM test
//

#include "hardware.h"
#include "osd.h"  // for keyboard input to DRAM debug only
#include "config.h"
#include "fpga.h"
#include "messaging.h"

extern uint8_t *pFileBuf;

const uint8_t kMemtest[128] =
{
    0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0xFF,0xFF,
    0xAA,0xAA,0x55,0x55,0x55,0x55,0xAA,0xAA,0xAA,0x55,0x55,0xAA,0x55,0xAA,0xAA,0x55,
    0x00,0x01,0x00,0x02,0x00,0x04,0x00,0x08,0x00,0x10,0x00,0x20,0x00,0x40,0x00,0x80,
    0x01,0x00,0x02,0x00,0x04,0x00,0x08,0x00,0x10,0x00,0x20,0x00,0x40,0x00,0x80,0x00,
    0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x0E,0xEE,0xEE,0x44,0x4D,
    0x11,0x11,0xEE,0xEC,0xAA,0x55,0x55,0xAB,0xBB,0xBB,0x00,0x0A,0xFF,0xFF,0x80,0x09,
    0x80,0x08,0x00,0x08,0xAA,0xAA,0x55,0x57,0x55,0x55,0xCC,0x36,0x33,0xCC,0xAA,0xA5,
    0xAA,0xAA,0x55,0x54,0x00,0x00,0xFF,0xF3,0xFF,0xFF,0x00,0x02,0x00,0x00,0xFF,0xF1
};

//
// General
//
uint8_t FPGA_Config(FF_FILE *pFile) // assume file is open and at start
{
  uint32_t bytesRead;
  uint32_t secCount;
  unsigned long time;

  DEBUG(2,"FPGA:Starting Configuration.");

  time = Timer_Get(0);

  // set PROG low to reset FPGA (open drain)
  IO_DriveLow_OD(PIN_FPGA_PROG_L); //AT91C_BASE_PIOA->PIO_OER = PIN_FPGA_PROG_L;

  SSC_EnableTxRx(); // start to drive config outputs
  Timer_Wait(1);
  IO_DriveHigh_OD(PIN_FPGA_PROG_L);  //AT91C_BASE_PIOA->PIO_ODR = PIN_FPGA_PROG_L;
  Timer_Wait(2);

  // check INIT is high
  if (IO_Input_L(PIN_FPGA_INIT_L)) {
    WARNING("FPGA:INIT is not high after PROG reset.");
    return FALSE;
  }
  // check DONE is low
  if (IO_Input_H(PIN_FPGA_DONE)) {
    WARNING("FPGA:DONE is high before configuration.");
    return FALSE;
  }

//  printf("[");
  secCount = 0;
  do {
    if (!((secCount++ >> 4) & 3)) {
      ACTLED_ON;
    } else {
      ACTLED_OFF;
    }
//    if ( (secCount & 15) == 0) printf("*");

    bytesRead = FF_Read(pFile, FS_FILEBUF_SIZE, 1, pFileBuf);
    //DumpBuffer(buffer, 512);
    SSC_WriteBufferSingle(pFileBuf, bytesRead);
  } while(bytesRead > 0);

  // some extra clocks
  SSC_Write(0x00);
  //
  SSC_DisableTxRx();
  ACTLED_OFF;
//  printf("]");
  Timer_Wait(1);

  // check DONE is high
  if (!IO_Input_H(PIN_FPGA_DONE) ) {
    WARNING("FPGA:DONE is low after configuration.");
    return FALSE;
  }
  else {
    time = Timer_Get(0)-time;

    DEBUG(0,"FPGA configured in %d ms.", (uint32_t) (time >> 20));
  }
  return TRUE;
}
//
// FILEIO
//

uint8_t FPGA_WaitStat(uint8_t mask, uint8_t wanted)
{
  uint8_t  stat;
  uint32_t timeout = Timer_Get(100);      // 100 ms timeout
  do {
    SPI_EnableFpga();
    SPI(0x87); // do Read
    stat = SPI(0);
    SPI_DisableFpga();

    if (Timer_Check(timeout)) {
      ERROR("FPGA:Waitstat timeout.");
      return (1);
    }
  } while ((stat & mask) != wanted);
  return (0);
}

uint8_t FPGA_SendBuffer(uint8_t *pBuf, uint16_t buf_tx_size)
{
  DEBUG(3,"FPGA:send buffer :%4x.",buf_tx_size);
  if (FPGA_WaitStat(0x02, 0)) // !HF
    return (1); // timeout

  SPI_EnableFpga();
  SPI(0xB0);
  SPI_WriteBufferSingle(pBuf, buf_tx_size);
  SPI_DisableFpga();

  return (0);
}

uint8_t FPGA_ReadBuffer(uint8_t *pBuf, uint16_t buf_tx_size)
{
  // we assume read has completed and FIFO contains complete transfer
  DEBUG(3,"FPGA:read buffer :%4x.",buf_tx_size);
  SPI_EnableFpga();
  SPI(0xA0); // should check status
  SPI_ReadBufferSingle(pBuf, buf_tx_size);
  SPI_DisableFpga();
  return (0);
}

uint8_t FPGA_FileToMem(FF_FILE *pFile, uint32_t base, uint32_t size)
// this function sends given file to FPGA's memory
// base - memory base address (bits 23..16)
// size - memory size (bits 23..16)
{
  uint8_t  rc = 0;
  uint32_t count = 0;
  uint32_t remaining_size = size;

  uint32_t bytes_read;
  uint32_t buf_tx_size;
  /*uint32_t buf_tx_size_even;*/

  DEBUG(3,"FPGA:Uploading file Addr:%8X Size:%8X.",base,size);
  FF_Seek(pFile, 0, FF_SEEK_SET);

  SPI_EnableFpga();
  SPI(0x80); // set address
  SPI((uint8_t)(base));
  SPI((uint8_t)(base >> 8));
  SPI((uint8_t)(base >> 16));
  SPI((uint8_t)(base >> 24));
  SPI_DisableFpga();

  SPI_EnableFpga();
  SPI(0x81); // set direction
  SPI(0x00); // write
  SPI_DisableFpga();

  count = (size + 511) >> 9; // sector count (rounded up) .. just to get the stars

  //printf("[");
  while (count--) {
    // read data sector from memory card
    bytes_read = FF_Read(pFile, 512, 1, pFileBuf);
    if (bytes_read == 0) break; // catch 0 len file error

    buf_tx_size = 512;

    // clip to smallest of file and transfer length
    if (remaining_size < buf_tx_size) buf_tx_size = remaining_size;
    if (bytes_read     < buf_tx_size) buf_tx_size = bytes_read;

    /*buf_tx_size_even = (buf_tx_size + 1) & 0xFFFE;*/

    rc |= FPGA_SendBuffer(pFileBuf, (uint16_t) buf_tx_size);
    //if ((count & 15) == 0) printf("*");
    remaining_size -= buf_tx_size;
  }
  //printf("]");

  if (FPGA_WaitStat(0x01, 0)) // wait for finish
    return(1);

  if (remaining_size != 0) {
    WARNING("FPGA: Sent file truncated. Requested :%8X Sent :%8X.",
                                               size, size - remaining_size);
    rc = 2;
  } else
    DEBUG(2,"FPGA:File uploaded.");

  return(rc) ;// no error
}
/*{{{*/
uint8_t FPGA_FileToMemVerify(FF_FILE *pFile, uint32_t base, uint32_t size)
{
  // for debug
  uint8_t  rc = 0;
  uint32_t count = 0;
  uint32_t remaining_size = size;

  uint32_t bytes_read;
  uint32_t buf_tx_size;
  /*uint32_t buf_tx_size_even;*/
  uint8_t *pTemp = NULL;

  DEBUG(2,"FPGA:Verifying Addr:%8X Size:%8X.",base,size);
  FF_Seek(pFile, 0, FF_SEEK_SET);

  pTemp = malloc(512);
  if (pTemp == NULL) {
    ERROR("FPGA:Memory allocation failure.");
    return (1);
  }

  SPI_EnableFpga();
  SPI(0x80); // set address
  SPI((uint8_t)(base));
  SPI((uint8_t)(base >> 8));
  SPI((uint8_t)(base >> 16));
  SPI((uint8_t)(base >> 24));
  SPI_DisableFpga();

  SPI_EnableFpga();
  SPI(0x81); // set direction
  SPI(0x80); // read
  SPI_DisableFpga();

  count = (size + 511) >> 9; // sector count (rounded up) .. just to get the stars

  //printf("[");
  while (count--) {
    // read data sector from memory card
    bytes_read = FF_Read(pFile, 512, 1, pFileBuf);
    if (bytes_read == 0) break;
    buf_tx_size = 512;

    // clip to smallest of file and transfer length
    if (remaining_size < buf_tx_size) buf_tx_size = remaining_size;
    if (bytes_read     < buf_tx_size) buf_tx_size = bytes_read;

    /*buf_tx_size_even = (buf_tx_size + 1) & 0xFFFE; // even*/

    SPI_EnableFpga();
    SPI(0x84); // do Read
    SPI((uint8_t)( buf_tx_size - 1));
    SPI((uint8_t)((buf_tx_size - 1) >> 8));
    SPI_DisableFpga();

    if (FPGA_WaitStat(0x04, 0)) { // wait for read finish
      free(pTemp); // give the memory back
      return(1);
    }

    FPGA_ReadBuffer(pTemp, buf_tx_size);
    if (memcmp(pFileBuf,pTemp, buf_tx_size)) {
      ERROR("!!Compare fail!! Block Addr:%8X", base);

      DEBUG(2,"Source:", base);
      DumpBuffer(pFileBuf,buf_tx_size);
      DEBUG(2,"Memory:", base);
      DumpBuffer(pTemp,buf_tx_size);

      rc = 1;
      break;
    }
    base += buf_tx_size;

    //if ((count & 15) == 0) printf("*");
    remaining_size -= buf_tx_size;
  }
  //printf("]");

  if (!rc) DEBUG(2,"FPGA:File verified complete.");

  free(pTemp); // give the memory back
  return(rc) ;
}
/*}}}*/


uint8_t FPGA_BufToMem(uint8_t *pBuf, uint32_t base, uint32_t size)
// base - memory base address (bits 23..16)
// size - must be <=512 and even
{
  uint32_t buf_tx_size = size;
  DEBUG(3,"FPGA:BufToMem Addr:%8X.",base);
  // uses write gate to get burst
  SPI_EnableFpga();
  SPI(0x80); // set address
  SPI((uint8_t)(base));
  SPI((uint8_t)(base >> 8));
  SPI((uint8_t)(base >> 16));
  SPI((uint8_t)(base >> 24));
  SPI_DisableFpga();

  SPI_EnableFpga();
  SPI(0x81); // set direction
  SPI(0x00); // write
  SPI_DisableFpga();

  if (size>512) { // arb limit while debugging
    buf_tx_size = 512;
    WARNING("FPGA:Max BufToMem size is 4K bytes.");
  }
  FPGA_SendBuffer(pBuf, buf_tx_size);

  if (FPGA_WaitStat(0x01, 0)) // wait for finish
    return(1);

  return 0 ;// no error
}

uint8_t FPGA_MemToBuf(uint8_t *pBuf, uint32_t base, uint32_t size)
// base - memory base address (bits 23..16)
// size - memory size (bits 23..16)
{
  uint32_t buf_tx_size = size;

  SPI_EnableFpga();
  SPI(0x80); // set address
  SPI((uint8_t)(base));
  SPI((uint8_t)(base >> 8));
  SPI((uint8_t)(base >> 16));
  SPI((uint8_t)(base >> 24));
  SPI_DisableFpga();

  SPI_EnableFpga();
  SPI(0x81); // set direction
  SPI(0x80); // read
  SPI_DisableFpga();

  if (size>512) { // arb limit while debugging
    buf_tx_size = 512;
    WARNING("FPGA:Max MemToBuf size is 512 bytes.");
  }
  SPI_EnableFpga();
  SPI(0x84); // do Read
  SPI((uint8_t)( buf_tx_size - 1)     );
  SPI((uint8_t)((buf_tx_size - 1) >> 8));
  SPI_DisableFpga();

  if (FPGA_WaitStat(0x04, 0)) // wait for read finish
    return(1);

  FPGA_ReadBuffer(pBuf, buf_tx_size);

  return (0) ;// no error
}

uint8_t FPGA_DramTrain(void)
{
  // actually just dram test for now
  uint32_t i;
  uint32_t addr;
  DEBUG(2,"DRAM enabled, running test.");
  // 25..0  64MByte
  // 25 23        15        7
  // 00 0000 0000 0000 0000 0000 0000

  memset(pFileBuf, 0, 512);
  for (i=0;i<128;i++) pFileBuf[i] = kMemtest[i];

  addr = 0;
  for (i=0;i<19;i++){
    pFileBuf[127] = (uint8_t) i;
    FPGA_BufToMem(pFileBuf, addr, 128);
    addr = (0x100 << i);
  }

  addr = 0;
  for (i=0;i<19;i++){
    memset(pFileBuf, 0xAA, 512);
    FPGA_MemToBuf(pFileBuf, addr, 128);
    if (memcmp(pFileBuf,&kMemtest[0],127) || (pFileBuf[127] != (uint8_t) i) ) {
      printf("!!Match fail Addr:%8X", addr);
      DumpBuffer(pFileBuf,128);
      return (1);
    }
    addr = (0x100 << i);
  }
  DEBUG(2,"DRAM passed.");
  return (0);
}

uint8_t FPGA_ProdTest(void)
{
  uint32_t ram_phase;
  uint32_t ram_ctrl;
  uint16_t key;

  DEBUG(0,"PRODTEST: phase 0");
  ram_phase = kDRAM_PHASE;
  ram_ctrl  = kDRAM_SEL;
  OSD_ConfigSendCtrl((ram_ctrl << 8) | ram_phase );
  FPGA_DramTrain();

  DEBUG(0,"PRODTEST: phase+10");
  ram_phase = kDRAM_PHASE + 10;
  ram_ctrl  = kDRAM_SEL;
  OSD_ConfigSendCtrl((ram_ctrl << 8) | ram_phase );
  FPGA_DramTrain();

  DEBUG(0,"PRODTEST: phase-10");
  ram_phase = kDRAM_PHASE - 10;
  ram_ctrl  = kDRAM_SEL;
  OSD_ConfigSendCtrl((ram_ctrl << 8) | ram_phase );
  FPGA_DramTrain();

  // return to nominal
  ram_phase = kDRAM_PHASE;
  ram_ctrl  = kDRAM_SEL;
  OSD_ConfigSendCtrl((ram_ctrl << 8) | ram_phase );

  /*void OSD_ConfigSendUser(uint32_t configD, uint32_t configS)*/
  uint32_t testpat  = 0;
  uint32_t testnum  = 0;
  uint32_t maskpat  = 0;
  uint32_t vidpat   = 0;
  uint32_t vidstd   = 0;

  uint32_t stat     = 0;
  uint32_t config_d = 0;
  uint32_t config_s = 0;
  uint32_t failed   = 0;

  OSD_ConfigSendUserS(0); // default, no reset required

  do {
    config_d = ((maskpat & 0xF) << 18) |((vidpat & 0x3) << 16) | ((testpat & 0xFF) << 8) | (testnum & 0xFF);

    OSD_ConfigSendUserD(config_d);
    Timer_Wait(1);
    OSD_ConfigSendUserD(config_d | 0x80000000);
    Timer_Wait(1);
    OSD_ConfigSendUserD(config_d | 0x00000000);
    Timer_Wait(1);

    stat = OSD_ConfigReadStatus();
    if ((stat & 0x03) != 0x01) {
      WARNING("Test %02X Num %02X, ** FAIL **", testpat, testnum);
      failed ++;
    }

    testnum++;
    switch (testpat) {
      case 0x00 : if (testnum == 0x32) { testnum = 0; testpat ++;} break;
      case 0x01 : if (testnum == 0x06) { testnum = 0; testpat ++;} break;
      case 0x02 : if (testnum == 0x18) { testnum = 0; testpat ++;} break;
      case 0x03 : if (testnum == 0x18) { testnum = 0; testpat ++;} break;
      case 0x04 : if (testnum == 0x18) { testnum = 0; testpat ++;} break;
      case 0x05 : if (testnum == 0x16) { testnum = 0; testpat ++;} break;
      case 0x06 : if (testnum == 0x26) { testnum = 0; testpat ++;} break;
      case 0x07 : if (testnum == 0x0A) { testnum = 0; testpat ++;} break;
    }
    if (testpat == 8) {
      if (failed == 0) {
        DEBUG(0,"IO TEST PASS");
      }
      testpat = 0;
      failed = 0;
    }

    key = OSD_GetKeyCode();
    if (key == KEY_F8) {
      vidpat = (vidpat - 1) & 0x3;
    }
    if (key == KEY_F9) {
      vidpat = (vidpat + 1) & 0x3;
    }

    if (key == KEY_F6) {
      if    (maskpat == 8) 
        maskpat = 0;
      else if (maskpat != 0) 
        maskpat--;
      DEBUG(0,"Mask %01X", maskpat);
    }
    if (key == KEY_F7) {
      if    (maskpat == 0)
        maskpat = 8;
      else if (maskpat != 15)
        maskpat++;
      DEBUG(0,"Mask %01X", maskpat);
    }

    if ((key == KEY_F4) || (key == KEY_F5)) {

      if (key == KEY_F4) vidstd = (vidstd - 1) & 0x7;
      if (key == KEY_F5) vidstd = (vidstd + 1) & 0x7;

      // update, hard reset of FPGA

      IO_DriveLow_OD(PIN_FPGA_RST_L); // make sure FPGA is held in reset

      // set up coder/clock
      switch (vidstd) {
        case 0 : CFG_vid_timing_SD(F60HZ);   CFG_set_coder(CODER_NTSC);        break;
        case 1 : CFG_vid_timing_SD(F50HZ);   CFG_set_coder(CODER_PAL);         break;
        case 2 : CFG_vid_timing_SD(F60HZ);   CFG_set_coder(CODER_NTSC_NOTRAP); break;
        case 3 : CFG_vid_timing_SD(F50HZ);   CFG_set_coder(CODER_PAL_NOTRAP);  break;
        case 4 : CFG_vid_timing_HD27(F60HZ); CFG_set_coder(CODER_DISABLE);     break;
        case 5 : CFG_vid_timing_HD27(F50HZ); CFG_set_coder(CODER_DISABLE);     break;
        case 6 : CFG_vid_timing_HD74(F60HZ); CFG_set_coder(CODER_DISABLE);     break;
        case 7 : CFG_vid_timing_HD74(F50HZ); CFG_set_coder(CODER_DISABLE);     break;
      }
      DEBUG(0,"VidStd %01X", vidstd);
      Timer_Wait(200);

      IO_DriveHigh_OD(PIN_FPGA_RST_L); // release reset
      Timer_Wait(200);

      if ((vidstd == 6) || (vidstd == 7))
        CFG_set_CH7301_HD();
      else
        CFG_set_CH7301_SD();

      // resend config
      config_s = (vidstd & 0x7);
      OSD_ConfigSendUserS(config_s);
      OSD_ConfigSendUserD(config_d);
      Timer_Wait(10);

      // apply new static config
      OSD_Reset(OSDCMD_CTRL_RES);

      Timer_Wait(10);
    }

    Timer_Wait(5);

  } while (key != KEY_MENU);
  return (0);
}

