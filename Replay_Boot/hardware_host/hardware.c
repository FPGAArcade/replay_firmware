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

#include "board.h"
#include "messaging.h"
#include "hardware/io.h"
#include "hardware/usart.h"

//
// General
//
void Hardware_Init(void)
{
    IO_Init();
}

#define printf(...) do {} while(0) //fprintf(stderr, __VA_ARGS__)

#include "hardware_host/io.c"
#include "hardware_host/irq.c"
#include "hardware_host/spi.c"
#include "hardware_host/ssc.c"
#include "hardware_host/timer.c"
#include "hardware_host/twi.c"
#include "hardware_host/usart.c"
#include "hardware_host/telnet_listen.c"
