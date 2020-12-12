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

#include "irq.h"

//
// IRQ
//

#if defined(AT91SAM7S256)

#define IRQ_MASK 0x00000080

#if !defined(__thumb__)
static inline unsigned __get_cpsr(void)
{
    unsigned long retval;
    asm volatile (" mrs  %0, cpsr" : "=r" (retval) : /* no inputs */  );
    return retval;
}

static inline void __set_cpsr(unsigned val)
{
    asm volatile (" msr  cpsr, %0" : /* no outputs */ : "r" (val)  );
}

unsigned disableIRQ(void)
{
    unsigned _cpsr;

    _cpsr = __get_cpsr();
    __set_cpsr(_cpsr | IRQ_MASK);
    return _cpsr;
}

unsigned enableIRQ(void)
{
    unsigned _cpsr;

    _cpsr = __get_cpsr();
    __set_cpsr(_cpsr & ~IRQ_MASK);
    return _cpsr;
}
#endif // !defined(__thumb__)

#endif // defined(AT91SAM7S256)
