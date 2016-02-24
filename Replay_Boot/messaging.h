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
/** @file messaging.h */

#ifndef MESSAGING_H_INCLUDED
#define MESSAGING_H_INCLUDED

#include "config.h"
#include "osd.h"
#include "iniparser.h"

/// DEBUGLEVEL: 0...off, 1...basic, 2...most, 3...gory details
#define debuglevel 0

/// OSDLEVEL:   see also DEBUGLEVEL (but specific for additional OSD messaging)
#define osdlevel   0

/// OPTIONAL DEBUGGING MESSAGES
//#define DEBUG(lvl, fmt...) if (lvl<=debuglevel) MSG_debug(lvl<=osdlevel,fmt);
// dont display debug on OSD, only INFO
#define DEBUG(lvl, fmt...) do { if (lvl<=debuglevel) MSG_debug(0, (const char*)__FILE__, (unsigned int)__LINE__, fmt); } while(0)

/// INFO MESSAGES, can't be disabled
#define INFO(fmt...) MSG_info(fmt)

/// WARNING MESSAGES, can't be disabled
#define WARNING(fmt...) MSG_warning(fmt)

/// ERROR MESSAGES, can't be disabled
/// The system will HALT and display on the OSD. LED will flash on-off-on-off
#define ERROR(fmt...) MSG_error(fmt)

/// For errors pre FPGA load. This will load the backup image to display the error message, and halt.
//#define ERROR_LOAD_DEFAULT(fmt...) MSG_error(fmt)

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

/** @brief flash LED and then reboot

*/
void MSG_fatal_error(uint8_t error);

void MSG_debug(uint8_t do_osd, const char* file, unsigned int line, char *fmt, ...);

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
