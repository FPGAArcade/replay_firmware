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

#include "../board.h"
#include "../hardware/timer.h"
#include "../hardware/irq.h"

static volatile uint64_t s_milliseconds = 0;

static inline HARDWARE_TICK GetSystemTime()
{
    uint64_t current_ms = millis();
    static uint8_t first = 0;

    if (!first) {
        first = 1;
        s_milliseconds = current_ms;
    }

    return (HARDWARE_TICK)((current_ms - s_milliseconds));
}

//
// Timer
//
void Timer_Init(void)
{
}

HARDWARE_TICK Timer_Get(uint32_t time_ms)
{
    HARDWARE_TICK systimer = GetSystemTime();
    systimer += time_ms;       // HACK - we know hw ticks match systimer
    return (systimer);
}

uint8_t Timer_Check(HARDWARE_TICK offset)
{
    uint32_t systimer = GetSystemTime();
    /*calculate difference*/
    offset -= systimer;
    /*check if <t> has passed*/
    return (offset > (HARDWARE_TICK)(1 << 31));
}

void Timer_Wait(uint32_t time_ms)
{
    HARDWARE_TICK tick = Timer_Get(time_ms);

    while (!Timer_Check(tick));
}
