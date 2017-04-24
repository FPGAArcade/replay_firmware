
#include "board.h"
#include "hardware/io.h"

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
