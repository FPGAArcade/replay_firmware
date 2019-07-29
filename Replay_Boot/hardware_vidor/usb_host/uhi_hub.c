// ADD REPLAY LICENSE!

#include "conf_usb_host.h"
#include "uhi_hub.h"
#include "uhc.h"
#include <string.h>

enum usb_hub_port_status {
	USB_HUB_STATUS_PORT_CONNECTION   = (1 << 0),
	USB_HUB_STATUS_PORT_ENABLE       = (1 << 1),
	USB_HUB_STATUS_PORT_SUSPEND      = (1 << 2),
	USB_HUB_STATUS_PORT_OVER_CURRENT = (1 << 3),
	USB_HUB_STATUS_PORT_RESET        = (1 << 4),
	// bits 5-7 reserved; return 0 when read
	USB_HUB_STATUS_PORT_POWER        = (1 << 8),
	USB_HUB_STATUS_PORT_LOW_SPEED    = (1 << 9),
	USB_HUB_STATUS_PORT_HIGH_SPEED   = (1 << 10),
	USB_HUB_STATUS_PORT_TEST         = (1 << 11),
	USB_HUB_STATUS_PORT_INDICATOR    = (1 << 12),
	// bits 13-15 reserved; return 0 when read
};


typedef void (*hub_sof_timeout_callback_t) (usb_add_t add);

typedef struct {
	uhc_device_t *dev;

	usb_hub_descriptor_t hub_desc;

	usb_ep_t ep_in;
	uint8_t	sof_timeout;
	uint8_t hub_enabled;
	uint8_t hub_status_length;
	uint8_t hub_status[4];

	uint8_t current_port_enum;
	uint32_t port_status;

	hub_sof_timeout_callback_t sof_timeout_callback;

}uhi_hub_dev_t;

static uhi_hub_dev_t uhi_hubs[] = 
{
	{ .dev = NULL, },
	{ .dev = NULL, },
	{ .dev = NULL, },
	{ .dev = NULL, },
	{ .dev = NULL, },
	{ .dev = NULL, },
};

static uhi_hub_dev_t* allocate_hub(uhc_device_t* dev)
{
	for (size_t i = 0; i < sizeof(uhi_hubs)/sizeof(uhi_hubs[0]); ++i){
		if (uhi_hubs[i].dev != NULL)
			continue;
		memset(&uhi_hubs[i], 0x00, sizeof(uhi_hubs[i]));
		uhi_hubs[i].dev = dev;
		return &uhi_hubs[i];
	}
	return NULL;
}
static void free_hub(uhi_hub_dev_t* hub)
{
	hub->dev = NULL;
}
static uhi_hub_dev_t* find_hub(usb_add_t add)
{
	for (size_t i = 0; i < sizeof(uhi_hubs)/sizeof(uhi_hubs[0]); ++i){
		if (uhi_hubs[i].dev == NULL)
			continue;
		if (uhi_hubs[i].dev->address == add)
			return &uhi_hubs[i];
	}
	return NULL;
}

static void hub_enable_timeout_callback(uhi_hub_dev_t* hub, uint32_t timeout, hub_sof_timeout_callback_t callback)
{
	usb_printf("%s timeout = %i ; callback = %08x\n\r", __FUNCTION__, timeout, callback);
	hub->sof_timeout_callback = callback;
	hub->sof_timeout = timeout;
}

static void hub_clear_port_feature_callback(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans)
{
	(void)add; (void)status; (void)payload_trans;
	usb_printf("%s addr = 0x%02x ; status = 0x%02x ; payload_trans = %d bytes\n\r", __FUNCTION__, add, status, payload_trans);
}

static void hub_set_port_feature_callback(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans)
{
	(void)add; (void)status; (void)payload_trans;
	usb_printf("%s addr = 0x%02x ; status = 0x%02x ; payload_trans = %d bytes\n\r", __FUNCTION__, add, status, payload_trans);
}

static void hub_set_feature_callback(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans)
{
	(void)add; (void)status; (void)payload_trans;
	usb_printf("%s addr = 0x%02x ; status = 0x%02x ; payload_trans = %d bytes\n\r", __FUNCTION__, add, status, payload_trans);
}

static void hub_descriptor_callback(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans)
{
	(void)add; (void)status; (void)payload_trans;
	usb_printf("%s addr = 0x%02x ; status = 0x%02x ; payload_trans = %d bytes\n\r", __FUNCTION__, add, status, payload_trans);

	uhi_hub_dev_t* uhi_hub_dev = find_hub(add);
	Assert(uhi_hub_dev);
	print_usb_desc(&uhi_hub_dev->hub_desc);
}

