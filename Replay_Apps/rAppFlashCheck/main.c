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
  OSD_WriteRC(14, 0, "         'X' to reset!          ", 0, 0xE, 0x01);
  OSD_WriteRC(0, 11,            "REPLAY APP", 0, 0xE, 0);
  OSD_Enable(DISABLE_KEYBOARD);
}

// show messages on OSD + USART
void _show(char *s, int line)  {
  printf("%s\r\n",s);
  OSD_WriteRC(line, 0, s, 0, 0x0F, 0);   // 0x09=blue, 0x00=black, 0x0F=white
}

static unsigned int crc32(volatile void* address, uint32_t length)
{
    uint32_t crc = 0xffffffff;
    uint8_t* data = (uint8_t*)address;

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

// Here we go!
int main(void)
{
  volatile uint32_t *boot   = (volatile uint32_t *)0x00100000;
  volatile uint32_t *loader = (volatile uint32_t *)0x00102000;
  volatile uint32_t *end    = (volatile uint32_t *)0x00140000;
  const uint32_t boot_size = (*(uint32_t*)0x100208 == 0xb007c0de) ? *(uint32_t*)0x10020c : 0;
  const uint32_t fw_size   = (*(uint32_t*)0x102020 == 0x600dc0de) ? *(uint32_t*)0x102024 : 0;
  char s[256];
  int osd_enabled=1;

  // re-init USART, disable IRQ-based access
  USART_ReInit(115200);
  // re-init OSD
  _init_osd();
  // re-init printf system, clear terminal output
  init_printf(0x0, USART_Putc);
  printf("\033[2J\r\n");

  // ------------------------------------------------------------------------
  // start app code here!

  OSD_WriteRC(4, 0, "CRC32 Checksums:", 0, 0x0B, 0);

  // crc32 checksum of boot area
  unsigned long blsum=crc32(boot, boot_size ? boot_size + 0x200 : loader - boot);
  sprintf(s,"Bootloader: 0x%08lx",blsum);
  _show(s,6);

  // crc32 checksum of loader area
  unsigned long ldsum=crc32(loader, fw_size ? fw_size : end - loader);
  sprintf(s,"Replay Firmware: 0x%08lx",ldsum);
  _show(s,7);

  // show flash content
  boot   = (volatile uint32_t *)0x00100000;
  sprintf(s,"BL 0: 0x%08lx",*boot++);
  _show(s,9);
  sprintf(s,"BL 1: 0x%08lx",*boot++);
  _show(s,10);
  loader = (volatile uint32_t *)0x00102000;
  sprintf(s,"LD 0: 0x%08lx",*loader++);
  _show(s,12);
  sprintf(s,"LD 1: 0x%08lx",*loader++);
  _show(s,13);

  // Loop forever
  while (TRUE) {
    uint16_t key;
    // get user control codes
    key = OSD_GetKeyCode();
    // this key starts the bootloader
    if (key == KEY_RESET) _call_bootloader();
    if (key == KEY_MENU) {
      if (osd_enabled) OSD_Disable();
      else OSD_Enable(DISABLE_KEYBOARD);
      osd_enabled = !osd_enabled;
    }
  }

  // ------------------------------------------------------------------------
  return 0; /* never reached */
}
