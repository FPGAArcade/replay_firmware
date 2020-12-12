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

#include "usb_debug.h"

void print_dev_desc(const usb_dev_desc_t* desc)
{
	(void)desc;
	usb_printf("    usb_dev_desc_t @ %08x :\n\r", desc);
    usb_printf("        bLength             = 0x%02x\n\r",desc->bLength);
    usb_printf("        bDescriptorType     = 0x%02x\n\r",desc->bDescriptorType);
    usb_printf("        bcdUSB              = 0x%04x\n\r",desc->bcdUSB);
    usb_printf("        bDeviceClass        = 0x%02x\n\r",desc->bDeviceClass);
    usb_printf("        bDeviceSubClass     = 0x%02x\n\r",desc->bDeviceSubClass);
    usb_printf("        bDeviceProtocol     = 0x%02x\n\r",desc->bDeviceProtocol);
    usb_printf("        bMaxPacketSize0     = 0x%02x\n\r",desc->bMaxPacketSize0);
    usb_printf("        idVendor            = 0x%04x\n\r",desc->idVendor);
    usb_printf("        idProduct           = 0x%04x\n\r",desc->idProduct);
    usb_printf("        bcdDevice           = 0x%04x\n\r",desc->bcdDevice);
    usb_printf("        iManufacturer       = 0x%02x\n\r",desc->iManufacturer);
    usb_printf("        iProduct            = 0x%02x\n\r",desc->iProduct);
    usb_printf("        iSerialNumber       = 0x%02x\n\r",desc->iSerialNumber);
    usb_printf("        bNumConfigurations  = 0x%02x\n\r",desc->bNumConfigurations);
}

void print_iface_desc(const usb_iface_desc_t* iface)
{
	(void)iface;
	usb_printf("    usb_iface_desc_t @ %08x :\n\r", iface);
	usb_printf("        bLength             = 0x%02x\n\r",iface->bLength);
	usb_printf("        bDescriptorType     = 0x%02x\n\r",iface->bDescriptorType);
	usb_printf("        bInterfaceNumber    = 0x%02x\n\r",iface->bInterfaceNumber);
	usb_printf("        bAlternateSetting   = 0x%02x\n\r",iface->bAlternateSetting);
	usb_printf("        bNumEndpoints       = 0x%02x\n\r",iface->bNumEndpoints);
	usb_printf("        bInterfaceClass     = 0x%02x\n\r",iface->bInterfaceClass);
	usb_printf("        bInterfaceSubClass  = 0x%02x\n\r",iface->bInterfaceSubClass);
	usb_printf("        bInterfaceProtocol  = 0x%02x\n\r",iface->bInterfaceProtocol);
	usb_printf("        iInterface          = 0x%02x\n\r",iface->iInterface);
}

void print_ep_desc(const usb_ep_desc_t* ep)
{
	(void)ep;
	usb_printf("    usb_ep_desc_t @ %08x :\n\r", ep);
	usb_printf("        bLength             = 0x%02x\n\r",ep->bLength);
	usb_printf("        bDescriptorType     = 0x%02x\n\r",ep->bDescriptorType);
	usb_printf("        bEndpointAddress    = 0x%02x\n\r",ep->bEndpointAddress);
	usb_printf("        bmAttributes        = 0x%02x\n\r",ep->bmAttributes);
	usb_printf("        wMaxPacketSize      = 0x%02x\n\r",ep->wMaxPacketSize);
	usb_printf("        bInterval           = 0x%02x\n\r",ep->bInterval);
}

void print_hid_desc(const usb_hid_descriptor_t* hid)
{
	(void)hid;
	usb_printf("    usb_hid_descriptor_t @ %08x :\n\r", hid);
	usb_printf("        bLength             = 0x%02x\n\r",hid->bLength);
	usb_printf("        bDescriptorType     = 0x%02x\n\r",hid->bDescriptorType);
	usb_printf("        bcdHID              = 0x%04x\n\r",hid->bcdHID);
	usb_printf("        bCountryCode        = 0x%02x\n\r",hid->bCountryCode);
	usb_printf("        bNumDescriptors     = 0x%02x\n\r",hid->bNumDescriptors);
	usb_printf("        bRDescriptorType    = 0x%02x\n\r",hid->bRDescriptorType);
	usb_printf("        wDescriptorLength   = 0x%04x\n\r",hid->wDescriptorLength);
}

