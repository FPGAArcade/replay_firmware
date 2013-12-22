//
// Replay Application Main File
//
// This is for Replay-apps only. As they will be called from a configured
// Replay board, they do not include any INIT and SDCARD stuff.
//
// $Id$
//

#include "hardware.h"
#include "fpga.h"
#include "osd.h"

// for sprintf
#include "printf.h"

const char version[] = {__BUILDNUMBER__};

// we copy the actual bootloader from flash to the RAM and call it
// (we assume there is no collision with the actual stack)
void _call_bootloader(void)
{
  int i;

  volatile uint32_t *src = (volatile uint32_t *)0x00100200;
  volatile uint32_t *dest = (volatile uint32_t *)0x00200000;

  // set PROG low to reset FPGA (open drain)
  IO_DriveLow_OD(PIN_FPGA_PROG_L);
  Timer_Wait(1);
  IO_DriveHigh_OD(PIN_FPGA_PROG_L);
  Timer_Wait(2);

  // copy bootloader to SRAM
  for(i = 0; i < 1024; i++) {
      *dest++ = *src++;
  }

  // launch bootloader in SRAM
  asm("ldr r3, = 0x00200000\n");
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
void _get_block(uint32_t base, void *buf)
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
  FPGA_WaitStat(0x04, 0);
  SPI_EnableFpga();
  SPI(0xA0); // should check status
  SPI_ReadBufferSingle(buf, 512);
  SPI_DisableFpga();
}

