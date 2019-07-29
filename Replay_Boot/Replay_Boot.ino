#include <Arduino.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include "main.h"
#include "hardware_vidor/usbblaster.h"
extern "C" {
#include "hardware/irq.h"
}

void setup() {

  Insert_SRAM_Sentinels();

  char buf[256];
  sprintf(buf, "FPGAArcade Replay VIDOR %s %s", __DATE__, __TIME__);

  Serial1.begin(115200);
  Serial1.println("\033[2J");
  Serial1.println("=========================================================");
  Serial1.println(buf);
  Serial1.println("=========================================================");
  Serial1.flush();

  USBBlaster_Disable();
}

void loop() {
  replay_main();
}

static __attribute__ ((noinline)) void Insert_SRAM_Sentinels()
{
  disableIRQ();

  const void* heap_end = sbrk(0);
  const void* stack_end = __builtin_frame_address(0);
  const uint32_t sentinel = 0xFA57F00D;

  uint32_t* start = (uint32_t*)((((intptr_t)heap_end)+3) & ~ 0x3);
  uint32_t*   end = (uint32_t*)((((intptr_t)stack_end) ) & ~ 0x3);

  while(start < end)
  {
    *start++ = sentinel;
  }

  enableIRQ();
}

__attribute__((naked)) void HardFault_Handler(void)
{
  /*
   * Get the appropriate stack pointer, depending on our mode,
   * and use it as the parameter to the C handler. This function
   * will never return
   *
   * Based on https://electronics.stackexchange.com/questions/293772/finding-the-source-of-a-hard-fault-using-extended-hardfault-handler
   */

 __asm(  ".syntax unified\n"
         "movs   r0, #4  \n"
         "mov    r1, lr  \n"
         "tst    r0, r1  \n"
         "beq    _msp    \n"
         "mrs    r0, psp \n"
         "b      PrintExceptionInfo \n"
         "_msp:  \n"
         "mrs    r0, msp \n"
         "b      PrintExceptionInfo \n"
         ".syntax divided\n") ;
}

extern "C" void PrintExceptionInfo(unsigned long* stack)
{
  static const char* regs[] = { "R0", "R1", "R2", "R3", "R12", "LR", "PC", "PSR", "SP" };

  kprintf("\n\r\n\r ** Hard Fault **\n\r\n\r");

  kprintf("***********************************************************************\n\r");
  kprintf("* %3s = %08x  %3s = %08x  %3s = %08x  %3s = %08x      *\n\r", regs[0], stack[0], regs[1], stack[1], regs[2], stack[2], regs[3], stack[3]);
  kprintf("* %3s = %08x  %3s = %08x  %3s = %08x  %3s = %08x      *\n\r", regs[4], stack[4], regs[5], stack[5], regs[6], stack[6], regs[7], stack[7]);
  kprintf("* %3s = %08x                                                      *\n\r", regs[8], &stack[0]);
  kprintf("***********************************************************************\n\r");

  for (int i = 0; i < 7; ++i)
  {
    const unsigned long rom_start = 0x00002000, rom_end = 0x00040000;
    const unsigned long ram_start = 0x20000000, ram_end = 0x20008000;
    unsigned long r = stack[i];

    if (rom_start <= r && r < rom_end ||
        ram_start <= r && r < ram_end)
    {
      kprintf("* @ %3s :                                                             *\n\r", regs[i]);
      kprintmem((const uint8_t*)(r & ~0x3) - 32, 64);
      kprintf("*                                                                     *\n\r");
    }
  }

  kprintf("* @ %3s:                                                              *\n\r", regs[8]);
  kprintmem((const uint8_t*)&stack[8], 512);
  kprintf("*                                                                     *\n\r");

  kprintf("***********************************************************************\n\r");

  pinMode(LED_BUILTIN, OUTPUT);
  while(true)
  {
   for (int i = 0; i < 100000; ++i)
     digitalWrite(LED_BUILTIN, HIGH);
   for (int i = 0; i < 100000; ++i)
     digitalWrite(LED_BUILTIN, LOW);
  }
}

static char buffer[256];
void kprintstr(const char* str)
{
  if (!str)
    return;
  while(*str)
  {
    sercom5.writeDataUART(*str++);
  }
}

extern "C" int kprintf(const char * fmt, ...)
{
  va_list args;
  va_start (args, fmt);
  int n = vsnprintf (buffer, sizeof(buffer)-1, fmt, args);
  if (0 < n && n < sizeof(buffer))
  {
    buffer[n] = 0;
    kprintstr(buffer);
  }
  va_end (args);
}

void kprintmem(const uint8_t* pBuffer, uint32_t size)
{
  uint32_t i, j, len;
  char format[150];
  char alphas[27];
  strcpy(format, "* 0x%08X: %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X ");

  for (i = 0; i < size; i += 16) {
    len = size - i;

        // last line is less than 16 bytes? rewrite the format string
    if (len < 16) {
      strcpy(format, "* 0x%08X: ");

      for (j = 0; j < 16; ++j) {
        if (j < len) {
          strcat(format, "%02X");

        } else {
          strcat(format, "__");
        }

        if ((j & 0x3) == 0x3) {
          strcat(format, " ");
        }
      }

    } else {
      len = 16;
    }

        // create the ascii representation
    for (j = 0; j < len; ++j) {
      alphas[j] = (isalnum(pBuffer[i + j]) ? pBuffer[i + j] : '.');
    }

    for (; j < 16; ++j) {
      alphas[j] = '_';
    }

    alphas[j] = 0;

    j = strlen(format);
    sprintf(format + j, "'%s'  *\n\r", alphas);

    kprintf(format, i + (intptr_t)pBuffer,
      pBuffer[i + 0], pBuffer[i + 1], pBuffer[i + 2], pBuffer[i + 3], pBuffer[i + 4], pBuffer[i + 5], pBuffer[i + 6], pBuffer[i + 7],
      pBuffer[i + 8], pBuffer[i + 9], pBuffer[i + 10], pBuffer[i + 11], pBuffer[i + 12], pBuffer[i + 13], pBuffer[i + 14], pBuffer[i + 15]);

    format[j] = '\0';
  }
}