static void hub_enumerate_port_status(usb_add_t add, uint8_t start_port);

static void hub_continue_port_enum(usb_add_t add) {
	usb_printf("%s %i\n\r", __FUNCTION__, add);
	uhi_hub_dev_t* uhi_hub_dev = find_hub(add);
	Assert(uhi_hub_dev);

	if (uhc_is_enumerating()) {
		hub_enable_timeout_callback(uhi_hub_dev, 250, hub_continue_port_enum);
	} else {
		hub_enumerate_port_status(uhi_hub_dev->dev->address, uhi_hub_dev->current_port_enum);
	}
}

static void uhi_hub_start_trans_report(usb_add_t add);

static void hub_get_port_status_callback(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans)
{
	(void)add; (void)status; (void)payload_trans;

	uhi_hub_dev_t* uhi_hub_dev = find_hub(add);
	Assert(uhi_hub_dev);
	const uint8_t port = uhi_hub_dev->current_port_enum;

	usb_printf("%s addr = 0x%02x ; status = 0x%02x ; payload_trans = %d bytes\n\r", __FUNCTION__, add, status, payload_trans);
	usb_printf("   port %d status = %08x\n\r", port, uhi_hub_dev->port_status);

	uint32_t port_status = uhi_hub_dev->port_status;

	uhd_speed_t speed = UHD_SPEED_FULL; // default speed

	if (port_status & USB_HUB_STATUS_PORT_LOW_SPEED)
		speed = UHD_SPEED_LOW;
	else if (port_status & USB_HUB_STATUS_PORT_HIGH_SPEED)
		speed = UHD_SPEED_HIGH;

	Assert(speed != UHD_SPEED_HIGH);		// this is not supported and should never happen

	// remove bits to enable switch case below
	port_status &= ~(USB_HUB_STATUS_PORT_LOW_SPEED | USB_HUB_STATUS_PORT_HIGH_SPEED | USB_HUB_STATUS_PORT_TEST | USB_HUB_STATUS_PORT_INDICATOR);

	#define DEVICE_DISCONNECTED   (USB_HUB_STATUS_PORT_CONNECTION << 16) | USB_HUB_STATUS_PORT_POWER
	#define DEVICE_CONNECTED      (USB_HUB_STATUS_PORT_CONNECTION << 16) | USB_HUB_STATUS_PORT_POWER | USB_HUB_STATUS_PORT_CONNECTION
	#define DEVICE_RESET_COMPLETE      (USB_HUB_STATUS_PORT_RESET << 16) | USB_HUB_STATUS_PORT_POWER | USB_HUB_STATUS_PORT_CONNECTION | USB_HUB_STATUS_PORT_ENABLE

	usb_printf("   reduced port status = %08x\n\r", port_status);
	usb_printf("   speed = %s\n\r", speed == UHD_SPEED_FULL ? "full" : "low");

	usb_setup_req_t req;
	switch(port_status)
	{
		case DEVICE_DISCONNECTED:
			usb_printf("   device on port %d disconnected\n\r", port);

			usb_fill_ClearPortFeature_req(&req, USB_HUB_FTR_C_PORT_ENABLE, port);
			if (!uhd_setup_request(add, &req, NULL, 0, NULL, hub_clear_port_feature_callback)) { usb_printf(" ERROR failure to clear port feature\n\r"); }

			usb_fill_ClearPortFeature_req(&req, USB_HUB_FTR_C_PORT_CONNECTION, port);
			if (!uhd_setup_request(add, &req, NULL, 0, NULL, hub_clear_port_feature_callback)) { usb_printf(" ERROR failure to clear port feature\n\r"); }

			uhc_notify_connection_hub(false, uhi_hub_dev->dev, port, speed);

			break;
		case DEVICE_CONNECTED:
			usb_printf("   device on port %d connected\n\r", port);

			usb_fill_ClearPortFeature_req(&req, USB_HUB_FTR_C_PORT_ENABLE, port);
			if (!uhd_setup_request(add, &req, NULL, 0, NULL, hub_clear_port_feature_callback)) { usb_printf(" ERROR failure to clear port feature\n\r"); }

			usb_fill_ClearPortFeature_req(&req, USB_HUB_FTR_C_PORT_CONNECTION, port);
			if (!uhd_setup_request(add, &req, NULL, 0, NULL, hub_clear_port_feature_callback)) { usb_printf(" ERROR failure to clear port feature\n\r"); }

			usb_fill_SetPortFeature_req(&req, USB_HUB_FTR_PORT_RESET, port);
			if (!uhd_setup_request(add, &req, NULL, 0, NULL, hub_set_port_feature_callback)) { usb_printf(" ERROR failure to set port feature\n\r"); }

			// skip continued enumeration
			uhi_hub_start_trans_report(add);
			return;

		case DEVICE_RESET_COMPLETE:
			usb_printf("   device on port %d reset done\n\r", port);

			usb_fill_ClearPortFeature_req(&req, USB_HUB_FTR_C_PORT_RESET, port);
			if (!uhd_setup_request(add, &req, NULL, 0, NULL, hub_clear_port_feature_callback)) { usb_printf(" ERROR failure to clear port feature\n\r"); }

			usb_fill_ClearPortFeature_req(&req, USB_HUB_FTR_C_PORT_CONNECTION, port);
			if (!uhd_setup_request(add, &req, NULL, 0, NULL, hub_clear_port_feature_callback)) { usb_printf(" ERROR failure to clear port feature\n\r"); }

			uhc_notify_connection_hub(true, uhi_hub_dev->dev, port, speed);

			break;
		default:
			usb_printf("  ignoring state.. \n\r");
	}

	hub_continue_port_enum(add);
}