// Here we go!
int main(void)
{
  char s[256];

  // re-init USART, disable IRQ-based access
  USART_ReInit(115200);
  // re-init OSD
  _init_osd();
  // re-init printf system, clear terminal output
  init_printf(0x0, USART_Putc);
  printf("\033[2J\r\n");

  // ------------------------------------------------------------------------
  // start app code here!

  // DRAM map:
  // 0x00000000 ... reserved for rApp code (up to 64kByte, theoretically)
  // 0x000FFE00 ... reserved for configuration stuff for flasher (1 block)
  //                0 (4byte):  start of new boot code
  //                1 (4byte): length of new boot code
  //                2 (4byte): checksum of new boot code
  //                3 (4byte):  start of new loader code
  //                4 (4byte): length of new loader code
  //                5 (4byte): checksum of new loader code
  // 0x00100000 ... reserved for new flash code (up to 256kByte)

  // get configuration
  uint32_t config[128];
  _get_block(0x000FFE00L,config);
  uint32_t bootbase = config[0];
  uint32_t bootlength = config[1];
  uint32_t bootsum = config[2];
  uint32_t loaderbase = config[3];
  uint32_t loaderlength = config[4];
  uint32_t loadersum = config[5];

  uint32_t i, j, l, length, bok=1, lok=1;
  volatile uint32_t code;  // set to base later
  volatile uint32_t *fptr; // set to base later

  sprintf(s,"BOOT:");
  _show(s,2);
  sprintf(s," Base:   0x%08lx",bootbase);
  _show(s,3);
  sprintf(s," Length: 0x%08lx",bootlength);
  _show(s,4);
  
  sprintf(s,"LOADER:");
  _show(s,5);
  sprintf(s," Base:   0x%08lx",loaderbase);
  _show(s,6);
  sprintf(s," Length: 0x%08lx",loaderlength);
  _show(s,7);

  sprintf(s,"CHECKSUMS (flash/sdcard):");
  _show(s,8);

  length=bootlength; l=0;
  uint32_t bsum=0, bfsum=0;
  code   = bootbase;
  fptr = (volatile uint32_t *)bootbase;
  for(i=0;i<((length/512)+1);i++) {
    uint32_t buf[128];
    _get_block(code,buf);
    for(j=0;j<128;j++) {
      if (l<length) bsum+=buf[j];
      else break;
      bfsum+=*fptr++;
      l += 4;
    }
    code+=512;
  }
  if (bootsum!=bsum) {
    sprintf(s," BOOT:   BAD SDCARD DATA!");
  } else {
    sprintf(s," BOOT:   0x%08lx/0x%08lx %s",bfsum,bsum,bfsum==bsum?" ":"!");
    if (bfsum!=bsum) bok=0;
  }
  _show(s,9);

  length=loaderlength; l=0;
  uint32_t lsum=0, lfsum=0;
  code   = loaderbase;
  fptr = (volatile uint32_t *)loaderbase;
  for(i=0;i<((length/512)+1);i++) {
    uint32_t buf[128];
    _get_block(code,buf);
    for(j=0;j<128;j++) {
      if (l<length) lsum+=buf[j];
      else break;
      lfsum+=*fptr++;
      l += 4;
    }
    code+=512;
  }
  if (loadersum!=lsum) {
    sprintf(s," LOADER: BAD SDCARD DATA!");
  } else {
    sprintf(s," LOADER: 0x%08lx/0x%08lx %s",lfsum,lsum,lfsum==lsum?" ":"!");
    if (lfsum!=lsum) lok=0;
  }
  _show(s,10);

  if (!lok) {    // we don't process bok for now until we know this works well!
    sprintf(s,"--PRESS 'F' TO START FLASHING!--");
    _show(s,12);
    // Loop forever
    while (TRUE) {
      uint16_t key;
      // get user control codes
      key = OSD_GetKeyCode();
      // this key starts the flashing
      if (key == 'f') break;
    }
    sprintf(s,"********************************");
    _showhi(s,11);
    sprintf(s,"* FLASHING! DO NOT SWITCH OFF! *");
    _showhi(s,12);
    sprintf(s,"********************************");
    _showhi(s,13);
    
    // Configure the flash again, just to be safe...
    AT91C_BASE_MC->MC_FMR = AT91C_MC_FWS_1FWS |
                            (48<<16);

    // flash the main loader
    length = loaderlength; l=0;
    code   = loaderbase;
    for(i=0;i<((length/512)+1);i++) {
      uint32_t buf[128];
      volatile uint32_t *p=0x0;
      _get_block(code,buf);
      // write first page from buffer
      for(j=0;j<64;j++) {
        p[j]=buf[j];
      l+=4;
      }
      Timer_Wait(10); // we need this to delay, the following while is quite useless...?
      while(!((AT91C_BASE_MC->MC_FSR) & AT91C_MC_FRDY));
      AT91C_BASE_MC->MC_FCR = (0x5A000000L) |
                              (code&0x3FF00L) |
                              AT91C_MC_FCMD_START_PROG;
      Timer_Wait(10); // we need this to delay, the following while is quite useless...?
      while(!((AT91C_BASE_MC->MC_FSR) & AT91C_MC_FRDY));
      code+=256;
      // write second page from buffer
      for(j=0;j<64;j++) {
        p[j]=buf[j+64];
        l+=4;
      }
      Timer_Wait(10); // we need this to delay, the following while is quite useless...?
      while(!((AT91C_BASE_MC->MC_FSR) & AT91C_MC_FRDY));
      AT91C_BASE_MC->MC_FCR = (0x5A000000L) |
                              (code&0x3FF00L) |
                              AT91C_MC_FCMD_START_PROG;
      Timer_Wait(10); // we need this to delay, the following while is quite useless...?
      while(!((AT91C_BASE_MC->MC_FSR) & AT91C_MC_FRDY));
      code+=256;
      // show something on OSD...
      if ((l&0xfff)==0) {
        sprintf(s,"0x%08x %lu%%",code,100*l/length);
        _show(s,13);
      }
    }
  }
  // 
  sprintf(s,"                                ");
  _show(s,11);
  sprintf(s,"--DONE, PLS. POWER CYCLE BOARD--");
  _show(s,12);
  sprintf(s,"                                ");
  _show(s,13);
  
  // Loop forever
  while (TRUE) {
      uint16_t key;
      // get user control codes
      key = OSD_GetKeyCode();
      // this key starts the flashing
      if (key == 'r') {
        _call_bootloader();
        // perform a reset
        asm("ldr r3, = 0x00000000\n");
        asm("bx  r3\n");
      }
  }

  // ------------------------------------------------------------------------
  return 0; /* never reached */
}
