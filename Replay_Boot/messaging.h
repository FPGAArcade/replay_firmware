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

typedef enum {
    MSG_TYPE_DEBUG,
    MSG_TYPE_INFO,
    MSG_TYPE_WARN,
    MSG_TYPE_ERR,
} MsgType_t;

/// OPTIONAL DEBUGGING MESSAGES
#define DEBUG(lvl, ...) do { if (lvl<=debuglevel) MSG_output(MSG_TYPE_DEBUG, (const char*)__FILENAME__, (unsigned int)__LINE__, __VA_ARGS__); } while(0)

/// INFO MESSAGES, can't be disabled
#define INFO(...) do { MSG_output(MSG_TYPE_INFO, 0, 0, __VA_ARGS__); } while(0)

/// WARNING MESSAGES, can't be disabled
#define WARNING(...) do { MSG_output(MSG_TYPE_WARN, 0, 0, __VA_ARGS__); } while(0)

/// ERROR MESSAGES, can't be disabled
/// The system will HALT and display on the OSD. LED will flash on-off-on-off
#define ERROR(...) do { MSG_output(MSG_TYPE_ERR, 0, 0, __VA_ARGS__); } while(0)

/// For errors pre FPGA load. This will load the backup image to display the error message, and halt.
//#define ERROR_LOAD_DEFAULT(fmt...) MSG_error(fmt)

/// comment out to remove asserts
#define ASSERT_ENABLED (debuglevel > 0)

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

void MSG_output(MsgType_t type, const char* file, unsigned int line, const char* fmt, ...);

// ASSERT
#if ASSERT_ENABLED
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
