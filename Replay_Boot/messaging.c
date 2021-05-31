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

/** @file messaging.c */

#include "board.h" // actled
#include "hardware/io.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "messaging.h"
#include "fpga.h"
#include "menu.h"
#include "osd.h"
#include "printf.h"

#if defined(ARDUINO_SAMD_MKRVIDOR4000)
#include "Reset.h"
#include <setjmp.h>
extern jmp_buf exit_env;
#endif

/** @brief status pointer for messaging via OSD

  We use this pointer only for the messaging functions,
  it needs to be initialized once at startup via MSG_init()!
*/
static status_t* msg_status = NULL;

/** @brief flag for messaging via serial

  We use this flag to enable serial messaging functions,
  it needs to be initialized once at startup via MSG_init()!
*/
static uint8_t msg_serial = 0;

void MSG_init(status_t* status, uint8_t serial_on)
{
    msg_status = status;
    msg_serial = serial_on;
}

static void _MSG_putcp(void* p, char c)
{
    *(*((char**)p))++ = c;
}

static void _MSG_to_osd(char* s, char t)
{
    const uint8_t last_idx = msg_status->info_start_idx;
    // cut string to visible range
    s[MAX_MENU_STRING - 3] = 0;

    // prevent duplicated/repeated messages
    if (!strcmp(msg_status->info[last_idx] + 2, s)) {
        return;
    }

    // increment index and generate info line
    msg_status->info_start_idx = (msg_status->info_start_idx >= 7) ?
                                 0 : msg_status->info_start_idx + 1;
    sprintf(msg_status->info[msg_status->info_start_idx], "%c %s", t, s);
    msg_status->update = 1;
}

// probably the only place this should be called is if the fallback FPGA image fails to load
void MSG_fatal_error(uint8_t error)
{
    uint8_t i, j;

    for (j = 0; j < 5; j++) {
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

    // hack! disable interrupts to not hang during reboot
    IRQ_DisableAllInterrupts();

    // perform a ARM reset
#if defined(ARDUINO_SAMD_MKRVIDOR4000)
    longjmp(exit_env, error);

    initiateReset(1);           // reboot in 'factory' mode (not reached)
    tickReset();

#elif defined(__arm__)
    asm("ldr r3, = 0x00000000\n");
    asm("bx  r3\n");
#else
    {
        int* p = 0;
        *p = 3;     // crash
    }
#endif

}

void MSG_output(MsgType_t type, const char* file, unsigned int line, const char* fmt, ...)
{
    char s[256]; // take "enough" size here, not to get any overflow problems...
    char* sp = &(s[0]);  // _MSG_putcp needs a pointer to the string...
    uint32_t timestamp = Timer_Convert(Timer_Get(0));
    uint32_t timestamp_s = timestamp / 1000;
    uint32_t timestamp_fraction = timestamp - timestamp_s * 1000;
    // process initial printf (nearly) w/o size limit...
    va_list argptr;
    va_start(argptr, fmt);
    tfp_format(&sp, _MSG_putcp, fmt, argptr);
    _MSG_putcp(&sp, 0);
    va_end(argptr);

    static const char* typeStr[] = {
        "DEBUG",    // MSG_TYPE_DEBUG
        "INFO",     // MSG_TYPE_INFO
        "WARN",     // MSG_TYPE_WARN
        "ERR",      // MSG_TYPE_ERR
    };

    const char* prefix = (MSG_TYPE_DEBUG <= type && type <= MSG_TYPE_ERR) ? typeStr[type] : "";

    // print on USART including CR/LF if enabled
    if (msg_serial) {
        if (file && line) {
            printf("%d.%03d: [%s:%d] %s\r\n", timestamp_s, timestamp_fraction, file, line, s);

        } else {
            printf("%d.%03d: [%s] %s\r\n", timestamp_s, timestamp_fraction, prefix, s);
        }
    }

    // optional OSD print (check also for set up of required status structure)
    if (msg_status) {
        if (type == MSG_TYPE_ERR) {
            // if the status structure pointer is set (OSD available), print it there

            MENU_init_ui(msg_status);

            OSD_Reset(0);
            Timer_Wait(100);

            WARNING("Error during core boot");
            WARNING("Check the .ini file");
            _MSG_to_osd(s, 'E');

            MENU_set_state(msg_status, SHOW_STATUS);
            msg_status->update = 1;

            MENU_handle_ui(0, msg_status);

            OSD_Enable(DISABLE_KEYBOARD);

        } else if (type != MSG_TYPE_DEBUG) {
            _MSG_to_osd(s, prefix[0]);
        }
    }
}

#if ASSERT_ENABLED

void AssertionFailure(const char* exp, const char* file, const char* baseFile, int line)
{
    if (!strcmp(file, baseFile)) {
        if (msg_serial) {
            printf("Assert(%s) failed in file %s, line %d\r\n", exp, file, line);
        }

    } else {
        if (msg_serial) printf("Assert(%s) failed in file %s (included from %s), line %d\r\n",
                                   exp, file, baseFile, line);
    }

    ERROR("Assert(%s) failed", exp);
}

#endif


//
// Debug
//
void DumpBuffer(const uint8_t* pBuffer, uint32_t size)
{
    DEBUG(1, "DumpBuffer: address = %08x ; size = %d (0x%x) bytes", pBuffer, size, size);
    uint32_t i, j, len;
    char format[150];
    char alphas[27];
    strcpy(format, "\t0x%08X: %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X ");

    for (i = 0; i < size; i += 16) {
        len = size - i;

        // last line is less than 16 bytes? rewrite the format string
        if (len < 16) {
            strcpy(format, "\t0x%08X: ");

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
        tfp_sprintf(format + j, "'%s'", alphas);

        DEBUG(1, format, i,
              pBuffer[i + 0], pBuffer[i + 1], pBuffer[i + 2], pBuffer[i + 3], pBuffer[i + 4], pBuffer[i + 5], pBuffer[i + 6], pBuffer[i + 7],
              pBuffer[i + 8], pBuffer[i + 9], pBuffer[i + 10], pBuffer[i + 11], pBuffer[i + 12], pBuffer[i + 13], pBuffer[i + 14], pBuffer[i + 15]);

        format[j] = '\0';
    }
}
