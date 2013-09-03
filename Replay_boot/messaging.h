/** @file messaging.h */

#ifndef MESSAGING_H_INCLUDED
#define MESSAGING_H_INCLUDED

#include"config.h"
#include"osd.h"
#include"iniparser.h"

// DEBUGLEVEL: 0...off, 1...basic, 2...most, 3...gory details
#define debuglevel 2
#define osdlevel   0

//#define DEBUG(lvl, fmt...)
#if osdlevel>0
  // SERIAL + OSD DEBUGGING
  #define DEBUG(lvl, fmt...) {        \
    if (lvl<=debuglevel) printf(fmt); \
    if (lvl<=osdlevel) {              \
      char str[OSDMAXLEN<<1];         \
      sprintf(str,fmt);               \
      str[OSDMAXLEN]=0;               \
      OSD_BootPrint(str);             \
    }                                 \
  }
  // SERIAL + OSD ERROR MESSAGES, can't be disabled
  #define ERROR(fmt...) {             \
    printf(fmt);                      \
    char str[OSDMAXLEN<<1];           \
    sprintf(str,fmt);                 \
    str[OSDMAXLEN]=0;                 \
    OSD_BootPrint(str);               \
  }
#else
  // SERIAL DEBUGGING
  #define DEBUG(lvl, fmt...) {        \
    if (lvl<=debuglevel) MSG_debug(fmt); \
  }
  // SERIAL ERROR MESSAGES, can't be disabled
  #define ERROR(fmt...) printf(fmt)
#endif

/** @brief SERIAL DEBUG MESSAGE

    Similar to printf, but ends up on USART only.
    Output is given as "DBG : <format>" on USART.

    @param fmt printf conform arguments
*/
void MSG_debug(char *fmt, ...);

/** @brief OSD/SERIAL INFO MESSAGE

    Similar to printf, but ends up on OSD info area and on USART.
    Output is given as "I <format>" on OSD.
    Output is given as "INFO <format>" on USART.

    @param status structure containing configuration/status structure
    @param fmt printf conform arguments
*/
void MSG_info(status_t *status, char *fmt, ...);

/** @brief OSD/SERIAL WARNING MESSAGE

    Similar to printf, but ends up on OSD info area and on USART.
    Output is given as "W:<format>".

    @param status structure containing configuration/status structure
    @param fmt printf conform arguments
*/
void MSG_warning(status_t *status, char *fmt, ...);

/** @brief OSD/SERIAL ERROR MESSAGE

    Similar to printf, but ends up on OSD info area and on USART.
    Output is given as "E:<format>".

    @param status structure containing configuration/status structure
    @param fmt printf conform arguments
*/
void MSG_error(status_t *status, char *fmt, ...);

#endif
