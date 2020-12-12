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
#include "hardware/usart.h"
#include "hardware/irq.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>

// Run socat from a separate terminal:
//
// $ socat pty,raw,echo=0,link=/tmp/replay-usart -,raw,echo=0
//
// or telnet
//
// $ telnet localhost 1234

// #define USART_TELNET_PORT 1234
int TELNET_init(uint16_t port);
bool TELNET_connected();
size_t TELNET_recv(void* buffer, size_t size);
void TELNET_send(void* buffer, size_t size);

// for RX, we use a software ring buffer as we are interested in any character
// as soon as it is received.
#define RXBUFLEN 16
#define RXBUFMASK (RXBUFLEN-1)
volatile uint8_t USART_rxbuf[RXBUFLEN];
volatile int16_t USART_rxptr, USART_rdptr;

// for TX, we use a hardware buffer triggered to be sent when full or on a CR.
#define TXBUFLEN 128
#define TXBUFMASK (TXBUFLEN-1)
volatile uint8_t USART_txbuf[TXBUFLEN];
volatile int16_t USART_txptr, USART_wrptr, USART_barrier;

int fd = -1; // fake serial file descriptor

void USART_Init(unsigned long baudrate)
{
#ifndef USART_TELNET_PORT

    fd = open("/tmp/replay-usart", O_RDWR);

    if (fd == -1) {
        return;
    }

    int saved_flags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, saved_flags | O_NONBLOCK);

#else

    if (TELNET_init(USART_TELNET_PORT)) {
        return;
    }

#endif

    USART_txptr = USART_wrptr = 0;
}

void USART_update(void)
{
    char data[RXBUFLEN];
    ssize_t length = -1;

#ifndef USART_TELNET_PORT

    if (fd == -1) {
        return;
    }

    length = read(fd, data, RXBUFLEN);

#else

    length = TELNET_recv(data, RXBUFLEN);

#endif

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

void USART_Putc(void* p, char c)
{
#ifndef USART_TELNET_PORT

    if (fd == -1) {
        return;
    }

#endif

    USART_txbuf[USART_wrptr] = c;
    USART_wrptr = (USART_wrptr + 1) & TXBUFMASK;

    if ((c == '\n') || (!USART_wrptr) || (USART_wrptr == USART_barrier) || 1) {
        volatile uint8_t* ptr = & (USART_txbuf[USART_txptr]);
        size_t len = (TXBUFLEN + USART_wrptr - USART_txptr) & TXBUFMASK;

#ifndef USART_TELNET_PORT
        write(fd, (const char*) ptr, len);
#else
        TELNET_send( (char*) ptr, len);
#endif

        USART_barrier = USART_txptr;
        USART_txptr = USART_wrptr;
    }
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

inline int16_t USART_CharAvail(void)
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
