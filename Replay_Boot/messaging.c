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
/** @file messaging.c */

#include "hardware.h" // actled
#include "messaging.h"
#include "fpga.h"
#include "menu.h"

/** @brief status pointer for messaging via OSD

  We use this pointer only for the messaging functions,
  it needs to be initialized once at startup via MSG_init()!
*/
static status_t *msg_status=NULL;

/** @brief flag for messaging via serial

  We use this flag to enable serial messaging functions,
  it needs to be initialized once at startup via MSG_init()!
*/
static uint8_t msg_serial=0;

void MSG_init(status_t *status, uint8_t serial_on)
{
  msg_status=status;
  msg_serial=serial_on;
}

static void _MSG_putcp(void* p,char c)
{
  *(*((char**)p))++ = c;
}

static void _MSG_to_osd(char *s,char t)
{
  // cut string to visible range
  s[MAX_MENU_STRING-2]=0;

  // increment index and generate info line
  msg_status->info_start_idx=(msg_status->info_start_idx>=7)?
                                               0:msg_status->info_start_idx+1;
  sprintf(msg_status->info[msg_status->info_start_idx],"%c %s",t,s);
  msg_status->update=1;
}

// probably the only place this should be called is if the fallback FPGA image fails to load
void MSG_fatal_error(uint8_t error)
{
  uint8_t i,j;
  for (j=0; j<5; j++) {
    for (i = 0; i < error; i++) {
      ACTLED_ON;
      Timer_Wait(250);
      ACTLED_OFF;
      Timer_Wait(250);
    }
    Timer_Wait(1000);
  }

  // make sure FPGA is held in reset
  IO_DriveLow_OD(PIN_FPGA_RST_L);
  // set PROG low to reset FPGA (open drain)
  IO_DriveLow_OD(PIN_FPGA_PROG_L);
  Timer_Wait(1);
  IO_DriveHigh_OD(PIN_FPGA_PROG_L);
  Timer_Wait(2);
  // perform a ARM reset
  asm("ldr r3, = 0x00000000\n");
  asm("bx  r3\n");

}

void MSG_debug(uint8_t do_osd, const char* file, unsigned int line, char *fmt, ...)
{ char s[256]; // take "enough" size here, not to get any overflow problems...
  char *sp = &(s[0]);  // _MSG_putcp needs a pointer to the string...

  // process initial printf (nearly) w/o size limit...
  va_list argptr;
  va_start(argptr,fmt);
  tfp_format(&sp,_MSG_putcp,fmt,argptr);
  _MSG_putcp(&sp,0);
  va_end(argptr);

  // print on USART including CR/LF if enabled
  if (msg_serial) printf("DBG: [%s:%d] %s\r\n",file, line, s);

  // optional OSD print (check also for set up of required status structure)
  if (do_osd && msg_status) _MSG_to_osd(s,'D');
}

void MSG_info(char *fmt, ...)
{ char s[256]; // take "enough" size here, not to get any overflow problems...
  char *sp = &(s[0]);  // _MSG_putcp needs a pointer to the string...

  // process initial printf (nearly) w/o size limit...
  va_list argptr;
  va_start(argptr,fmt);
  //sprintf(s,fmt,argptr);
  tfp_format(&sp,_MSG_putcp,fmt,argptr);
  _MSG_putcp(&sp,0);
  va_end(argptr);

  // print on USART including CR/LF if enabled
  if (msg_serial) printf("INFO: %s\r\n",s);

  // if the status structure pointer is set (OSD available), print it there
  if (msg_status) _MSG_to_osd(s,'I');
}

void MSG_warning(char *fmt, ...)
{ char s[256]; // take "enough" size here, not to get any overflow problems...
  char *sp = &(s[0]);  // _MSG_putcp needs a pointer to the string...

  // process initial printf (nearly) w/o size limit...
  va_list argptr;
  va_start(argptr,fmt);
  //sprintf(s,fmt,argptr);
  tfp_format(&sp,_MSG_putcp,fmt,argptr);
  _MSG_putcp(&sp,0);
  va_end(argptr);

  // print on USART including CR/LF if enabled
  if (msg_serial) printf("WARN: %s\r\n",s);

  // if the status structure pointer is set (OSD available), print it there
  if (msg_status) _MSG_to_osd(s,'W');
}


void MSG_error(char *fmt, ...)
{ char s[256]; // take "enough" size here, not to get any overflow problems...
  char *sp = &(s[0]);  // _MSG_putcp needs a pointer to the string...
  //int i;

  // process initial printf (nearly) w/o size limit...
  va_list argptr;
  va_start(argptr,fmt);
  //sprintf(s,fmt,argptr);
  tfp_format(&sp,_MSG_putcp,fmt,argptr);
  _MSG_putcp(&sp,0);
  va_end(argptr);

  // print on USART including CR/LF if enabled
  if (msg_serial) printf("ERR:  %s\r\n",s);

  if (1) { // set up default FPGA always for now
    IO_DriveLow_OD(PIN_FPGA_RST_L);

    CFG_vid_timing_HD27(F60HZ);
    CFG_set_coder(CODER_DISABLE);

    FPGA_Default();

    IO_DriveHigh_OD(PIN_FPGA_RST_L);
    Timer_Wait(200);

    OSD_Reset(OSDCMD_CTRL_RES|OSDCMD_CTRL_HALT);
    CFG_set_CH7301_HD();

    // dynamic/static setup bits
    OSD_ConfigSendUserS(0x00000000);
    OSD_ConfigSendUserD(0x00000060); // 60HZ progressive

    OSD_Reset(OSDCMD_CTRL_RES);
    WARNING("Using hardcoded fallback!");
  }


  if (msg_status) {
  // if the status structure pointer is set (OSD available), print it there

     MENU_init_ui(msg_status);

     OSD_Reset(0);
     Timer_Wait(100);

     WARNING("Error during core boot");
     WARNING("Check the .ini file");
     _MSG_to_osd(s,'E');

     msg_status->show_status=1;
     msg_status->update=1;

     MENU_handle_ui(0,msg_status);

     OSD_Enable(DISABLE_KEYBOARD);
  }

  while (1) {
    ACTLED_OFF;
    Timer_Wait(150);
    ACTLED_ON;
    Timer_Wait(150);
    ACTLED_OFF;
    Timer_Wait(150);
    ACTLED_ON;
    Timer_Wait(150);
    ACTLED_OFF;
    Timer_Wait(300);
  }

  /*
  // make sure FPGA is held in reset
  IO_DriveLow_OD(PIN_FPGA_RST_L);
  // set PROG low to reset FPGA (open drain)
  IO_DriveLow_OD(PIN_FPGA_PROG_L);
  Timer_Wait(1);
  IO_DriveHigh_OD(PIN_FPGA_PROG_L);
  Timer_Wait(2);
  // perform a ARM reset
  asm("ldr r3, = 0x00000000\n");
  asm("bx  r3\n");
  */
}

#ifdef ASSERT

void AssertionFailure(char *exp, char *file, char *baseFile, int line)
{
    if (!strcmp(file, baseFile)) {
      if (msg_serial) printf("Assert(%s) failed in file %s, line %d\r\n", exp, file, line);
    } else {
      if (msg_serial) printf("Assert(%s) failed in file %s (included from %s), line %d\r\n",
        exp, file, baseFile, line);
    }
}

#endif

