/**
 * \file
 *
 * \brief USB host Human Interface Device (HID) mouse driver.
 *
 * Copyright (c) 2011-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#include "conf_usb_host.h"
#include "usb_protocol.h"
#include "uhd.h"
#include "uhc.h"
#include "uhi_hid_mouse.h"
#include "uhi_hid.h"
#include <string.h>

/**
 * \ingroup uhi_hid_mouse_group
 * \defgroup uhi_hid_mouse_group_internal Implementation of UHI HID Mouse
 *
 * Class internal implementation
 * @{
 */

/**
 * \name Index in HID report for usual HID mouse events
 * @{
 */
#define UHI_HID_MOUSE_BTN        0
#define UHI_HID_MOUSE_MOV_X      1
#define UHI_HID_MOUSE_MOV_Y      2
#define UHI_HID_MOUSE_MOV_SCROLL 3
//@}

/**
 * \name Structure to store information about USB Device HID mouse
 */
//@{
typedef struct {
	uhc_device_t *dev;
	usb_ep_t ep_in;
	uint8_t interface_number;
	uint8_t report_id;	
	uint8_t report_size;
	uint8_t report_btn_prev;
	uint16_t hid_report_size;
	COMPILER_WORD_ALIGNED uint8_t report[64];
}uhi_hid_mouse_dev_t;

static uhi_hid_mouse_dev_t uhi_hid_mouse_dev = {
	.dev = NULL,
	.report_id = 0xff
	};

//@}


/**
 * \name Internal routines
 */
//@{
static void uhi_hid_mouse_start_trans_report(usb_add_t add);
static void uhi_hid_mouse_report_reception(
		usb_add_t add,
		usb_ep_t ep,
		uhd_trans_status_t status,
		iram_size_t nb_transfered);
//@}


/**
 * \name Functions required by UHC
 * @{
 */

uhc_enum_status_t uhi_hid_mouse_install(uhc_device_t* dev, usb_conf_desc_t* conf_desc)
{
	usb_printf("%s\n\r", __FUNCTION__);
	uhi_hid_mouse_dev.hid_report_size = 0;

	bool b_iface_supported;
	uint16_t conf_desc_lgt;
	usb_iface_desc_t *ptr_iface;

	conf_desc_lgt = le16_to_cpu(conf_desc->wTotalLength);
	ptr_iface = (usb_iface_desc_t*)conf_desc;
	b_iface_supported = false;
	while(conf_desc_lgt) {
		switch (ptr_iface->bDescriptorType) {

		case USB_DT_INTERFACE:
			usb_printf(" .. USB_DT_INTERFACE\n\r");
			print_iface_desc(ptr_iface);
			if ((ptr_iface->bInterfaceClass   == HID_CLASS)
			&& (ptr_iface->bInterfaceProtocol == HID_PROTOCOL_MOUSE) ) {
				// USB HID Mouse interface found
				// Start allocation endpoint(s)
				usb_printf("  -> HID_CLASS and HID_PROTOCOL_MOUSE => supported\n\r");
				b_iface_supported = true;
				uhi_hid_mouse_dev.interface_number = ptr_iface->bInterfaceNumber;
			} else {
				// Stop allocation endpoint(s)
				usb_printf("  -> not HID_CLASS/HID_PROTOCOL_MOUSE; skipping\n\r");
				b_iface_supported = false;
			}
			break;

		case USB_DT_HID:
			//  Process HID report
			usb_printf(" .. USB_DT_HID\n\r");
			usb_hid_descriptor_t* ptr_hid = (usb_hid_descriptor_t*)ptr_iface;
			print_hid_desc(ptr_hid);
			if (!b_iface_supported) {
				usb_printf("  -> EP not supported or already config'd\n\r");
				break;
			}

			if (ptr_hid->bRDescriptorType != USB_DT_HID_REPORT)
				break;

			uhi_hid_mouse_dev.hid_report_size = le16_to_cpu(ptr_hid->wDescriptorLength);
			Assert(uhi_hid_mouse_dev.hid_report_size <= sizeof(uhi_hid_report_data.hid_report));

			usb_printf("  -> uhi_hid_mouse_dev.hid_report_size = %i bytes\n\r", uhi_hid_mouse_dev.hid_report_size);

			break;

		case USB_DT_ENDPOINT:
			//  Allocation of the endpoint
			usb_printf(" .. USB_DT_ENDPOINT\n\r");
			usb_ep_desc_t* ptr_ep = (usb_ep_desc_t*)ptr_iface;
			print_ep_desc(ptr_ep);
			if (!b_iface_supported) {
				usb_printf("  -> EP not supported or already config'd\n\r");
				break;
			}
			if (uhi_hid_mouse_dev.dev != NULL) {
				return UHC_ENUM_SOFTWARE_LIMIT; // Device already allocated
			}
			if (!uhd_ep_alloc(dev->address, (usb_ep_desc_t*)ptr_iface, dev->speed)) {
				return UHC_ENUM_HARDWARE_LIMIT; // Endpoint allocation fail
			}
			Assert(((usb_ep_desc_t*)ptr_iface)->bEndpointAddress & USB_EP_DIR_IN);
			uhi_hid_mouse_dev.ep_in = ((usb_ep_desc_t*)ptr_iface)->bEndpointAddress;
			uhi_hid_mouse_dev.report_size =
					le16_to_cpu(((usb_ep_desc_t*)ptr_iface)->wMaxPacketSize);
			Assert(uhi_hid_mouse_dev.report_size <= sizeof(uhi_hid_mouse_dev.report));
			uhi_hid_mouse_dev.dev = dev;
			// All endpoints of all interfaces supported allocated
			usb_printf("  -> HID_MOUSE success\n\r");
			return UHC_ENUM_SUCCESS;

		default:
			// Ignore descriptor
			usb_printf("  -> Ignoring %02x (size %d)\n\r", ptr_iface->bDescriptorType, ptr_iface->bLength);
			break;
		}
		Assert(conf_desc_lgt>=ptr_iface->bLength);
		conf_desc_lgt -= ptr_iface->bLength;
		ptr_iface = (usb_iface_desc_t*)((uint8_t*)ptr_iface + ptr_iface->bLength);
	}

	return UHC_ENUM_UNSUPPORTED; // No interface supported
}


