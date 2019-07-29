// ADD REPLAY LICENSE!

#include "conf_usb_host.h"
#include "usb_protocol.h"
#include "uhd.h"
#include "uhc.h"
#include "uhi_hid_keyboard.h"
#include "uhi_hid.h"
#include <string.h>

#define UHI_HID_KEYBOARD_MODIFIERS   0
#define UHI_HID_KEYBOARD_RESERVED    1
#define UHI_HID_KEYBOARD_KEYS        2

typedef struct {
	uhc_device_t *dev;
	usb_ep_t ep_in;
	uint8_t interface_number;
	uint8_t report_size;
	uint8_t leds;
	uint16_t hid_report_size;
	COMPILER_WORD_ALIGNED uint8_t report[8];
	uint8_t	last_report[8];
}uhi_hid_keyboard_dev_t;

static uhi_hid_keyboard_dev_t uhi_hid_keyboard_dev = {
	.dev = NULL,
	.leds = 0,
	};

static void uhi_hid_keyboard_start_trans_report(usb_add_t add);
static void uhi_hid_keyboard_report_reception(
		usb_add_t add,
		usb_ep_t ep,
		uhd_trans_status_t status,
		iram_size_t nb_transfered);

uhc_enum_status_t uhi_hid_keyboard_install(uhc_device_t* dev, usb_conf_desc_t* conf_desc)
{
	usb_printf("%s\n\r", __FUNCTION__);
	uhi_hid_keyboard_dev.hid_report_size = 0;

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
			&& (ptr_iface->bInterfaceProtocol == HID_PROTOCOL_KEYBOARD) ) {
				// USB HID Keyboard interface found
				// Start allocation endpoint(s)
				usb_printf("  -> HID_CLASS and HID_PROTOCOL_KEYBOARD => supported\n\r");
				b_iface_supported = true;
				uhi_hid_keyboard_dev.interface_number = ptr_iface->bInterfaceNumber;
			} else {
				// Stop allocation endpoint(s)
				usb_printf("  -> not HID_CLASS/HID_PROTOCOL_KEYBOARD; skipping\n\r");
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

			uhi_hid_keyboard_dev.hid_report_size = le16_to_cpu(ptr_hid->wDescriptorLength);
			Assert(uhi_hid_keyboard_dev.hid_report_size <= sizeof(uhi_hid_report_data.hid_report));

			usb_printf("  -> uhi_hid_keyboard_dev.hid_report_size = %i bytes\n\r", uhi_hid_keyboard_dev.hid_report_size);

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
			if (uhi_hid_keyboard_dev.dev != NULL) {
				return UHC_ENUM_SOFTWARE_LIMIT; // Device already allocated
			}
			if (!uhd_ep_alloc(dev->address, (usb_ep_desc_t*)ptr_iface, dev->speed)) {
				return UHC_ENUM_HARDWARE_LIMIT; // Endpoint allocation fail
			}
			Assert(((usb_ep_desc_t*)ptr_iface)->bEndpointAddress & USB_EP_DIR_IN);
			uhi_hid_keyboard_dev.ep_in = ((usb_ep_desc_t*)ptr_iface)->bEndpointAddress;
			uhi_hid_keyboard_dev.report_size =
					le16_to_cpu(((usb_ep_desc_t*)ptr_iface)->wMaxPacketSize);
			Assert(uhi_hid_keyboard_dev.report_size <= sizeof(uhi_hid_keyboard_dev.report));
			uhi_hid_keyboard_dev.dev = dev;
			// All endpoints of all interfaces supported allocated
			usb_printf("  -> HID_KEYBOARD success\n\r");
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

static void _uhi_hid_keyboard_get_hid_report(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans)
{
	(void)add;
	usb_printf("%s addr = 0x%02x ; status = 0x%02x ; payload_trans = %d bytes\n\r", __FUNCTION__, add, status, payload_trans);

	if (status != UHD_TRANS_NOERROR || payload_trans != uhi_hid_keyboard_dev.hid_report_size) {
		usb_printf(" ERROR failure to retrieve HID report\n\r");
		return;
	}

	uint8_t report_id = get_report_id(uhi_hid_report_data.hid_report, uhi_hid_keyboard_dev.hid_report_size);
	(void)report_id;
	usb_printf("report_id = %i\n\r", report_id);

	// Init value
	memset(uhi_hid_keyboard_dev.last_report, 0x00, sizeof(uhi_hid_keyboard_dev.last_report));
	uhi_hid_keyboard_start_trans_report(uhi_hid_keyboard_dev.dev->address);
	UHI_HID_KEYBOARD_CHANGE(uhi_hid_keyboard_dev.dev, true);
}

void uhi_hid_keyboard_enable(uhc_device_t* dev)
{
	if (uhi_hid_keyboard_dev.dev != dev) {
		return;  // No interface to enable
	}

	Assert(uhi_hid_keyboard_dev.hid_report_size);

	usb_printf(" Requesting HID report (%i bytes) on interface %i\n\r", uhi_hid_keyboard_dev.hid_report_size, uhi_hid_keyboard_dev.interface_number);
	usb_setup_req_t req;
	req.bmRequestType = USB_REQ_RECIP_INTERFACE|USB_REQ_TYPE_STANDARD|USB_REQ_DIR_IN;
	req.bRequest = USB_REQ_GET_DESCRIPTOR;
	req.wValue = (USB_DT_HID_REPORT << 8);
	req.wIndex = uhi_hid_keyboard_dev.interface_number;
	req.wLength = uhi_hid_keyboard_dev.hid_report_size;
	if (!uhd_setup_request(dev->address,
		&req,
		uhi_hid_report_data.hid_report, uhi_hid_keyboard_dev.hid_report_size,
		NULL, _uhi_hid_keyboard_get_hid_report)) {
		usb_printf(" ERROR failure to request HID report\n\r");
	}
}

void uhi_hid_keyboard_uninstall(uhc_device_t* dev)
{
	if (uhi_hid_keyboard_dev.dev != dev) {
		return; // Device not enabled in this interface
	}

	usb_printf("%s\n\r", __FUNCTION__);
	uhi_hid_keyboard_dev.dev = NULL;
	UHI_HID_KEYBOARD_CHANGE(dev, false);
}

static void uhi_hid_keyboard_start_trans_report(usb_add_t add)
{
	// Start transfer on interrupt endpoint IN
	uhd_ep_run(add, uhi_hid_keyboard_dev.ep_in, true, uhi_hid_keyboard_dev.report,
			uhi_hid_keyboard_dev.report_size, 0, uhi_hid_keyboard_report_reception);
}

static void uhi_hid_keyboard_process_leds(uint8_t key)
{
	uint8_t leds = uhi_hid_keyboard_dev.leds;

	switch(key) {
		case HID_KEYPAD_NUM_LOCK:
			uhi_hid_keyboard_dev.leds ^= HID_LED_NUM_LOCK;
			break;
		case HID_CAPS_LOCK:
			uhi_hid_keyboard_dev.leds ^= HID_LED_CAPS_LOCK;
			break;
		case HID_SCROLL_LOCK:
			uhi_hid_keyboard_dev.leds ^= HID_LED_SCROLL_LOCK;
			break;
	}

	if (leds == uhi_hid_keyboard_dev.leds)
		return;

	usb_printf(" Setting LEDs to %02x\n\r", uhi_hid_keyboard_dev.leds);
	usb_setup_req_t req;
	req.bmRequestType = USB_REQ_RECIP_INTERFACE|USB_REQ_TYPE_CLASS|USB_REQ_DIR_OUT;
	req.bRequest = USB_REQ_HID_SET_REPORT;
	req.wValue = REPORT_TYPE_OUTPUT << 8;
	req.wIndex = uhi_hid_keyboard_dev.interface_number;
	req.wLength = sizeof(uhi_hid_keyboard_dev.leds);
	if (!uhd_setup_request(uhi_hid_keyboard_dev.dev->address,
		&req,
		&uhi_hid_keyboard_dev.leds, sizeof(uhi_hid_keyboard_dev.leds),
		NULL, NULL)) {
		usb_printf(" ERROR failure to set LEDs\n\r");
	}
}

static void uhi_hid_keyboard_report_reception(
		usb_add_t add,
		usb_ep_t ep,
		uhd_trans_status_t status,
		iram_size_t nb_transfered)
{
	(void)(ep);

	if ((status == UHD_TRANS_NOTRESPONDING) || (status == UHD_TRANS_TIMEOUT)) {
		uhi_hid_keyboard_start_trans_report(add);
		return; // HID keyboard transfer restart
	}

	if ((status != UHD_TRANS_NOERROR) || (nb_transfered < 4)) {
		return; // HID keyboard transfer aborted
	}

	const uint8_t* report = uhi_hid_keyboard_dev.report;
	uint8_t* last_report = uhi_hid_keyboard_dev.last_report;
	const uint8_t length = sizeof(uhi_hid_keyboard_dev.report);

	usb_printmem(report, nb_transfered);

	if (report[UHI_HID_KEYBOARD_MODIFIERS] != last_report[UHI_HID_KEYBOARD_MODIFIERS]) {
		UHI_HID_KEYBOARD_EVENT_META(last_report[UHI_HID_KEYBOARD_MODIFIERS], report[UHI_HID_KEYBOARD_MODIFIERS]);
	}

	for (int i = UHI_HID_KEYBOARD_KEYS; i < length; ++i) {

		// 0x01 =>Overrun Error, 0x02 =>POST Fail, 0x03 =>ErrorUndefined
		bool add_key =      report[i] >= HID_A;
		bool rem_key = last_report[i] >= HID_A;

		for (int j = UHI_HID_KEYBOARD_KEYS; j < length; ++j) {

			if (report[i] == last_report[j]) {
				add_key = false;
			}

			if (last_report[i] == report[j]) {
				rem_key = false;
			}
		}
		if (add_key) {
			uhi_hid_keyboard_process_leds(report[i]);
			UHI_HID_KEYBOARD_EVENT_KEY_DOWN(report[0], report[i]);
		}
		if (rem_key) {
			UHI_HID_KEYBOARD_EVENT_KEY_UP(last_report[0], last_report[i]);
		}
	}

	memcpy(last_report, report, length);

	uhi_hid_keyboard_start_trans_report(add);
}
//@}

//@}
