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

#pragma once

#include "Arduino.h"

#ifdef __cplusplus
extern "C"
#endif
int kprintf( const char * format, ... );

#ifdef __cplusplus
extern "C"
#endif
void kprintmem(const uint8_t* memory, uint32_t size);

#if 0
#define usb_printf(...) do { kprintf(__VA_ARGS__ ); } while(0)
#define usb_printmem(p,s) do { kprintmem(p,s); } while(0)
#else
#define usb_printf(...) do { } while(0)
#define usb_printmem(p,s) do { } while(0)
#endif

#undef Assert
#if 0
#define Assert(expr) do { if(!(expr)) { kprintf("*** expr '%s' failed at %s:%i\n\r", #expr, __FILE__, __LINE__); asm("BKPT #0"); } } while(0)
#else
#define Assert(expr) do {  } while(0)
#endif

#include "usb_protocol.h"
#include "usb_protocol_hid.h"
#include "usb_protocol_hub.h"
#include "uhc.h"

void print_dev_desc(const usb_dev_desc_t* desc);
void print_iface_desc(const usb_iface_desc_t* iface);
void print_ep_desc(const usb_ep_desc_t* ep);
void print_hid_desc(const usb_hid_descriptor_t* hid);
void print_conf_desc(const usb_conf_desc_t* conf, const bool full);
void print_usb_desc(const usb_hub_descriptor_t* hub);
void print_uhc_dev(const uhc_device_t* dev);
