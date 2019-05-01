
#include "../board.h"
#include "../hardware/io.h"
#include "../hardware/irq.h"

#include "../messaging.h"
#include "../hardware/timer.h"

uint8_t pin_fpga_init_l = TRUE;
uint8_t pin_fpga_done = FALSE;

void IO_DriveLow_OD_(uint32_t pin, const char* pin_name)
{
    DEBUG(3, "%s  : %08x (%s)", __FUNCTION__, pin, pin_name);
    digitalWrite(pin, LOW);
}

void IO_DriveHigh_OD_(uint32_t pin, const char* pin_name)
{
    DEBUG(3, "%s : %08x (%s)", __FUNCTION__, pin, pin_name);
    digitalWrite(pin, HIGH);
}

static uint8_t IO_Input_(uint32_t pin)  // return pin state high == TRUE / low == FALSE
{
    if (pin == PIN_CARD_DETECT) {
        return FALSE;

    } else if (pin == PIN_CODER_FITTED_L) {
        return TRUE;

    } else if (pin == PIN_MENU_BUTTON) {
        return TRUE;

    } else if (pin == PIN_FPGA_DONE) {
        return pin_fpga_done;

    } else if (pin == PIN_FPGA_INIT_L) {
        return pin_fpga_init_l;

    } else {
        return digitalRead(pin);
    }
}

uint8_t IO_Input_H_(uint32_t pin, const char* pin_name)  // returns true if pin high
{
    uint8_t v = IO_Input_(pin);
    DEBUG(3, "%s : %08x (%s) => %s", __FUNCTION__, pin, pin_name, v ? "TRUE" : "FALSE");
    return v;
}

uint8_t IO_Input_L_(uint32_t pin, const char* pin_name)  // returns true if pin low
{
    uint8_t v = !IO_Input_(pin);
    DEBUG(3, "%s : %08x (%s) => %s", __FUNCTION__, pin, pin_name, v ? "TRUE" : "FALSE");
    return v;
}

void ISR_VerticalBlank();       // a bit hacky - just assume this is implemented _somewhere_
extern volatile uint32_t vbl_counter;
void IO_WaitVBL(void)
{
    uint32_t irq_old = LOW;
    uint32_t irq = digitalRead(PIN_CONF_DOUT);
    uint32_t vbl = vbl_counter;
    HARDWARE_TICK timeout = Timer_Get(100);

    while (!(!irq && irq_old) && vbl != vbl_counter) {
        irq_old = irq;
        irq = digitalRead(PIN_CONF_DOUT);

        if (Timer_Check(timeout)) {
            WARNING("IO_WaitVBL timeout");
            break;
        }
    }
}

void IO_Init(void)
{
    pinMode(PIN_ACT_LED, OUTPUT);

    pinMode(PIN_FPGA_RST_L, OUTPUT);
    digitalWrite(PIN_FPGA_RST_L, HIGH);

    attachInterrupt(PIN_CONF_DOUT, ISR_VerticalBlank, FALLING);
}

void IO_ClearOutputData_(uint32_t pins, const char* pin_names)
{
    DEBUG(3, "%s : %08x (%s)", __FUNCTION__, pins, pin_names);
}
void IO_SetOutputData_(uint32_t pins, const char* pin_names)
{
    DEBUG(3, "%s : %08x (%s)", __FUNCTION__, pins, pin_names);
}
