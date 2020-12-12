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

#if defined(AT91SAM7S256)

#include "board.h"
#include "usb_hardware.h"
#include "messaging.h"

static usb_func recv_func = 0;

static uint8_t CheckConnectedEP0()
{
    return 1;
}

void usb_connect(usb_func _recv)
{
    volatile int i;

    const uint32_t USB_DP_PUP = AT91C_PIO_PA16;

    // PLL USB Divider
    AT91C_BASE_CKGR->CKGR_PLLR |= AT91C_CKGR_USBDIV_1 ;

    // USB System and Peripheral Clock Enable
    AT91C_BASE_PMC->PMC_SCER = AT91C_PMC_UDP;
    AT91C_BASE_PMC->PMC_PCER = (1 << AT91C_ID_UDP);

    AT91C_BASE_PIOA->PIO_PER = USB_DP_PUP;
    // Keep disconnected for a while
    for (i = 0; i < 1000000; i++) {
        AT91C_BASE_PIOA->PIO_SODR = USB_DP_PUP;
        AT91C_BASE_PIOA->PIO_OER = USB_DP_PUP;
    }
    // Let it connect
    AT91C_BASE_PIOA->PIO_CODR = USB_DP_PUP;
    AT91C_BASE_PIOA->PIO_OER = USB_DP_PUP;

    if (AT91C_BASE_UDP->UDP_ISR & AT91C_UDP_ENDBUSRES) {
        AT91C_BASE_UDP->UDP_ICR = AT91C_UDP_ENDBUSRES;
    }

    recv_func = _recv;

    usb_poll();
}

static uint8_t buffer[64] __attribute__((aligned(8)));     // 64 bytes == max endpoint size with USB 2.0 full-speed
uint8_t usb_poll()
{
    uint8_t ret = FALSE;
    AT91PS_UDP udp = AT91C_BASE_UDP;
    const uint32_t num_eps = sizeof(AT91C_BASE_UDP->UDP_CSR) / sizeof(AT91C_BASE_UDP->UDP_CSR[0]);

    const uint8_t ctrl_ep = 0;
    const  uint8_t pingpong_eps[8] = { 0, 1, 1, 0, 0,0,0,0 }; // which end-points support ping-pong
    static uint8_t recvbank_eps[8] = { AT91C_UDP_RXSETUP, AT91C_UDP_RX_DATA_BK0, AT91C_UDP_RX_DATA_BK0, AT91C_UDP_RX_DATA_BK0, 0,0,0,0 };
    static uint16_t current_config = 0;
    (void)current_config;

/*
    DEBUG(1,"UDP_NUM      = %08x", udp->UDP_NUM);
    DEBUG(1,"UDP_GLBSTATE = %08x", udp->UDP_GLBSTATE);
    DEBUG(1,"UDP_FADDR    = %08x", udp->UDP_NUM);
    DEBUG(1,"UDP_IER      = %08x", udp->UDP_IER);
    DEBUG(1,"UDP_IDR      = %08x", udp->UDP_IDR);
    DEBUG(1,"UDP_IMR      = %08x", udp->UDP_IMR);
    DEBUG(1,"UDP_ISR      = %08x", udp->UDP_ISR);
    DEBUG(1,"UDP_ICR      = %08x", udp->UDP_ICR);
    DEBUG(1,"UDP_RSTEP    = %08x", udp->UDP_RSTEP);
    DEBUG(1,"UDP_TXVC     = %08x", udp->UDP_TXVC);
*/
    if (udp->UDP_ISR & AT91C_UDP_ENDBUSRES) {
        udp->UDP_ICR = AT91C_UDP_ENDBUSRES;

        DEBUG(1,"USB BUS RESET!");

        // following a reset we should be ready to receive a setup packet
        udp->UDP_RSTEP = 0xf;
        udp->UDP_RSTEP = 0;

        udp->UDP_FADDR = AT91C_UDP_FEN;

        udp->UDP_CSR[ctrl_ep] = AT91C_UDP_EPTYPE_CTRL | AT91C_UDP_EPEDS;

        for (int ep = 0; ep < num_eps; ++ep) {
            if (pingpong_eps[ep]) {
                recvbank_eps[ep] = AT91C_UDP_RX_DATA_BK0;
            }
        }

        current_config = 0;

        ret = 1;
    }

    // Loop through all endpoints
    for (int ep = 0; ep < num_eps; ++ep) {

        uint32_t epint = ((uint32_t) 0x1 << ep);                // AT91C_UDP_EPINTx

        if (udp->UDP_ISR & epint) {
            udp->UDP_ICR = epint;

            uint32_t bytecnt = ((udp->UDP_CSR[ep]) >> 16) & 0x7FF;  // AT91C_UDP_RXBYTECNT

            if (bytecnt > sizeof(buffer)) {
                // ERROR!
                DEBUG(1,"FIFO TOO BIG ERROR");
                return 0;
            }
            
            if ((udp->UDP_CSR[ep] & recvbank_eps[ep]) == 0) {
                continue;
            }

//            DEBUG(1,"EP[%d] interrupt requested", ep);
//            DEBUG(1,"EP[%d] rxbytecnt = %d", ep, bytecnt);

//            DEBUG(1,"EP[%d] control status matched %08x vs %08x", ep, udp->UDP_CSR[ep], recvbank_eps[ep]);

            // copy out the bytes
            for(int32_t i = 0; i < bytecnt; i++) {
                buffer[i] = udp->UDP_FDR[ep];
            }

//            DEBUG(1,"EP[%d] %d bytes copied", ep, bytecnt);

            if(ep == ctrl_ep) {
                UsbSetupPacket* setup = (UsbSetupPacket*) buffer;

                if (setup->bmRequestType & 0x80) {
//                    DEBUG(1,"EP[%d] bmRequestType = %02x", ep, setup->bmRequestType);
                    udp->UDP_CSR[ep] |= AT91C_UDP_DIR;
                    while ( !(udp->UDP_CSR[ep] & AT91C_UDP_DIR) );
                }
            }

//            DEBUG(1,"EP[%d] clear flags %08x .. ", ep, recvbank_eps[ep]);
            udp->UDP_CSR[ep] &= ~recvbank_eps[ep];
            while ( (udp->UDP_CSR[ep] & recvbank_eps[ep]) );

            if (pingpong_eps[ep]) {
                recvbank_eps[ep] = (recvbank_eps[ep] == AT91C_UDP_RX_DATA_BK0) ? AT91C_UDP_RX_DATA_BK1 : AT91C_UDP_RX_DATA_BK0;
//                DEBUG(1,"EP[%d] set match mask to %08x", ep, recvbank_eps[ep]);
            }

//            DEBUG(1,"EP[%d] calling 'recv_func' = %08x with %d bytes", ep, recv_func, bytecnt);
            recv_func(ep, buffer, bytecnt);
        }
    }

    return ret;
}

