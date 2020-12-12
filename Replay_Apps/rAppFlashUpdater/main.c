/*--------------------------------------------------------------------
 *               Replay Firmware Application (rApp)
 *                      www.fpgaarcade.com
 *                     All rights reserved.
 *
 *                     admin@fpgaarcade.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *--------------------------------------------------------------------
 *
 * Copyright (c) 2020, The FPGAArcade community (see AUTHORS.txt)
 *
 */

//
// Replay Application Main File
//
// This is for Replay-apps only. As they will be called from a configured
// Replay board, they do not include any INIT and SDCARD stuff.
//

#include "hardware.h"
#include "fpga.h"
#include "osd.h"

// for sprintf
#include "printf.h"

__attribute__ ((noreturn)) void _call_bootloader(void)
{
  // set PROG low to reset FPGA (open drain)
  IO_DriveLow_OD(PIN_FPGA_PROG_L);
  Timer_Wait(1);
  IO_DriveHigh_OD(PIN_FPGA_PROG_L);
  Timer_Wait(2);

  AT91C_BASE_AIC->AIC_IDCR = AT91C_ALL_INT;
  AT91C_BASE_SPI ->SPI_PTCR = AT91C_PDC_RXTDIS | AT91C_PDC_TXTDIS;

  // perform a ARM reset
  asm("ldr r3, = 0x00000000\n");
  asm("bx  r3\n");

  // we will never reach here
  while (1) {}
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

// reset with data = 0x00000000; length = 0xffffffff
static unsigned int feed_crc32(uint32_t address, uint32_t length)
{
    static uint32_t crc;
    uint8_t* data = (uint8_t*)address;

    if (data == 0 && length == 0xffffffff) {
        crc = length;
        length = 0;
    }

    for (uint32_t i = 0; i < length; ++i) {
        uint32_t byte = *data++;
        crc = crc ^ byte;

        for (int j = 7; j >= 0; j--) {    // Do eight times.
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }

    return ~crc;
}

void flash(uint32_t base, uint32_t size)
{
  char s[256];
  uint32_t i, j, l, length;
  volatile uint32_t code;
  length = size; l=0;
  code   = base;
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
      sprintf(s,"@0x%08x (%lu%% written)",code,100*l/length);
      OSD_WriteRC(12, 2, s, 0, 0xF, 0);
    }
  }
}

// Here we go!
int main(void)
{
  char s[256];

  // re-init USART, disable IRQ-based access
  USART_ReInit(115200);
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

  uint32_t i, j, l, length, bok, lok;
  volatile uint32_t code;  // set to base later

  while (1) {
    // re-init OSD
    _init_osd();

    sprintf(s,"Flasher rApp by Wolfgang Scherr");
    _show(s,2);

    sprintf(s,"BOOT (base/length):");
    _show(s,3);
    sprintf(s," Base:   0x%08lx/0x%08lx",bootbase,bootlength);
    _show(s,4);
    
    sprintf(s,"LOADER (base/length):");
    _show(s,5);
    sprintf(s," Base:   0x%08lx/0x%08lx",loaderbase,loaderlength);
    _show(s,6);

    sprintf(s,"CHECKSUMS (flash/sdcard):");
    _show(s,7);

    bok=1, lok=1;
    length=bootlength; l=0;
    feed_crc32(0x0,0xffffffff);
    const uint32_t current_bootlength = (*(uint32_t*)0x100208 == 0xb007c0de) ? *(uint32_t*)0x10020c + 0x200: bootlength;
    uint32_t bsum=0, bfsum=feed_crc32(bootbase,current_bootlength);
    code   = bootbase;
    feed_crc32(0x0,0xffffffff);
    for(i=0;i<((length/512)+1);i++) {
      uint32_t buf[128];
      _get_block(code,buf);
      for(j=0;j<128;j++) {
        if (l<length) bsum=feed_crc32((uint32_t)&buf[j],sizeof(buf[j]));
        else break;
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
    _show(s,8);

    length=loaderlength; l=0;
    feed_crc32(0x0,0xffffffff);
    const uint32_t current_loaderlength = (*(uint32_t*)0x102020 == 0x600dc0de) ? *(uint32_t*)0x102024 : loaderlength;
    uint32_t lsum=0, lfsum=feed_crc32(loaderbase,current_loaderlength);
    code   = loaderbase;
    feed_crc32(0x0,0xffffffff);
    for(i=0;i<((length/512)+1);i++) {
      uint32_t buf[128];
      _get_block(code,buf);
      for(j=0;j<128;j++) {
        if (l<length) lsum=feed_crc32((uint32_t)&buf[j],sizeof(buf[j]));
        else break;
        l += 4;
      }
      code+=512;
    }
    if (loadersum!=lsum) {
      sprintf(s," LOADER: BAD SDCARD DATA!");
      printf("loadersum = %08x, lsum = %08x",loadersum,lsum);
    } else {
      sprintf(s," LOADER: 0x%08lx/0x%08lx %s",lfsum,lsum,lfsum==lsum?" ":"!");
      if (lfsum!=lsum) lok=0;
    }
    _show(s,9);

    if (!bok || !lok) {
      sprintf(s,"--PRESS 'F' TO START FLASHING!--");
      _show(s,11);
      sprintf(s,"--  (OR PRESS 'R' TO REBOOT)  --");
      _show(s,12);
      // Loop forever
      while (TRUE) {
        uint16_t key;
        // get user control codes
        key = OSD_GetKeyCode();
        // user decided to reboot
        if (key == 'r') {
          _call_bootloader();
        }
        // this key starts the flashing
        if (key == 'f') break;
      }
      sprintf(s,"********************************");
      _showhi(s,10);
      sprintf(s,"* FLASHING! DO NOT SWITCH OFF! *");
      _showhi(s,11);
      sprintf(s,"*                              *");
      _showhi(s,12);
      sprintf(s,"********************************");
      _showhi(s,13);
      
      // Configure the flash again, just to be safe...
      AT91C_BASE_MC->MC_FMR = AT91C_MC_FWS_1FWS |
                              (48<<16);

      // flash the boot loader
      if (!bok) {
        printf("flashing bootloader\r\n");
        flash(bootbase, bootlength);
      }
      // flash the main loader
      if (!lok) {
        printf("flashing firmware\r\n");
        flash(loaderbase, loaderlength);
      }
    } else {
      // 
      sprintf(s,"                                ");
      _show(s,10);
      sprintf(s,"                                ");
      _show(s,11);
      sprintf(s,">flash OK - press 'R' to reboot<");
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
          }
      }
    }
  }
  
  // ------------------------------------------------------------------------
  return 0; /* never reached */
}
