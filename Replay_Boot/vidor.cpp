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

#include "board.h"

#if defined(ARDUINO_SAMD_MKRVIDOR4000)

#include "hardware_vidor/embedded.cpp"
#include "hardware_vidor/hardware.cpp"

extern "C" {
#include "hardware_vidor/board_driver_jtag.c"
#include "usb/msc.c"
}

//#include "filesys/ff_format.c"

#endif