static void _uhi_hid_mouse_get_hid_report(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans)
{
	(void)add;
	usb_printf("%s addr = 0x%02x ; status = 0x%02x ; payload_trans = %d bytes\n\r", __FUNCTION__, add, status, payload_trans);

	if (status != UHD_TRANS_NOERROR || payload_trans != uhi_hid_mouse_dev.hid_report_size) {
		usb_printf(" ERROR failure to retrieve HID report\n\r");
		return;
	}

	uhi_hid_mouse_dev.report_id = get_report_id(uhi_hid_report_data.hid_report, uhi_hid_mouse_dev.hid_report_size);

	usb_printf("report_id = %i\n\r", uhi_hid_mouse_dev.report_id);

	// Init value
	uhi_hid_mouse_dev.report_btn_prev = 0;
	uhi_hid_mouse_start_trans_report(uhi_hid_mouse_dev.dev->address);
	UHI_HID_MOUSE_CHANGE(uhi_hid_mouse_dev.dev, true);
}

void uhi_hid_mouse_enable(uhc_device_t* dev)
{
	if (uhi_hid_mouse_dev.dev != dev) {
		return;  // No interface to enable
	}

	Assert(uhi_hid_mouse_dev.hid_report_size);

	usb_printf(" Requesting HID report (%i bytes) on interface %i\n\r", uhi_hid_mouse_dev.hid_report_size, uhi_hid_mouse_dev.interface_number);
	usb_setup_req_t req;
	req.bmRequestType = USB_REQ_RECIP_INTERFACE|USB_REQ_TYPE_STANDARD|USB_REQ_DIR_IN;
	req.bRequest = USB_REQ_GET_DESCRIPTOR;
	req.wValue = (USB_DT_HID_REPORT << 8);
	req.wIndex = uhi_hid_mouse_dev.interface_number;
	req.wLength = uhi_hid_mouse_dev.hid_report_size;
	if (!uhd_setup_request(dev->address,
		&req,
		uhi_hid_report_data.hid_report, uhi_hid_mouse_dev.hid_report_size,
		NULL, _uhi_hid_mouse_get_hid_report)) {
		usb_printf(" ERROR failure to request HID report\n\r");
	}
}

