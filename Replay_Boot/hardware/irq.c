
#include "irq.h"

//
// IRQ
//

#if defined(AT91SAM7S256)

#define IRQ_MASK 0x00000080

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

#endif // defined(AT91SAM7S256)
