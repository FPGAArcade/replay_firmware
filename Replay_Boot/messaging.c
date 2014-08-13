/** @file messaging.c */

#include "hardware.h" // actled
#include "messaging.h"

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

void MSG_debug(uint8_t do_osd, char *fmt, ...)
{ char s[256]; // take "enough" size here, not to get any overflow problems...
  char *sp = &(s[0]);  // _MSG_putcp needs a pointer to the string...

  // process initial printf (nearly) w/o size limit...
  va_list argptr;
  va_start(argptr,fmt);
  tfp_format(&sp,_MSG_putcp,fmt,argptr);
  _MSG_putcp(&sp,0);
  va_end(argptr);

  // print on USART including CR/LF if enabled
  if (msg_serial) printf("DBG:  %s\r\n",s);

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
  int i;

  // process initial printf (nearly) w/o size limit...
  va_list argptr;
  va_start(argptr,fmt);
  //sprintf(s,fmt,argptr);
  tfp_format(&sp,_MSG_putcp,fmt,argptr);
  _MSG_putcp(&sp,0);
  va_end(argptr);

  // print on USART including CR/LF if enabled
  if (msg_serial) printf("ERR:  %s\r\n",s);

  // if the status structure pointer is set (OSD available), print it there
  if (msg_status) _MSG_to_osd(s,'E');

  for (i=0;i<4;i++) {
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
