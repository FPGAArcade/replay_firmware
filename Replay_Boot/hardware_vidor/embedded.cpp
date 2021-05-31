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

#define INCBIN_PREFIX _binary_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "../hardware_host/incbin.h"

// If the incbin's below fail, copy platform.local.txt
// (from <root>/Replay_Boot/platform.local.txt)
// to: (may need to adjust "1.8.11" to match future versions)
// Windows : %USERPROFILE%\AppData\Local\Arduino15\packages\arduino\hardware\samd\1.8.11
// macOS   : $HOME/Library/Arduino15/packages/arduino/hardware/samd/1.8.11

INCBIN(replay_ini, "../loader_embedded/replay.ini");

#include "board_driver_jtag.h"

extern "C" {
#include "../messaging.h"
}

/*

$ quartus_cpf.exe -c loader.sof loader.ttf
$ go run make_composite_binary.go -i loader.ttf:1:512 -o loader.h > signature.h

*/

#define no_data    0xFF, 0xFF, 0xFF, 0xFF, \
    0xFF, 0xFF, 0xFF, 0xFF, \
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, \
    0xFF, 0xFF, 0xFF, 0xFF, \
    0x00, 0x00, 0x00, 0x00  \

#define NO_BOOTLOADER   no_data
#define NO_APP        no_data
#define NO_USER_DATA    no_data

    __attribute__ ((used, section(".fpga_bitstream_signature")))
    const unsigned char signatures[4096] = {
#include "signature.h"
    };
    __attribute__ ((used, section(".fpga_bitstream")))
    const unsigned char bitstream[] = {
#include "loader.h"
    };

    extern "C" uint8_t pin_fpga_done;
    extern void enableFpgaClock();

    extern "C" void JTAG_StartEmbeddedCore()
    {
        enableFpgaClock();

        if (jtagInit() < 0) {
            ERROR("JTAG init failed!");
            return;
        }

        jtagBootApp();

        delay(1000);

        jtagDeinit();

        DEBUG(1, "Embedded core started.");
        pin_fpga_done = TRUE;
    }
