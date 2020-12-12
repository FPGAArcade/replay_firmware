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
#include "timer.h"
#include "irq.h"

static volatile HARDWARE_TICK s_milliseconds = 0;
static void ISR_System(void)
{
    if (AT91C_BASE_PITC->PITC_PISR & AT91C_PITC_PITS) {
        s_milliseconds += (AT91C_BASE_PITC->PITC_PIVR & AT91C_PITC_PICNT) >> 20;
    }

    AT91C_BASE_AIC->AIC_ICCR = (1 << AT91C_ID_SYS);
    AT91C_BASE_AIC->AIC_EOICR = 0;
}
static inline HARDWARE_TICK GetSystemTime()
{
    // normally AT91C_PITC_PITS will read 0, unless interrupts are disabled..
    if (AT91C_BASE_PITC->PITC_PISR & AT91C_PITC_PITS) {
        s_milliseconds += (AT91C_BASE_PITC->PITC_PIVR & AT91C_PITC_PICNT) >> 20;
    }

    return s_milliseconds;
}

//
// Timer
//
void Timer_Init(void)
{
    void (*vector)(void) = &ISR_System;

    disableIRQ();   // is this really necessary?
    AT91C_BASE_AIC->AIC_IDCR = (1 << AT91C_ID_SYS);

    AT91C_BASE_PITC->PITC_PIMR = AT91C_PITC_PITEN | ((BOARD_MCK / 16 / 1000 - 1) & AT91C_PITC_PIV) | AT91C_PITC_PITIEN; //counting period 1ms

    AT91C_BASE_AIC->AIC_SMR[AT91C_ID_SYS] = 0;
    AT91C_BASE_AIC->AIC_SVR[AT91C_ID_SYS] = (int32_t)vector;

    s_milliseconds = AT91C_BASE_PITC->PITC_PIVR >> 20;  // dummy read / clear values
    s_milliseconds = 0;

    AT91C_BASE_AIC->AIC_ICCR = (1 << AT91C_ID_SYS);
    AT91C_BASE_AIC->AIC_IECR = (1 << AT91C_ID_SYS);
    enableIRQ();
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
    return (offset > (1 << 31));
}

void Timer_Wait(uint32_t time_ms)
{
    HARDWARE_TICK tick = Timer_Get(time_ms);

    while (!Timer_Check(tick));
}
