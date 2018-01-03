
#include "board.h"
#include "hardware/io.h"
#include "hardware/irq.h"

void IO_DriveLow_OD(uint32_t pin)
{
    // we assume for OD pins the pin is already assigned to PIO and cleared
    AT91C_BASE_PIOA->PIO_OER = pin;
}

void IO_DriveHigh_OD(uint32_t pin)
{
    AT91C_BASE_PIOA->PIO_ODR = pin;
}

uint8_t IO_Input_H(uint32_t pin)  // returns true if pin high
{
    volatile uint32_t read;
    read = AT91C_BASE_PIOA->PIO_PDSR;

    if (!(read & pin)) {
        return FALSE;

    } else {
        return TRUE;
    }
}

uint8_t IO_Input_L(uint32_t pin)  // returns true if pin low
{
    volatile uint32_t read;
    read = AT91C_BASE_PIOA->PIO_PDSR;

    if (!(read & pin)) {
        return TRUE;

    } else {
        return FALSE;
    }
}

void ISR_VerticalBlank();       // a bit hacky - just assume this is implemented _somewhere_
static void ISR_io(void)
{
    if (AT91C_BASE_PIOA->PIO_ISR & PIN_CONF_DOUT) {
        if ((AT91C_BASE_PIOA->PIO_PDSR & PIN_CONF_DOUT) == 0) { // falling edge (active low)
            ISR_VerticalBlank();
        }
    }

    AT91C_BASE_AIC->AIC_ICCR = (1 << AT91C_ID_PIOA);
    AT91C_BASE_AIC->AIC_EOICR = 0;
}

void IO_Init(void)
{
    void (*vector)(void) = &ISR_io;

    disableIRQ();
    AT91C_BASE_AIC->AIC_IDCR = (1 << AT91C_ID_PIOA);
    AT91C_BASE_AIC->AIC_SMR[AT91C_ID_PIOA] = 0;
    AT91C_BASE_AIC->AIC_SVR[AT91C_ID_PIOA] = (int32_t)vector;

    AT91C_BASE_PIOA->PIO_IER = PIN_CONF_DOUT; // vbl
    AT91C_BASE_PIOA->PIO_IFER = PIN_CONF_DOUT; // glitch

    AT91C_BASE_AIC->AIC_ICCR = (1 << AT91C_ID_PIOA);
    AT91C_BASE_AIC->AIC_IECR = (1 << AT91C_ID_PIOA);
    enableIRQ();
}
