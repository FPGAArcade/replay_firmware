
#include "../board.h"
#include "../hardware/io.h"
#include "../hardware/irq.h"

#include "../messaging.h"
#include "../hardware/timer.h"

void IO_DriveLow_OD(uint32_t pin)
{
    printf("%s : %08x\n", __FUNCTION__, pin);
}

void IO_DriveHigh_OD(uint32_t pin)
{
    printf("%s : %08x\n", __FUNCTION__, pin);
}

uint8_t IO_Input_H(uint32_t pin)  // returns true if pin high
{
    printf("%s : %08x\n", __FUNCTION__, pin);

    return FALSE;
}

uint8_t IO_Input_L(uint32_t pin)  // returns true if pin low
{
    printf("%s : %08x\n", __FUNCTION__, pin);

    return FALSE;
}

void ISR_VerticalBlank();       // a bit hacky - just assume this is implemented _somewhere_
static volatile uint32_t vbl_counter = 0;
static void* ISR_io(void* param)
{
    (void)param;
    return NULL;
}

void IO_Init(void)
{
}

void IO_ClearOutputDataX(const char* pin)
{
    printf("%s : %s\n", __FUNCTION__, pin);
}
void IO_SetOutputDataX(const char* pin)
{
    printf("%s : %s\n", __FUNCTION__, pin);
}

void IO_WaitVBL(void)
{
}
