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

#include <stdint.h>
#include "board.h"

typedef struct _UsbSetupPacket {
    uint8_t        bmRequestType;
    uint8_t        bRequest;
    uint16_t        wValue;
    uint16_t        wIndex;
    uint16_t        wLength;
} __attribute__((packed)) UsbSetupPacket;

enum EndpointTypes
{
	EPTYPE_CTRL			= (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_CTRL),
	EPTYPE_ISO_OUT		= (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_ISO_OUT),
	EPTYPE_BULK_OUT		= (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_OUT),
	EPTYPE_INT_OUT		= (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_INT_OUT),
	EPTYPE_ISO_IN		= (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_ISO_IN),
	EPTYPE_BULK_IN		= (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_BULK_IN),
	EPTYPE_INT_IN		= (AT91C_UDP_EPEDS | AT91C_UDP_EPTYPE_INT_IN),
};


#define USB_REQUEST_GET_STATUS          0
#define USB_REQUEST_CLEAR_FEATURE       1
#define USB_REQUEST_SET_FEATURE         3
#define USB_REQUEST_SET_ADDRESS         5
#define USB_REQUEST_GET_DESCRIPTOR      6
#define USB_REQUEST_SET_DESCRIPTOR      7
#define USB_REQUEST_GET_CONFIGURATION   8
#define USB_REQUEST_SET_CONFIGURATION   9
#define USB_REQUEST_GET_INTERFACE       10
#define USB_REQUEST_SET_INTERFACE       11
#define USB_REQUEST_SYNC_FRAME          12
#define USB_REQUEST_GET_MAX_LUN			0xfe

#define USB_DESCRIPTOR_TYPE_DEVICE              1
#define USB_DESCRIPTOR_TYPE_CONFIGURATION       2
#define USB_DESCRIPTOR_TYPE_STRING              3
#define USB_DESCRIPTOR_TYPE_INTERFACE           4
#define USB_DESCRIPTOR_TYPE_ENDPOINT            5
#define USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER    6
#define USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONF    7
#define USB_DESCRIPTOR_TYPE_INTERFACE_POWER     8
#define USB_DESCRIPTOR_TYPE_HID                 0x21
#define USB_DESCRIPTOR_TYPE_HID_REPORT          0x22

#define USB_DEVICE_CLASS_HID                    0x03

typedef void (*usb_func)(uint8_t ep, uint8_t* packet, uint32_t length);
void usb_connect(usb_func _recv);
uint8_t usb_poll();
void usb_disconnect();

void usb_send(uint8_t ep, uint32_t wMaxPacketSize, const uint8_t* packet, uint32_t packet_length, uint32_t wLength);
void usb_send_async(uint8_t ep, usb_func _send, uint32_t length);

void usb_send_ep0_stall(void);
void usb_send_stall(uint8_t ep);

uint32_t usb_recv(uint8_t ep, uint8_t* packet, uint32_t length);

void usb_setup_faddr(uint16_t wValue);
void usb_setup_endpoints(uint32_t* ep_types, uint32_t num_eps);

// helpers
//void usb_send_ep0(const uint8_t* packet, uint32_t packet_length, uint32_t wLength);
#if 1
static inline void usb_send_ep0(const uint8_t* packet, uint32_t packet_length, uint32_t wLength)
{
	usb_send(/*ep = */ 0, /*wMaxPacketSize = */ 8, packet, packet_length, wLength);
}
#endif
static inline void usb_send_ep0_zlp()
{
	usb_send(/*ep = */ 0, /*wMaxPacketSize = */ 8, /*packet = */ 0, /*packet_length = */ 0, /*wLength = */ 0);
}