void print_conf_desc(const usb_conf_desc_t* conf, const bool full)
{
	(void)conf;
	usb_printf("    usb_conf_desc_t @ %08x :\n\r", conf);
	usb_printf("        bLength             = 0x%02x\n\r",conf->bLength);
	usb_printf("        bDescriptorType     = 0x%02x\n\r",conf->bDescriptorType);
	usb_printf("        wTotalLength        = 0x%04x\n\r",conf->wTotalLength);
	usb_printf("        bNumInterfaces      = 0x%02x\n\r",conf->bNumInterfaces);
	usb_printf("        bConfigurationValue = 0x%02x\n\r",conf->bConfigurationValue);
	usb_printf("        iConfiguration      = 0x%02x\n\r",conf->iConfiguration);
	usb_printf("        bmAttributes        = 0x%02x\n\r",conf->bmAttributes);
	usb_printf("        bMaxPower           = 0x%02x\n\r",conf->bMaxPower);
	if (conf->bLength != conf->wTotalLength && !full) {
		usb_printf("        (ignoring additional descriptors)\n\r");
		return;
	}

	typedef struct {
		uint8_t bLength;
		uint8_t bDescriptorType;
	} usb_desc_header_t;

	usb_desc_header_t* desc = (usb_desc_header_t*)conf;
	for (uint16_t remaining = le16_to_cpu(conf->wTotalLength); remaining > 0; ) {

		switch(desc->bDescriptorType) {

			case USB_DT_DEVICE:
				print_dev_desc((usb_dev_desc_t*)desc);
				break;
			case USB_DT_CONFIGURATION:
				break;
			case USB_DT_STRING:
				usb_printf("    usb_string_desc_t @ %08x :\n\r", desc);
				break;
			case USB_DT_INTERFACE:
				print_iface_desc((usb_iface_desc_t*)desc);
				break;
			case USB_DT_ENDPOINT:
				print_ep_desc((usb_ep_desc_t*)desc);
				break;
			case USB_DT_HID:
				print_hid_desc((usb_hid_descriptor_t*)desc);
				break;

			default:
			// Ignore descriptor
			usb_printf("  -> Unknown desc %02x (size %d)\n\r", desc->bDescriptorType, desc->bLength);
			break;

		}

		Assert(remaining >= desc->bLength);
		remaining -= desc->bLength;
		desc = (usb_desc_header_t*)((uint8_t*)desc + desc->bLength);
	}
}

void print_usb_desc(const usb_hub_descriptor_t* hub)
{
	(void)hub;
	usb_printf("    usb_hub_descriptor_t @ %08x :\n\r", hub);

	usb_printf("        bDescLength         = 0x%02x\n\r",hub->bDescLength);
	usb_printf("        bDescriptorType     = 0x%02x\n\r",hub->bDescriptorType);
	usb_printf("        bNbrPorts           = 0x%02x\n\r",hub->bNbrPorts);
	usb_printf("        wHubCharacteristics = 0x%02x\n\r",hub->wHubCharacteristics);
	usb_printf("        bPwrOn2PwrGood      = 0x%02x\n\r",hub->bPwrOn2PwrGood);
	usb_printf("        bHubContrCurrent    = 0x%02x\n\r",hub->bHubContrCurrent);
	usb_printf("        variables[0]        = 0x%08x\n\r",hub->variables[0]);
}

void print_uhc_dev(const uhc_device_t* dev)
{
	(void)dev;
	usb_printf("uhc_device_t @ %08x :\n\r", dev);
	print_dev_desc(&dev->dev_desc);
	usb_printf("    address  = 0x%02x\n\r",dev->address);
	switch(dev->speed) {
		case UHD_SPEED_LOW:
	    usb_printf("    speed    = LOW (0x%02x)\n\r",dev->speed);
		break;
		case UHD_SPEED_FULL:
	    usb_printf("    speed    = FULL (0x%02x)\n\r",dev->speed);
		break;
		case UHD_SPEED_HIGH:
	    usb_printf("    speed    = HIGH (0x%02x)\n\r",dev->speed);
		break;
		default:
	    usb_printf("    speed    = UNKNOWN (0x%02x)\n\r",dev->speed);
		break;
	}
#ifdef USB_HOST_LPM_SUPPORT
    usb_printf("    lpm_desc = 0x%08x\n\r",dev->lpm_desc);
#endif
#ifdef USB_HOST_HUB_SUPPORT
    usb_printf("    prev     = 0x%08x\n\r",dev->prev);
    usb_printf("    next     = 0x%08x\n\r",dev->next);
    usb_printf("    hub      = 0x%08x\n\r",dev->hub);
    usb_printf("    power    = 0x%02x\n\r",dev->power);
    usb_printf("    hub_port = 0x%04x\n\r",dev->hub_port);
#endif
}
