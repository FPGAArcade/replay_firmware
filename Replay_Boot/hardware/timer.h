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

#ifndef HARDWARE_TIMER_H_INCLUDED
#define HARDWARE_TIMER_H_INCLUDED

#include <stdint.h>

typedef uint32_t HARDWARE_TICK;						// system timer type

void Timer_Init(void);

HARDWARE_TICK Timer_Get(uint32_t time_ms);			// returns hardware ticks relative to cpu frequency
uint8_t Timer_Check(HARDWARE_TICK offset);			// return TRUE/FALSE if the number of hardware ticks has elapsed
void Timer_Wait(uint32_t time_ms);					// busy-wait for 'time_ms' milliseconds

// uint32_t Timer_Convert(HARDWARE_TICK offset);
#define Timer_Convert(offset) ((uint32_t)(offset))	// HARDWARE_TICK is in millisecond resolution

#endif
