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

#include "../board.h"

// for RX, we use a software ring buffer as we are interested in any character
// as soon as it is received.
#define RXBUFLEN 16
#define RXBUFMASK (RXBUFLEN-1)
volatile uint8_t USART_rxbuf[RXBUFLEN];
volatile int16_t USART_rxptr, USART_rdptr;

static int read(void* buf, size_t count)
{
    uint8_t* p = (uint8_t*)buf;

    while (count--) {
        if (!Serial1.available()) {
            break;
        }

        *p++ = Serial1.read();
    }

    return p - (uint8_t*)buf;
}

extern "C" {

#include "../hardware/usart.h"

void USART_Init(unsigned long baudrate)
{
    Serial1.end();
    Serial1.begin(baudrate);

#if 0 // debuglevel > 0
    SerialUSB.begin(9600);
    HARDWARE_TICK delay = Timer_Get(0);
    for (int i = 0; i < 15; ++i) {
        while (!SerialUSB && !Timer_Check(delay))
            ;
        if (SerialUSB)
            break;
        Serial1.println("Waiting for SerialUSB...");
        delay = Timer_Get(1000);
    }
#endif
}

void USART_update(void)
{
    char data[RXBUFLEN];
    ssize_t length = -1;

    length = read(data, RXBUFLEN);

    if (length == -1) {
        return;
    }

    if (length) {
        if ((length + USART_rxptr) > (RXBUFLEN - 1)) {
            memcpy((void*) & (USART_rxbuf[USART_rxptr]), data, RXBUFLEN - USART_rxptr);
            memcpy((void*)USART_rxbuf, (void*) & (data[(RXBUFLEN - USART_rxptr)]), length - (RXBUFLEN - USART_rxptr));

        } else {
            memcpy((void*) & (USART_rxbuf[USART_rxptr]), data, length);
        }

        USART_rxptr = (USART_rxptr + length) & RXBUFMASK;
    }
}

void USART_Putc(void*, char c)
{
    Serial1.write(c);
#if 0 // debuglevel > 0
    SerialUSB.write(c);
#endif
}


uint8_t USART_GetValid(void)
{
    uint8_t val = 0;

    if (USART_rxptr != USART_rdptr) {
        val = 1;
    }

    return val;
}

uint8_t USART_Getc(void)
{
    uint8_t val;

    if (USART_rxptr != USART_rdptr) {
        val = USART_rxbuf[USART_rdptr];
        USART_rdptr = (USART_rdptr + 1) & RXBUFMASK;

    } else {
        val = 0;
    }

    return val;
}

uint8_t USART_Peekc(void)
{
    uint8_t val;

    if (USART_rxptr != USART_rdptr) {
        val = USART_rxbuf[USART_rdptr];

    } else {
        val = 0;
    }

    return val;
}

int16_t USART_CharAvail(void)
{
    return (RXBUFLEN + USART_rxptr - USART_rdptr) & RXBUFMASK;
}

/*
 * Returns the current rxbuf in buf, up to len bytes
 * @returns # of bytes copied
 */
uint16_t USART_PeekBuf(uint8_t* buf, int16_t maxlen)
{
    uint16_t i, avail = USART_CharAvail();

    if (avail == 0) {
        return 0;
    }

    for (i = 0; i < avail && i < maxlen; ++i) {
        buf[i] = USART_rxbuf[(USART_rdptr + i) & RXBUFMASK];
    }

    return i;
}

/**
 * Compare given buf with contents of USART rxbuf. If the buffer
 * matches in its entirety, it is also plucked from the rxbuf.
 * @returns 1 if match or 0 if no match
 */
uint16_t USART_GetBuf(const uint8_t* buf, int16_t len)
{
    uint16_t i;

    if (USART_CharAvail() < len) {
        return 0;    // not enough chars to compare
    }

    for (i = 0; i < len; ++i) {
        if (USART_rxbuf[(USART_rdptr + i) & RXBUFMASK] != buf[i]) {
            break;
        }
    }

    if (i != len) {
        return 0;    // no match
    }

    // got it, remove chars from buffer
    USART_rdptr = (USART_rdptr + len) & RXBUFMASK;
    return 1;
}

} // extern "C"
