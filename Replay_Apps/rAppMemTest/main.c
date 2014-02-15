//
// Replay Application Main File
//
// This is for Replay-apps only. As they will be called from a configured
// Replay board, they do not include any INIT and SDCARD stuff.
//
// $Id: main.c 241 2013-12-23 11:59:21Z wolfgang.scherr $
//

#include "hardware.h"
#include "fpga.h"
#include "osd.h"

// for memset and memcmp
#include "string.h"

// for sprintf
#include "printf.h"

const char version[] = {__BUILDNUMBER__};

// reboot
void _call_bootloader(void)
{
  // launch bootloader in FLASH
  //asm("ldr r3, = 0x00200000\n");
  asm("ldr r3, = 0x00000000\n");
  asm("bx  r3\n");
}

// initialize OSD again
void _init_osd(void)
{
  OSD_SetDisplay(0);
  OSD_SetPage(0);
  OSD_Clear();
  OSD_Write  (0,     "      ***              ***      ", 0);
  OSD_WriteRC(1, 0,  "                                ", 0, 0, 0x01);
  OSD_WriteRC(14, 0, "                                ", 0, 0, 0x01);
  OSD_WriteRC(0, 11,            "REPLAY APP", 0, 0xE, 0);
  OSD_Enable(DISABLE_KEYBOARD);
}

// show messages on OSD + USART
void _show(char *s, int line)  {
  printf("%s\r\n",s);
  OSD_WriteRC(line, 0, s, 0, 0x0F, 0);   // 0x09=blue, 0x00=black, 0x0F=white
}
void _showhi(char *s, int line)  {
  printf("%s\r\n",s);
  OSD_WriteRC(line, 0, s, 0, 0x0E, 0);   // 0x0e=yellow, 0x09=blue, 0x00=black, 0x0F=white
}

// get 512 byte memory block from FPGA
int _get_block(uint32_t base, void *buf)
{
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

  SPI_EnableFpga();
  SPI(0x84); // read first buffer, FPGA stalls if we don't read this size
  SPI((uint8_t)( 512 - 1));
  SPI((uint8_t)((512 - 1) >> 8));
  SPI_DisableFpga();

  if (FPGA_WaitStat(0x04, 0)) // wait for finish
    return 1; // timeout

  SPI_EnableFpga();
  SPI(0xA0); // should check status
  SPI_ReadBufferSingle(buf, 512);
  SPI_DisableFpga();

  return 0;
}

// set 512 byte memory block in FPGA
int _set_block(uint32_t base, void *buf)
{
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

  if (FPGA_WaitStat(0x02, 0)) // !HF
    return 1; // timeout

  SPI_EnableFpga();
  SPI(0xB0);
  SPI_WriteBufferSingle(buf, 512);
  SPI_DisableFpga();

  if (FPGA_WaitStat(0x01, 0)) // wait for finish
    return 1;

  return 0 ;// no error
}

void _reset(void)
{
  // soft reset
  SPI_EnableOsd();
  SPI(0x11);
  SPI_DisableOsd();
}

void _halt(void)
{
  // soft reset
  SPI_EnableOsd();
  SPI(0x11|0x12);
  SPI_DisableOsd();
}

void _send_control(uint32_t config)
{
  SPI_EnableOsd();
  SPI(0x20 | 0x03); // ctrl
  SPI((uint8_t)(config));
  SPI((uint8_t)(config >> 8));
  SPI_DisableOsd();
}

#define kDRAM_PHASE 0x68
#define kDRAM_SEL   0x02

void _set_dram_phase(uint8_t phase)
{
    _send_control((kDRAM_SEL << 8) | phase);
}

//
// Memory Test
// 25..0  64MByte
//

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

// simple RAM test
uint8_t _test_mem(void)
{
  uint8_t mBuf[512];
  uint32_t i;
  uint32_t addr;

  memset(mBuf, 0, 512);
  for (i=0;i<128;i++) {
    mBuf[i]     = kMemtest[i];
  }

  addr = 0;
  for (i=0;i<19;i++){
    mBuf[127] = (uint8_t) i;
    _set_block(addr, mBuf);
    addr = (0x100 << i);
  }

  addr = 0;
  for (i=0;i<19;i++){
    memset(mBuf, 0xAA, 512);
    _get_block(addr, mBuf);
    if (memcmp(mBuf,&kMemtest[0],127) || (mBuf[127] != (uint8_t) i) ) {
      // FAIL!
      return 1;
    }
    addr = (0x100 << i);
  }

  // PASS!
  return 0;
}

// Here we go!
int main(void)
{
  char s[256];
  int i, low, high;
  
  // re-init USART, disable IRQ-based access
  USART_ReInit(115200);
  // re-init OSD
  _init_osd();
  // re-init printf system, clear terminal output
  init_printf(0x0, USART_Putc);
  printf("\033[2J\r\n");

  // ------------------------------------------------------------------------
  // start app code here!

  // reset but keep core stopped
  _halt();

  sprintf(s,"RamTest rApp by Wolfgang Scherr");
  _show(s,2);

  _set_dram_phase(kDRAM_PHASE);
  Timer_Wait(1000);
  i=_test_mem();
  sprintf(s,"Nominal phase: %s",i?"bad":"good");
  _show(s,4);

  Timer_Wait(1000);

  //====================================================
  sprintf(s,"Find high boundary...");
  _show(s,6);
  for(i=1;i<(255-kDRAM_PHASE);++i) {
    _set_dram_phase(kDRAM_PHASE+i);
    if (_test_mem()) break;
  }
  high=i;
  sprintf(s,"Highest value: +%d (=%d)",i,kDRAM_PHASE+i);
  _show(s,6);
  //====================================================

  //====================================================
  _set_dram_phase(kDRAM_PHASE);
  Timer_Wait(1000);
  i=_test_mem();
  sprintf(s,"Nominal phase again: %s",i?"bad":"good");
  _show(s,7);
  //====================================================

  //====================================================
  sprintf(s,"Find low boundary...");
  _show(s,8);
  for(i=1;i<kDRAM_PHASE;++i) {
    _set_dram_phase(kDRAM_PHASE-i);
    if (_test_mem()) break;
  }
  low=i;
  sprintf(s,"Lowest value: -%d (=%d)",i,kDRAM_PHASE-i);
  _show(s,8);
  //====================================================

  //====================================================
  i=(high-low)>>1;
  sprintf(s,"Got middle value: %d (=%d)",i,kDRAM_PHASE+i);
  _show(s,10);
  _set_dram_phase(kDRAM_PHASE+i);
  Timer_Wait(1000);
  //====================================================

  sprintf(s,"Press 'r' to reboot...");
  _show(s,13);
  
  // Loop forever
  while (TRUE) {
      uint16_t key;
      // get user control codes
      key = OSD_GetKeyCode();
      // this key starts the flashing
      if (key == 'r') {
        _call_bootloader();
      }
  }

  // ------------------------------------------------------------------------
  return 0; /* never reached */
}
