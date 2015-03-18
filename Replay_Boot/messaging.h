/** @file messaging.h */

#ifndef MESSAGING_H_INCLUDED
#define MESSAGING_H_INCLUDED

#include "config.h"
#include "osd.h"
#include "iniparser.h"

/// DEBUGLEVEL: 0...off, 1...basic, 2...most, 3...gory details
#define debuglevel 1

/// OSDLEVEL:   see also DEBUGLEVEL (but specific for additional OSD messaging)
#define osdlevel   0

/// OPTIONAL DEBUGGING MESSAGES
#define DEBUG(lvl, fmt...) if (lvl<=debuglevel) MSG_debug(lvl<=osdlevel,fmt);

/// INFO MESSAGES, can't be disabled
#define INFO(fmt...) MSG_info(fmt)

/// WARNING MESSAGES, can't be disabled
#define WARNING(fmt...) MSG_warning(fmt)

/// ERROR MESSAGES, can't be disabled
#define ERROR(fmt...) MSG_error(fmt)

/// comment out to remove asserts
//#define ASSERT 1

/** @brief INIT MESSAGE SYSTEM

    Set up structure required for OSD, call once after reset.

    @param status central replay status structure
    @param serial_on set to 1 to use serial output as well
*/
void MSG_init(status_t *status, uint8_t serial_on);

/** @brief SERIAL DEBUG MESSAGE

    Similar to printf, but ends up on USART only.
    Output is given as "DBG : <format>" on USART.

    @param do_osd set to 1 to print also on OSD
    @param fmt printf conform arguments
*/
void MSG_debug(uint8_t do_osd, char *fmt, ...);

/** @brief OSD/SERIAL INFO MESSAGE

    Similar to printf, but ends up on OSD info area and on USART.
    Output is given as "I <format>" on OSD.
    Output is given as "INFO <format>" on USART.

    @param fmt printf conform arguments
*/
void MSG_info(char *fmt, ...);

/** @brief OSD/SERIAL WARNING MESSAGE

    Similar to printf, but ends up on OSD info area and on USART.
    Output is given as "W:<format>".

    @param fmt printf conform arguments
*/
void MSG_warning(char *fmt, ...);

/** @brief OSD/SERIAL ERROR MESSAGE

    Similar to printf, but ends up on OSD info area and on USART.
    Output is given as "E:<format>".

    @param fmt printf conform arguments
*/
void MSG_error(char *fmt, ...);

// ASSERT
#ifdef ASSERT
  void AssertionFailure(char *exp, char *file, char *baseFile, int line);
  #define Assert(exp)  if (exp) ; \
        else AssertionFailure( #exp, __FILE__, __BASE_FILE__, __LINE__ )
#else
  #define Assert(exp)
#endif

#endif
