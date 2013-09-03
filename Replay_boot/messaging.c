/** @file messaging.c */

#include"messaging.h"

static void _putcp(void* p,char c)
	{
	*(*((char**)p))++ = c;
	}

void MSG_debug(char *fmt, ...) 
{ char s[256]; // take "enough" size here, not to get any overflow problems...
  char *sp = &(s[0]);  // _putcp needs a pointer to the string...
  
  // process initial printf (nearly) w/o size limit...
  va_list argptr;
  va_start(argptr,fmt);
  tfp_format(&sp,_putcp,fmt,argptr);
  _putcp(&sp,0);
  va_end(argptr);

  // print on USART including CR/LF
  printf("DBG:  %s\r\n",s);
}

void MSG_info(status_t *status, char *fmt, ...) 
{ char s[256]; // take "enough" size here, not to get any overflow problems...
  char *sp = &(s[0]);  // _putcp needs a pointer to the string...
  
  // process initial printf (nearly) w/o size limit...
  va_list argptr;
  va_start(argptr,fmt);
  //sprintf(s,fmt,argptr);
  tfp_format(&sp,_putcp,fmt,argptr);
  _putcp(&sp,0);
  va_end(argptr);

  // cut string to visible range
  s[MAX_MENU_STRING-2]=0;

  // increment index and generate info line
  status->info_start_idx=status->info_start_idx>=7?0:status->info_start_idx+1;
  sprintf(status->info[status->info_start_idx],"I %s",s);
  status->update=1;

  // also print on USART including CR/LF
  printf("INFO: %s\r\n",s);
}

void MSG_warning(status_t *status, char *fmt, ...) 
{ char s[256]; // take "enough" size here, not to get any overflow problems...
  char *sp = &(s[0]);  // _putcp needs a pointer to the string...
  
  // process initial printf (nearly) w/o size limit...
  va_list argptr;
  va_start(argptr,fmt);
  //sprintf(s,fmt,argptr);
  tfp_format(&sp,_putcp,fmt,argptr);
  _putcp(&sp,0);
  va_end(argptr);

  // cut string to visible range
  s[MAX_MENU_STRING-2]=0;

  // increment index and generate info line
  status->info_start_idx=status->info_start_idx>=7?0:status->info_start_idx+1;
  sprintf(status->info[status->info_start_idx],"W %s",s);
  status->update=1;

  // also print on USART including CR/LF
  printf("WARN: %s\r\n",s);
}

void MSG_error(status_t *status, char *fmt, ...) 
{ char s[256]; // take "enough" size here, not to get any overflow problems...
  char *sp = &(s[0]);  // _putcp needs a pointer to the string...
  
  // process initial printf (nearly) w/o size limit...
  va_list argptr;
  va_start(argptr,fmt);
  //sprintf(s,fmt,argptr);
  tfp_format(&sp,_putcp,fmt,argptr);
  _putcp(&sp,0);
  va_end(argptr);

  // cut string to visible range
  s[MAX_MENU_STRING-2]=0;

  // increment index and generate info line
  status->info_start_idx=status->info_start_idx>=7?0:status->info_start_idx+1;
  sprintf(status->info[status->info_start_idx],"E %s",s);
  status->update=1;

  // also print on USART including CR/LF
  printf("ERR:  %s\r\n",s);
}