void usb_disconnect()
{
    const uint32_t USB_DP_PUP = AT91C_PIO_PA16;

    AT91C_BASE_PIOA->PIO_PER = USB_DP_PUP;
    AT91C_BASE_PIOA->PIO_SODR = USB_DP_PUP;
    AT91C_BASE_PIOA->PIO_OER = USB_DP_PUP;
}

#define min(a, b) (((a) > (b)) ? (b) : (a))

void usb_send(uint8_t ep, uint32_t wMaxPacketSize, const uint8_t* packet, uint32_t packet_length, uint32_t wLength)
{
    AT91PS_UDP udp = AT91C_BASE_UDP;

    if (packet_length != 512 || 3<=debuglevel)
    DEBUG(2,"usb_send (ep = %d, wMaxPacketSize = %d, packet = %08x, packet_length = %d, wLength = %d)", ep, wMaxPacketSize, packet, packet_length, wLength);

    int i, thisTime;
    int len = wLength ? min(packet_length, wLength) : packet_length;
    uint8_t sendzlp = (((len & (wMaxPacketSize-1)) == 0) && len != wLength) || packet == 0;
    while(len > 0) {
        thisTime = min(len, wMaxPacketSize);

        for(i = 0; i < thisTime; i++) {
            udp->UDP_FDR[ep] = packet[i];
        }

        if(udp->UDP_CSR[ep] & AT91C_UDP_TXCOMP) {
            udp->UDP_CSR[ep] &= ~AT91C_UDP_TXCOMP;
            while(udp->UDP_CSR[ep] & AT91C_UDP_TXCOMP)
                ;
        }

        udp->UDP_CSR[ep] |= AT91C_UDP_TXPKTRDY;

        while(!(udp->UDP_CSR[ep] & AT91C_UDP_TXCOMP)) {
            if(udp->UDP_CSR[ep] & AT91C_UDP_RX_DATA_BK0) {
                // This means that the host is trying to write to us, so
                // abandon our write to them.
                udp->UDP_CSR[ep] &= ~AT91C_UDP_RX_DATA_BK0;
                return;
            }
            if (!CheckConnectedEP0())
                return;
        }
        udp->UDP_CSR[ep] &= ~AT91C_UDP_TXCOMP;

        while(udp->UDP_CSR[ep] & AT91C_UDP_TXCOMP)
            if (!CheckConnectedEP0())
                return;

        len -= thisTime;
        packet += thisTime;
    }

    if (sendzlp) {
        udp->UDP_CSR[ep] |= AT91C_UDP_TXPKTRDY;
        while(!(udp->UDP_CSR[ep] & AT91C_UDP_TXCOMP))
            ;
        udp->UDP_CSR[ep] &= ~AT91C_UDP_TXCOMP;
        while(udp->UDP_CSR[ep] & AT91C_UDP_TXCOMP)
            ;
    }
}