static void hub_enumerate_port_status(usb_add_t add, uint8_t start_port)
{
	usb_printf("%s add = %i start port = %i\n\r", __FUNCTION__, add, start_port);

	uhi_hub_dev_t* uhi_hub_dev = find_hub(add);
	Assert(uhi_hub_dev);

	const uint8_t max_num_ports = 8*uhi_hub_dev->hub_status_length-1;
//	uint8_t* stat = &uhi_hub_dev.hub_status[uhi_hub_dev.hub_status_length-1];
	int port = 0;
	for (port = start_port+1; port <= uhi_hub_dev->hub_desc.bNbrPorts; port++) {

		// numport = 12 => 2 bytes
		// [xxx11111][1111111_]
		// port 1	15-1  = 14	=> 1
		// port 2	15-2  = 13	=> 1
		// port 3	15-3  = 12	=> 1
		// port 4	15-4  = 11	=> 1
		// port 5	15-5  = 10	=> 1
		// port 6	15-6  = 9	=> 1
		// port 7	15-7  = 8	=> 1
		// port 8	15-8  = 7	=> 0
		// port 9	15-9  = 6	=> 0
		// port 10	15-10 = 5	=> 0
		// port 11	15-11 = 4	=> 0
		// port 12	15-12 = 3	=> 0

		const uint8_t status_offset = (max_num_ports - port) >> 3;
		const uint8_t stat = uhi_hub_dev->hub_status[status_offset];

		const uint8_t port_nr = (port & 0x7);		// look at the port status in sections of 8 (one byte)
		const uint8_t mask = 1 << port_nr;

		usb_printf("   port = %d ; port_nr = %d ; %02x & %02x => %02x\n\r", port, port_nr, stat, mask, (stat & mask));

		if ((stat & mask) == 0)
			continue;

		usb_printf("   sending GetPortStatus(%d)\n\r", port);
		uhi_hub_dev->current_port_enum = port;
		usb_setup_req_t req;
		req.bmRequestType = USB_REQ_RECIP_OTHER|USB_REQ_TYPE_CLASS|USB_REQ_DIR_IN;
		req.bRequest = USB_REQ_GET_STATUS;
		req.wValue = 0;
		req.wIndex = port;
		req.wLength = sizeof(uint32_t);
		if (!uhd_setup_request(add,
			&req,
			(uint8_t*)&uhi_hub_dev->port_status, sizeof(uhi_hub_dev->port_status),
			NULL, hub_get_port_status_callback)) {
			usb_printf(" ERROR failure to get port status\n\r");
		}
		break;
	}

	if (port > uhi_hub_dev->hub_desc.bNbrPorts)
		uhi_hub_start_trans_report(add);
}

static void uhi_hub_report_reception(
		usb_add_t add,
		usb_ep_t ep,
		uhd_trans_status_t status,
		iram_size_t nb_transfered);

