/*--------------------------------------------------------------------
 *                       Replay Firmware
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

/** @file messaging.h */

#ifndef MESSAGING_H_INCLUDED
#define MESSAGING_H_INCLUDED

#include "config.h"
#include "stringlight.h"

#define __FILENAME_BACKSLASH__ (__builtin_strrchr(__FILE__, '\\') ? __builtin_strrchr(__FILE__, '\\') + 1 : __FILE__)
#define __FILENAME__ (__builtin_strrchr(__FILENAME_BACKSLASH__, '/') ? __builtin_strrchr(__FILENAME_BACKSLASH__, '/') + 1 : __FILENAME_BACKSLASH__)

/// DEBUGLEVEL: 0...off, 1...basic, 2...most, 3...gory details
#define debuglevel 0

/// OSDLEVEL:   see also DEBUGLEVEL (but specific for additional OSD messaging)
#define osdlevel   0

/// OPTIONAL DEBUGGING MESSAGES
//#define DEBUG(lvl, fmt...) if (lvl<=debuglevel) MSG_debug(lvl<=osdlevel,fmt);
// dont display debug on OSD, only INFO
#define DEBUG(lvl, ...) do { if (lvl<=debuglevel) MSG_debug(0, (const char*)__FILENAME__, (unsigned int)__LINE__, __VA_ARGS__); } while(0)

/// INFO MESSAGES, can't be disabled
#define INFO(...) MSG_info(__VA_ARGS__)

/// WARNING MESSAGES, can't be disabled
#define WARNING(...) MSG_warning(__VA_ARGS__)

/// ERROR MESSAGES, can't be disabled
/// The system will HALT and display on the OSD. LED will flash on-off-on-off
#define ERROR(...) MSG_error(__VA_ARGS__)

/// For errors pre FPGA load. This will load the backup image to display the error message, and halt.
//#define ERROR_LOAD_DEFAULT(fmt...) MSG_error(fmt)

/// comment out to remove asserts
//#define ASSERT 1

/** @brief INIT MESSAGE SYSTEM

    Set up structure required for OSD, call once after reset.

    @param status central replay status structure
    @param serial_on set to 1 to use serial output as well
*/
void MSG_init(status_t* status, uint8_t serial_on);

/** @brief SERIAL DEBUG MESSAGE

    Similar to printf, but ends up on USART only.
    Output is given as "DBG : <format>" on USART.

    @param do_osd set to 1 to print also on OSD
    @param fmt printf conform arguments
*/

/** @brief flash LED and then reboot

*/
void MSG_fatal_error(uint8_t error);

void MSG_debug(uint8_t do_osd, const char* file, unsigned int line, const char* fmt, ...);

/** @brief OSD/SERIAL INFO MESSAGE

    Similar to printf, but ends up on OSD info area and on USART.
    Output is given as "I <format>" on OSD.
    Output is given as "INFO <format>" on USART.

    @param fmt printf conform arguments
*/
void MSG_info(const char* fmt, ...);

/** @brief OSD/SERIAL WARNING MESSAGE

    Similar to printf, but ends up on OSD info area and on USART.
    Output is given as "W:<format>".

    @param fmt printf conform arguments
*/
void MSG_warning(const char* fmt, ...);

/** @brief OSD/SERIAL ERROR MESSAGE

    Similar to printf, but ends up on OSD info area and on USART.
    Output is given as "E:<format>".

    @param fmt printf conform arguments
*/
void MSG_error(const char* fmt, ...);

// ASSERT
#ifdef ASSERT
void AssertionFailure(const char* exp, const char* file, const char* baseFile, int line);
#define Assert(exp)  if (exp) ; \
    else AssertionFailure( #exp, __FILE__, __BASE_FILE__, __LINE__ )
#else
#define Assert(exp)
#endif

#define STATIC_ASSERT(COND) typedef char static_assertion[(COND)?1:-1]

//
// Debug
//
void DumpBuffer(const uint8_t* pBuffer, uint32_t size);

#endif
