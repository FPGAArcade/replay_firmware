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

// hardware abstraction layer for Sam7S

#include "../board.h"
extern "C" {
#include "../messaging.h"
#include "../hardware.h"
#include "../hardware/io.h"
}

#include "usbhid.h"

//
// General
//
void Hardware_Init(void)
{
    IO_Init();
    USBHID_Init();
}

extern void enableFpgaClock(void);

extern "C" {
#include "../hardware_vidor/io.c"
#include "../hardware_vidor/irq.c"
#include "../hardware_vidor/ssc.c"
#include "../hardware_vidor/timer.c"
#include "../hardware_vidor/twi.c"
}
#include "../hardware_vidor/spi.cpp"
#include "../hardware_vidor/usart.cpp"
#include "../hardware_vidor/jtag.cpp"
#include "../hardware_vidor/msc_hardware.cpp"
#include "../hardware_vidor/usbhid.cpp"
#include "../hardware_vidor/usbblaster.cpp"
