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
#include "stdio.h"

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

int main(void)
{
  volatile uint32_t *boot   = (volatile uint32_t *)0x00100000;
  volatile uint32_t *loader = (volatile uint32_t *)0x00102000;
  volatile uint32_t *end    = (volatile uint32_t *)0x00140000;
  char s[256];

  _init_osd();

  // simple checksum of boot area
  unsigned long blsum=0;
  while(boot!=loader) {
    blsum += *boot++;
  }
  // simple checksum of loader area
  unsigned long ldsum=0;
  while(loader!=end) {
    ldsum += *loader++;
  }

  OSD_WriteRC(4, 0, "Checksums:", 0, 0x09, 0);

  sprintf(s,"Bootloader: 0x%08lx",blsum);
  OSD_WriteRC(6, 0, s, 0, 0x09, 0);

  sprintf(s,"Replay Loader: 0x%08lx",ldsum);
  OSD_WriteRC(7, 0, s, 0, 0x09, 0);

  // Loop forever
  while (TRUE) {
    uint16_t key;
    // get user control codes
    key = OSD_GetKeyCode();
    // this key starts the bootloader
    if (key == KEY_RESET) _call_bootloader();
    if (key == KEY_MENU) OSD_Disable();
  }
  return 0; /* never reached */
}