static void uhi_hub_start_trans_report(usb_add_t add)
{
	usb_printf("%s\n\r", __FUNCTION__);
	uhi_hub_dev_t* uhi_hub_dev = find_hub(add);
	Assert(uhi_hub_dev);
	if (!uhd_ep_run(add, uhi_hub_dev->ep_in, true, &uhi_hub_dev->hub_status[0],
			uhi_hub_dev->hub_status_length, 0, uhi_hub_report_reception)) {
		usb_printf("   -> failed to run endpoint\n\r");
	}
}
static void uhi_hub_report_reception(
		usb_add_t add,
		usb_ep_t ep,
		uhd_trans_status_t status,
		iram_size_t nb_transfered)
{
	(void)ep;

	usb_printf("%s addr = 0x%02x ; ep = 0x%02x ; status = 0x%02x ; nb_transfered = %d bytes\n\r", __FUNCTION__, add, ep, status, nb_transfered);

	if (status == UHD_TRANS_DISCONNECT) {
		return;
	}

	if ((status == UHD_TRANS_NOTRESPONDING) || (status == UHD_TRANS_TIMEOUT)) {
		uhi_hub_start_trans_report(add);
		return;
	}

	uhi_hub_dev_t* uhi_hub_dev = find_hub(add);
	Assert(uhi_hub_dev);

	if ((status != UHD_TRANS_NOERROR) || (nb_transfered < uhi_hub_dev->hub_status_length)) {
		usb_printf(" hub status read error\n\r");
		return;
	}

	usb_printf("  .. hub_status (%d) = 0x%02x,0x%02x,0x%02x,0x%02x\n\r", uhi_hub_dev->hub_status_length, uhi_hub_dev->hub_status[0], uhi_hub_dev->hub_status[1], uhi_hub_dev->hub_status[2], uhi_hub_dev->hub_status[3]);

	hub_enumerate_port_status(add, 0);
}



uhc_enum_status_t uhi_hub_install(uhc_device_t* dev, usb_conf_desc_t* conf_desc)
{
	usb_printf("%s\n\r", __FUNCTION__);
	if (dev->dev_desc.bDeviceClass != HUB_CLASS) {
		usb_printf("  -> not a hub; skipping\n\r");
		return UHC_ENUM_UNSUPPORTED;
	}

	uhi_hub_dev_t* uhi_hub_dev = NULL;

	uint16_t conf_desc_lgt = le16_to_cpu(conf_desc->wTotalLength);
	usb_iface_desc_t* ptr_iface = (usb_iface_desc_t*)conf_desc;
	bool b_iface_supported = false;
	while(conf_desc_lgt && uhi_hub_dev == NULL) {
		switch (ptr_iface->bDescriptorType) {

		case USB_DT_CONFIGURATION:
			usb_printf(" .. USB_DT_CONFIGURATION\n\r");
			print_conf_desc((usb_conf_desc_t*)ptr_iface, false);
			break;

		case USB_DT_INTERFACE:
			usb_printf(" .. USB_DT_INTERFACE\n\r");
			print_iface_desc(ptr_iface);
			if (ptr_iface->bInterfaceClass   == HUB_CLASS) {
				usb_printf("  -> HUB_CLASS => supported\n\r");
				b_iface_supported = true;
			} else {
				// Stop allocation endpoint(s)
				usb_printf("  -> not HUB_CLASS; skipping\n\r");
				b_iface_supported = false;
			}
			break;

		case USB_DT_ENDPOINT:
			usb_printf(" .. USB_DT_ENDPOINT\n\r");
			usb_ep_desc_t* ptr_ep = (usb_ep_desc_t*)ptr_iface;
			print_ep_desc(ptr_ep);

			//  Allocation of the endpoint
			if (!b_iface_supported) {
				break;
			}
			if (!uhd_ep_alloc(dev->address, ptr_ep, dev->speed)) {
				usb_printf("  -> end-point allocation failure\n\r");
				return UHC_ENUM_HARDWARE_LIMIT; // Endpoint allocation fail
			}
			uhi_hub_dev = allocate_hub(dev);
			Assert(uhi_hub_dev);
			Assert(ptr_ep->bEndpointAddress & USB_EP_DIR_IN);
			Assert(ptr_ep->wMaxPacketSize <= sizeof(uhi_hub_dev->hub_status));
			uhi_hub_dev->ep_in = ptr_ep->bEndpointAddress;
			uhi_hub_dev->hub_status_length = ptr_ep->wMaxPacketSize;
			usb_printf("  -> HUB_CLASS endpoint alloc'd\n\r");
			break;

		default:
			// Ignore descriptor
			usb_printf(" .. (%02x)\n\r", ptr_iface->bDescriptorType);
			break;
		}
		Assert(conf_desc_lgt>=ptr_iface->bLength);
		conf_desc_lgt -= ptr_iface->bLength;
		ptr_iface = (usb_iface_desc_t*)((uint8_t*)ptr_iface + ptr_iface->bLength);
	}

	if (!uhi_hub_dev || uhi_hub_dev->dev == NULL) {
		usb_printf("  -> failed to configure hub; aborting\n\r");
		return UHC_ENUM_UNSUPPORTED;
	}

	usb_printf("  -> HUB_CLASS detected\n\r");
	print_uhc_dev(dev);

	// Send USB device descriptor request
	usb_setup_req_t req;
	req.bmRequestType = USB_REQ_RECIP_DEVICE|USB_REQ_TYPE_CLASS|USB_REQ_DIR_IN;
	req.bRequest = USB_REQ_GET_DESCRIPTOR;
	req.wValue = (USB_DT_HUB << 8);
	req.wIndex = 0;
	req.wLength = sizeof(usb_dev_desc_t);
	if (!uhd_setup_request(dev->address,
		&req,
		(uint8_t *) & uhi_hub_dev->hub_desc,
		sizeof(usb_hub_descriptor_t),
		NULL, hub_descriptor_callback)) {
		return UHC_ENUM_MEMORY_LIMIT;
	}

	usb_printf("  -> HUB success\n\r");

	return UHC_ENUM_SUCCESS;
}

