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

#ifndef HARDWARE_IO_H_INCLUDED
#define HARDWARE_IO_H_INCLUDED

#if defined(AT91SAM7S256)

#include "hardware/timer.h"
#include "messaging.h"	// ugly.. 

void IO_DriveLow_OD(uint32_t pin);
void IO_DriveHigh_OD(uint32_t pin);
uint8_t IO_Input_H(uint32_t pin);
uint8_t IO_Input_L(uint32_t pin);
static inline void IO_ClearOutputData(uint32_t pin)
{
    AT91C_BASE_PIOA->PIO_CODR = pin;
}
static inline void IO_SetOutputData(uint32_t pin)
{
    AT91C_BASE_PIOA->PIO_SODR = pin;
}

extern volatile uint32_t vbl_counter;
static inline void IO_WaitVBL(void)
{
    uint32_t pioa_old = 0;
    uint32_t pioa = AT91C_BASE_PIOA->PIO_PDSR;
    uint32_t vbl = vbl_counter;
    HARDWARE_TICK timeout = Timer_Get(100);

    while (!(~pioa & pioa_old & PIN_CONF_DOUT) && vbl != vbl_counter) {
        pioa_old = pioa;
        pioa     = AT91C_BASE_PIOA->PIO_PDSR;

        if (Timer_Check(timeout)) {
            WARNING("IO_WaitVBL timeout");
            break;
        }
    }
}

#elif defined(ARDUINO_SAMD_MKRVIDOR4000)

#define IO_DriveLow_OD(pin)     do { IO_DriveLow_OD_(pin, #pin); } while(0)
#define IO_DriveHigh_OD(pin)    do { IO_DriveHigh_OD_(pin, #pin); } while(0)
#define IO_Input_H(pin)         IO_Input_H_(pin, #pin)
#define IO_Input_L(pin)         IO_Input_L_(pin, #pin)

#define IO_ClearOutputData(x) 	do { IO_ClearOutputData_(x, #x); } while(0)
#define IO_SetOutputData(x)		do { IO_SetOutputData_(x, #x);   } while(0)

void IO_DriveLow_OD_(uint32_t pin, const char* pin_name);
void IO_DriveHigh_OD_(uint32_t pin, const char* pin_name);
uint8_t IO_Input_H_(uint32_t pin, const char* pin_name);
uint8_t IO_Input_L_(uint32_t pin, const char* pin_name);

void IO_ClearOutputData_(uint32_t pins, const char* pin_names);
void IO_SetOutputData_(uint32_t pins, const char* pin_names);

void IO_WaitVBL(void);

#else

#include <stdint.h>

void IO_DriveLow_OD(const char* pin);
void IO_DriveHigh_OD(const char* pin);
uint8_t IO_Input_H(const char* pin);
uint8_t IO_Input_L(const char* pin);

#define IO_ClearOutputData(x) 	do { IO_ClearOutputDataX(#x); } while(0)
#define IO_SetOutputData(x)		do { IO_SetOutputDataX(#x);   } while(0)

void IO_ClearOutputDataX(const char* pin);
void IO_SetOutputDataX(const char* pin);

void IO_WaitVBL(void);

#endif

void IO_Init(void);

#endif
