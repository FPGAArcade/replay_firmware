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

void ISR_USART(void)
{
    uint32_t isr_status = AT91C_BASE_US0->US_CSR;

    // returns if no character
    if (!(isr_status & AT91C_US_RXRDY)) {
        return;
    }

    USART_rxbuf[USART_rxptr] = AT91C_BASE_US0->US_RHR;
    USART_rxptr = (USART_rxptr + 1) & RXBUFMASK;
}

void USART_Init(unsigned long baudrate)
{
#if USB_USART==1
    USB_Open();
#else
    void (*handler)(void) = &ISR_USART;

    // disable IRQ on ARM
    disableIRQ();

    // Configure PA5 and PA6 for USART0 use
    AT91C_BASE_PIOA->PIO_PDR = AT91C_PA5_RXD0 | AT91C_PA6_TXD0;
    // disable pullup on output
    AT91C_BASE_PIOA->PIO_PPUDR = PIN_TXD;

    // Enable the peripheral clock in the PMC
    AT91C_BASE_PMC->PMC_PCER = 1 << AT91C_ID_US0;

    // Reset and disable receiver & transmitter
    AT91C_BASE_US0->US_CR = AT91C_US_RSTRX | AT91C_US_RSTTX | AT91C_US_RXDIS | AT91C_US_TXDIS;

    // Configure USART0 mode
    AT91C_BASE_US0->US_MR = AT91C_US_USMODE_NORMAL | AT91C_US_CLKS_CLOCK | AT91C_US_CHRL_8_BITS | AT91C_US_PAR_NONE | AT91C_US_NBSTOP_1_BIT | AT91C_US_CHMODE_NORMAL;

    // Configure the USART0 @115200 bauds
    AT91C_BASE_US0->US_BRGR = BOARD_MCK / 16 / baudrate;

    // Disable AIC interrupt first
    AT91C_BASE_AIC->AIC_IDCR = 1 << AT91C_ID_US0;

    // Configure AIC interrupt mode and handler
    AT91C_BASE_AIC->AIC_SMR[AT91C_ID_US0] = 0;
    AT91C_BASE_AIC->AIC_SVR[AT91C_ID_US0] = (int32_t)handler;

    // Clear AIC interrupt
    AT91C_BASE_AIC->AIC_ICCR = 1 << AT91C_ID_US0;

    // Enable AIC interrupt
    AT91C_BASE_AIC->AIC_IECR = 1 << AT91C_ID_US0;

    // Enable receiver & transmitter
    AT91C_BASE_US0->US_CR = AT91C_US_RXEN | AT91C_US_TXEN;

    // Enable USART RX interrupt
    AT91C_BASE_US0->US_IER = AT91C_US_RXRDY;

    // enable IRQ on ARM
    enableIRQ();
#endif

    USART_txptr = USART_wrptr = 0;
}

void USART_update(void)
{
#if USB_USART==1

    if (pCDC.IsConfigured(&pCDC)) {
        char data[RXBUFLEN];
        uint16_t length;
        length = pCDC.Read(&pCDC, data, RXBUFLEN);

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

#endif
}

void USART_Putc(void* p, char c)
{
    // if both PDC channels are blocked, we still have to wait --> bad luck...
    while (AT91C_BASE_US0->US_TNCR) ;

    // ok, thats the simplest solution - we could still continue in some cases,
    // but I am not sure if it is worth the effort...

    USART_txbuf[USART_wrptr] = c;
    USART_wrptr = (USART_wrptr + 1) & TXBUFMASK;

    if ((c == '\n') || (!USART_wrptr) || (USART_wrptr == USART_barrier)) {
#if USB_USART==1

        if (pCDC.IsConfigured(&pCDC)) {
            //pCDC.Write(&pCDC, data, length);
            pCDC.Write(&pCDC, (const char*) & (USART_txbuf[USART_txptr]), (TXBUFLEN + USART_wrptr - USART_txptr) & TXBUFMASK);
            USART_barrier = USART_txptr;
            USART_txptr = USART_wrptr;
        }

#else

        // flush the buffer now (end of line, end of buffer reached or buffer full)
        if ((AT91C_BASE_US0->US_TCR == 0) && (AT91C_BASE_US0->US_TNCR == 0)) {
            USART_barrier = USART_txptr;
            AT91C_BASE_US0->US_TPR = (uint32_t) & (USART_txbuf[USART_txptr]);
            AT91C_BASE_US0->US_TCR = (TXBUFLEN + USART_wrptr - USART_txptr) & TXBUFMASK;
            AT91C_BASE_US0->US_PTCR = AT91C_PDC_TXTEN;
            USART_txptr = USART_wrptr;

        } else if (AT91C_BASE_US0->US_TNCR == 0) {
            AT91C_BASE_US0->US_TNPR = (uint32_t) & (USART_txbuf[USART_txptr]);
            AT91C_BASE_US0->US_TNCR = (TXBUFLEN + USART_wrptr - USART_txptr) & TXBUFMASK;
            USART_txptr = USART_wrptr;
        }

#endif
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