void uhi_hid_mouse_uninstall(uhc_device_t* dev)
{
	if (uhi_hid_mouse_dev.dev != dev) {
		return; // Device not enabled in this interface
	}

	usb_printf("%s\n\r", __FUNCTION__);
	uhi_hid_mouse_dev.dev = NULL;
	UHI_HID_MOUSE_CHANGE(dev, false);
}
//@}

/**
 * \name Internal routines
 */
//@{

/**
 * \brief Starts the reception of the HID mouse report
 *
 * \param add   USB address to use
 */
static void uhi_hid_mouse_start_trans_report(usb_add_t add)
{
	// Start transfer on interrupt endpoint IN
	uhd_ep_run(add, uhi_hid_mouse_dev.ep_in, true, uhi_hid_mouse_dev.report,
			uhi_hid_mouse_dev.report_size, 0, uhi_hid_mouse_report_reception);
}

/**
 * \brief Decodes the HID mouse report received
 *
 * \param add           USB address used by the transfer
 * \param status        Transfer status
 * \param nb_transfered Number of data transfered
 */
static void uhi_hid_mouse_report_reception(
		usb_add_t add,
		usb_ep_t ep,
		uhd_trans_status_t status,
		iram_size_t nb_transfered)
{
	uint8_t state_prev;
	uint8_t state_new;
	(void)(ep);

	if ((status == UHD_TRANS_NOTRESPONDING) || (status == UHD_TRANS_TIMEOUT)) {
		uhi_hid_mouse_start_trans_report(add);
		return; // HID mouse transfer restart
	}

	if ((status != UHD_TRANS_NOERROR) || (nb_transfered < 4)) {
		return; // HID mouse transfer aborted
	}

	uint8_t* p = uhi_hid_mouse_dev.report;

	if (uhi_hid_mouse_dev.report_id != 0xff) {
		if (uhi_hid_mouse_dev.report_id != *p) {
			usb_printf("discarding report 0x%02x\n\r", *p);
			uhi_hid_mouse_start_trans_report(add);
		}
		if (nb_transfered < 5) {
			usb_printf("discarding short report %i bytes\n\r", nb_transfered);
			uhi_hid_mouse_start_trans_report(add);
		}
		p++;
	}

	usb_printmem(uhi_hid_mouse_dev.report, nb_transfered);

	// Decode buttons
	state_prev = uhi_hid_mouse_dev.report_btn_prev;
	state_new = p[UHI_HID_MOUSE_BTN];
	if ((state_prev & 0x01) != (state_new & 0x01)) {
		UHI_HID_MOUSE_EVENT_BTN_LEFT((state_new & 0x01) ? true : false);
	}
	if ((state_prev & 0x02) != (state_new & 0x02)) {
		UHI_HID_MOUSE_EVENT_BTN_RIGHT((state_new & 0x02) ? true : false);
	}
	if ((state_prev & 0x04) != (state_new & 0x04)) {
		UHI_HID_MOUSE_EVENT_BTN_MIDDLE((state_new & 0x04) ? true : false);
	}
	uhi_hid_mouse_dev.report_btn_prev = state_new;

	// Decode moves
	if ((p[UHI_HID_MOUSE_MOV_X] != 0)
			|| (p[UHI_HID_MOUSE_MOV_Y] != 0)
			|| (p[UHI_HID_MOUSE_MOV_SCROLL] != 0)) {
		UHI_HID_MOUSE_EVENT_MOVE(
				(int8_t)p[UHI_HID_MOUSE_MOV_X],
				(int8_t)p[UHI_HID_MOUSE_MOV_Y],
				(int8_t)p[UHI_HID_MOUSE_MOV_SCROLL]);
	}

	uhi_hid_mouse_start_trans_report(add);
}
//@}

//@}