void usb_send_async(uint8_t ep, usb_func _send, uint32_t length)
{

}

void usb_send_ep0_stall(void)
{
    AT91PS_UDP udp = AT91C_BASE_UDP;
    const uint8_t ep = 0;

    udp->UDP_CSR[ep] |= AT91C_UDP_FORCESTALL;
    while(!(udp->UDP_CSR[ep] & AT91C_UDP_STALLSENT))
        ;
    udp->UDP_CSR[ep] &= ~(AT91C_UDP_FORCESTALL | AT91C_UDP_STALLSENT);
    while(udp->UDP_CSR[ep] & (AT91C_UDP_FORCESTALL | AT91C_UDP_STALLSENT))
        ;
}

void usb_send_stall(uint8_t ep)
{
    AT91PS_UDP udp = AT91C_BASE_UDP;

    udp->UDP_CSR[ep] |= AT91C_UDP_FORCESTALL;
    while(!(udp->UDP_CSR[ep] & AT91C_UDP_STALLSENT))
        ;
    udp->UDP_CSR[ep] &= ~(AT91C_UDP_FORCESTALL | AT91C_UDP_STALLSENT);
    while(udp->UDP_CSR[ep] & (AT91C_UDP_FORCESTALL | AT91C_UDP_STALLSENT))
        ;
}

static usb_func read_old_recv = NULL;
static uint8_t read_ep = 0xff;
static uint8_t* read_offset = NULL;
static uint32_t read_remaining = 0;
static void usb_recv_cb(uint8_t ep, uint8_t* packet, uint32_t length)
{
    DEBUG(3,"usb_recv_cb (ep = %d, packet = %08x, packet_length = %d) while %d bytes remaining", ep, packet, length, read_remaining);
    if (ep != read_ep) {                      // we're reading but we found something else
        if (read_old_recv) {
            read_old_recv(ep, packet, length);
        }
        return;
    }
    uint32_t valid_bytes = min(length, read_remaining);
    memcpy(read_offset, packet, length);
    read_offset += valid_bytes;
    read_remaining -= valid_bytes;
    if (length == 0) {
        WARNING("USB:Short read");
        read_offset = NULL;
    }
}
uint32_t usb_recv(uint8_t ep, uint8_t* packet, uint32_t length)
{
    DEBUG(2,"usb_recv (ep = %d, packet = %08x, packet_length = %d)", ep, packet, length);
    if (read_ep != 0xff) {
        ERROR("USB:Cannot issue read operations on two differen EPs!");
        return 0;
    }
    read_ep = ep;
    read_old_recv = recv_func;
    recv_func = usb_recv_cb;
    read_offset = packet;
    read_remaining = length;
    while(read_offset && read_remaining) {
        usb_poll();
    }
    read_ep = 0xff;
    recv_func = read_old_recv;
    read_old_recv = NULL;
    return length - read_remaining;
}


void usb_setup_endpoints(uint32_t* ep_types, uint32_t num_eps)
{
    int i;
    AT91_REG global = 0, ep[3] = {0,0,0};
    AT91PS_UDP udp = AT91C_BASE_UDP;

    for (i = 0; i < min(num_eps, 3); ++i) {
        global |= ep_types[i];
        ep[i]   = ep_types[i];
    }

    if (global) {
//        DEBUG(1,"setup = %08x, %08x, %08x, %08x", AT91C_UDP_CONFG, ep[0], ep[1], ep[2]);
        udp->UDP_GLBSTATE   = AT91C_UDP_CONFG;
        udp->UDP_CSR[1]     = ep[0];
        udp->UDP_CSR[2]     = ep[1];
        udp->UDP_CSR[3]     = ep[2];
    } else {
//       DEBUG(1,"setup = %08x, %08x, %08x, %08x", AT91C_UDP_FADDEN, ep[0], ep[1], ep[2]);
        udp->UDP_GLBSTATE   = AT91C_UDP_FADDEN;
        udp->UDP_CSR[1]     = 0;
        udp->UDP_CSR[2]     = 0;
        udp->UDP_CSR[3]     = 0;
    }

}

void usb_setup_faddr(uint16_t wValue)
{
    AT91PS_UDP udp = AT91C_BASE_UDP;
    udp->UDP_FADDR = (AT91C_UDP_FEN | wValue);
    udp->UDP_GLBSTATE  = (wValue) ? AT91C_UDP_FADDEN : 0;
}

#endif // defined(AT91SAM7S256)
