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

#ifndef HARDWARE_IRQ_H_INCLUDED
#define HARDWARE_IRQ_H_INCLUDED

#include "../board.h"

unsigned disableIRQ(void);
unsigned enableIRQ(void);

#if defined(AT91SAM7S256)
static inline void IRQ_DisableAllInterrupts(void)
{
    AT91C_BASE_AIC->AIC_IDCR = AT91C_ALL_INT;
}
#else
void IRQ_DisableAllInterrupts(void);
#endif

#endif
