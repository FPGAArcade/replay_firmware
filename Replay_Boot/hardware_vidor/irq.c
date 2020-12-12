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

#include "../hardware/irq.h"

//
// IRQ
//

void IRQ_DisableAllInterrupts(void)
{
}

extern "C" unsigned disableIRQ(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    __DSB();		// completes all explicit memory accesses
    return mask;
}

extern "C" unsigned enableIRQ(void)
{
    uint32_t mask = __get_PRIMASK();
    __enable_irq();
    __ISB();		// flushes the pipeline
    return mask;
}
