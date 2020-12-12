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

#ifndef HARDWARE_USART_H_INCLUDED
#define HARDWARE_USART_H_INCLUDED

#define USB_USART 0

#if USB_USART==1
#include"cdc_enumerate.h"
extern struct _AT91S_CDC pCDC;
#endif

// Must call USART_Init before Put/Get
void USART_Init(unsigned long baudrate);
// Putc printf callback
void USART_Putc(void*, char c);

uint8_t USART_GetValid(void);
uint8_t USART_Getc(void);
uint8_t USART_Peekc(void);
int16_t USART_CharAvail(void);
uint16_t USART_PeekBuf(uint8_t* buf, int16_t len);
uint16_t USART_GetBuf(const uint8_t* buf, int16_t len);
void USART_update(void);

#endif
