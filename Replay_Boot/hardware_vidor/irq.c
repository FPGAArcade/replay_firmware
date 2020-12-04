// ADD REPLAY LICENSE!

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
