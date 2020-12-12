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
#include "hardware/io.h"
#include "hardware/irq.h"

#include "messaging.h"
#include "hardware/timer.h"
#include <string.h>
#include <pthread.h>
#include <unistd.h>

uint8_t pin_fpga_prog_l = FALSE;
uint8_t pin_fpga_init_l = TRUE;
uint8_t pin_fpga_done = FALSE;

void IO_DriveLow_OD(const char* pin)
{
    if (!strcmp(pin, "PIN_FPGA_PROG_L")) {
        pin_fpga_prog_l = TRUE;
        pin_fpga_done = FALSE;
    }

    printf("%s : %s\n", __FUNCTION__, pin);
}

void IO_DriveHigh_OD(const char* pin)
{
    if (!strcmp(pin, "PIN_FPGA_PROG_L")) {
        pin_fpga_prog_l = FALSE;
    }

    printf("%s : %s\n", __FUNCTION__, pin);
}

uint8_t IO_Input_H(const char* pin)  // returns true if pin high
{
    printf("%s : %s\n", __FUNCTION__, pin);

    if (!strcmp(pin, "PIN_FPGA_DONE")) {
        return pin_fpga_done;
    }

    return FALSE;
}

uint8_t IO_Input_L(const char* pin)  // returns true if pin low
{
    printf("%s : %s\n", __FUNCTION__, pin);

    if (!strcmp(pin, "PIN_FPGA_INIT_L")) {
        return pin_fpga_init_l;
    }

    return FALSE;
}

void ISR_VerticalBlank();       // a bit hacky - just assume this is implemented _somewhere_
static volatile uint32_t vbl_counter = 0;
static void* ISR_io(void* param)
{
    (void)param;
    HARDWARE_TICK cur, last = Timer_Get(0);
    uint32_t deltaTime;

    while (1) {
        ISR_VerticalBlank();
        vbl_counter++;

        // simple fake approx 60Hz
        do {
            cur = Timer_Get(0);
            deltaTime = Timer_Convert(cur - last);
            usleep(1000);
        } while (deltaTime <= 16);

        last = cur;
    }

    return NULL;
}

void IO_Init(void)
{
    pthread_t fake_vbl;
    pthread_create(&fake_vbl, NULL, ISR_io, NULL);
}

void IO_ClearOutputDataX(const char* pin)
{

}
void IO_SetOutputDataX(const char* pin)
{

}

void IO_WaitVBL(void)
{
    volatile uint32_t cur = vbl_counter;

    while ((vbl_counter - cur) < 1) {
        usleep(1000);
    }
}