void uhi_hub_enable(uhc_device_t* dev)
{
	usb_printf("%s\n\r", __FUNCTION__);

	uhi_hub_dev_t* uhi_hub_dev = find_hub(dev->address);
	if (!uhi_hub_dev) {
		usb_printf("  -> not a hub\n\r");
		return;
	}

	uhi_hub_dev->hub_enabled = 1;

	for (int port = 1; port <= uhi_hub_dev->hub_desc.bNbrPorts; port++) {
		usb_printf(" Setting power on port %i\n\r",port);

		usb_setup_req_t req;
		req.bmRequestType = USB_REQ_RECIP_OTHER|USB_REQ_TYPE_CLASS|USB_REQ_DIR_OUT;
		req.bRequest = USB_REQ_SET_FEATURE;
		req.wValue = USB_HUB_FTR_PORT_POWER;
		req.wIndex = port;
		req.wLength = 0;
		if (!uhd_setup_request(dev->address,
			&req,
			NULL,
			0,
			NULL, hub_set_feature_callback)) {
			usb_printf(" ERROR failure to set power on\n\r");
			break;
		}
	}

	UHI_HUB_CHANGE(dev, true);
	uhi_hub_start_trans_report(dev->address);
}

void uhi_hub_uninstall(uhc_device_t* dev)
{
	uhi_hub_dev_t* uhi_hub_dev = find_hub(dev->address);
	if (!uhi_hub_dev) {
		return;
	}

	usb_printf("%s\n\r", __FUNCTION__);
	free_hub(uhi_hub_dev);
	UHI_HUB_CHANGE(dev, false);
}

void uhi_hub_update(bool b_micro)
{
	if (!b_micro) {

		// Manage SOF timeout
		for (size_t i = 0; i < sizeof(uhi_hubs)/sizeof(uhi_hubs[0]); ++i){
			uhi_hub_dev_t* uhi_hub_dev = &uhi_hubs[i];
			if (uhi_hub_dev->dev == NULL) {
				continue;
			}
			if (uhi_hub_dev->sof_timeout) {
				if (--uhi_hub_dev->sof_timeout == 0) {
					usb_printf("%s timeout addr %i ; callback = %08x\n\r", __FUNCTION__, uhi_hub_dev->dev->address, uhi_hub_dev->sof_timeout_callback);
					Assert(uhi_hub_dev->sof_timeout_callback);
					uhi_hub_dev->sof_timeout_callback(uhi_hub_dev->dev->address);
				}

			}
		}

	}
}

void uhi_hub_suspend(uhc_device_t* dev)
{
	(void)dev;
	if (!dev->hub)
		return;
	uhi_hub_dev_t* uhi_hub_dev = find_hub(dev->hub->address);
	Assert(uhi_hub_dev);
	usb_printf("%s %08x vs %08x\n\r", __FUNCTION__, dev, uhi_hub_dev->dev);
}

void uhi_hub_send_reset(uhc_device_t* dev, uhd_callback_reset_t callback)
{
	(void)dev;
	if (!dev->hub)
		return;
	uhi_hub_dev_t* uhi_hub_dev = find_hub(dev->hub->address);
	Assert(uhi_hub_dev);
	usb_printf("%s %08x vs %08x\n\r", __FUNCTION__, dev, uhi_hub_dev->dev);
	callback();
}
