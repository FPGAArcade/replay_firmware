/**
 * \file
 *
 * \brief USB Host Controller (UHC)
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
#include "uhi.h"
#include "uhc.h"
#include <stdlib.h>
#include <string.h>

#ifndef USB_HOST_UHI
#  error USB_HOST_UHI must be defined with unless one UHI interface in conf_usb_host.h file.
#endif

// Optional UHC callbacks
#ifndef UHC_CONNECTION_EVENT
#  define UHC_CONNECTION_EVENT(dev,b_present)
#endif
#ifndef UHC_DEVICE_CONF
#  define UHC_DEVICE_CONF(dev) 1
#endif
#ifndef UHC_ENUM_EVENT
#  define UHC_ENUM_EVENT(dev,event)
#endif
#ifndef UHC_WAKEUP_EVENT
#  define UHC_WAKEUP_EVENT()
#endif

/**
 * \ingroup uhc_group
 * \defgroup uhc_group_interne Implementation of UHC
 *
 * Internal implementation
 * @{
 */

//! \name Internal variables to manage the USB host stack
//! @{

#ifdef USB_HOST_HUB_SUPPORT
#define USB_MAX_NUM_DEVICES (USB_PIPE_NUM)
#else
#define USB_MAX_NUM_DEVICES 1
#endif

static uint32_t g_devices_in_use = 0;	//bitfield
static uhc_device_t g_devices[USB_PIPE_NUM];

//! Entry point of all devices connected on USB tree
static uhc_device_t g_uhc_device_root;

//! Number of enumeration try
static uint8_t uhc_enum_try;

//! Maximum try to enumerate a device
#define UHC_ENUM_NB_TRY 4

//! Entry point of all devices connected on USB tree
#define UHC_USB_ADD_NOT_VALID 0xFF

#ifdef USB_HOST_HUB_SUPPORT
//! USB address of the USB device under enumeration process
#  define UHC_DEVICE_ENUM_ADD uhc_dev_enum->address

//! Device under enumeration process
static uhc_device_t *uhc_dev_enum;

//! Total power of the connected devices
static uint16_t uhc_power_running = 0;

#else
//! USB address of the USB device under enumeration process
#  define UHC_DEVICE_ENUM_ADD 1 // Static without USB HUB

//! Device under enumeration process
#  define uhc_dev_enum        (&g_uhc_device_root) // Static without USB HUB

//! Total power of the devices connected
#  define uhc_power_running   0 // No used without USB HUB
#endif

//! Type of callback on a SOF timeout
typedef void (*uhc_sof_timeout_callback_t) (void);

//! Callback currently registered on a SOF timeout
static uhc_sof_timeout_callback_t uhc_sof_timeout_callback;

//! Number of SOF remaining before call uhc_sof_timeout_callback callback
uint8_t uhc_sof_timeout;

//! Array of all UHI available
static uhi_api_t uhc_uhis[] = {USB_HOST_UHI};

//! Number of UHI available
#define UHC_NB_UHI  (sizeof(uhc_uhis)/sizeof(uhc_uhis[0]))

//! Volatile flag to pool the end of Get USB string setup request
static volatile bool uhc_setup_request_finish;

//! Volatile flag to know the status of Get USB string setup request
static volatile bool uhc_setup_request_finish_status;

//! @}


//! \name Internal functions to manage the USB device enumeration
//! @{
static void uhc_enable_timeout_callback(
		uint8_t timeout,
		uhc_sof_timeout_callback_t callback);
static void uhc_enumeration_suspend(void);
static void uhc_enumeration_reset(uhd_callback_reset_t callback);
static void uhc_connection_tree(bool b_plug, uhc_device_t* dev);
static void uhc_enumeration_step1(void);
static void uhc_enumeration_step2(void);
static void uhc_enumeration_step3(void);
static void uhc_enumeration_step4(void);
static void uhc_enumeration_step5(void);
static void uhc_enumeration_step6(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans);
static void uhc_enumeration_step7(void);
static void uhc_enumeration_step8(void);
static void uhc_enumeration_step9(void);
static void uhc_enumeration_step10(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans);
static void uhc_enumeration_step11(void);
static void uhc_enumeration_step12(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans);
static void uhc_enumeration_step13(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans);
static void uhc_enumeration_step14(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans);
static void uhc_enumeration_step15(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans);
#ifdef USB_HOST_LPM_SUPPORT
static void uhc_enumeration_step16_lpm(void);
static void uhc_enumeration_step17_lpm(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans);
static void uhc_enumeration_step18_lpm(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans);
#endif // USB_HOST_LPM_SUPPORT
static void uhc_enumeration_error(uhc_enum_status_t status);
//! @}

static void uhc_remotewakeup(bool b_enable);
static void uhc_setup_request_callback(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans);



/**
 * \brief Enable a internal timeout on SOF events
 *
 * \param timeout  value of timeout (ms)
 * \param callback Callback to call at the end of timeout
 */
static void uhc_enable_timeout_callback(uint8_t timeout,
		uhc_sof_timeout_callback_t callback)
{
	uhc_sof_timeout_callback = callback;
	uhc_sof_timeout = timeout;
}

/**
 * \brief Enters a specific device in USB suspend mode
 * Suspend the USB line or a port on USB hub.
 */
static void uhc_enumeration_suspend(void)
{
#ifdef USB_HOST_HUB_SUPPORT
	if (&g_uhc_device_root != uhc_dev_enum) {
		// Device connected on USB hub
		uhi_hub_suspend(uhc_dev_enum);
	} else
#endif
	{
		// Suspend USB line
		uhd_suspend();
	}
}

/**
 * \brief Sends the USB Reset signal on the USB line of a device
 *
 * \param callback Callback to call at the end of Reset signal
 */
static void uhc_enumeration_reset(uhd_callback_reset_t callback)
{
	usb_printf("%s\n\r", __FUNCTION__);

	// Reset USB line
#ifdef USB_HOST_HUB_SUPPORT
	if (&g_uhc_device_root != uhc_dev_enum) {
		// Device connected on USB hub
		uhi_hub_send_reset(uhc_dev_enum, callback);
	} else
#endif
	{
		uhd_send_reset(callback);
	}
}

/**
 * \brief Manage a device plug or unplug on the USB tree
 *
 * \param b_plug   true, if it is a device connection
 * \param dev      Information about device connected or disconnected
 */
static void uhc_connection_tree(bool b_plug, uhc_device_t* dev)
{
	usb_printf("%s : %s\n\r", __FUNCTION__, b_plug ? "connect" : "disconnect");

	if (b_plug) {
		uhc_enum_try = 1;
#ifdef USB_HOST_HUB_SUPPORT
		uhc_dev_enum = dev;
#endif
		uhc_dev_enum->conf_desc = NULL;
		uhc_dev_enum->address = 0;
		UHC_CONNECTION_EVENT(uhc_dev_enum, true);
		uhc_enumeration_step1();
	} else {
		if (uhc_dev_enum == dev) {
			// Eventually stop enumeration timeout on-going on this device
			uhc_sof_timeout = 0;
		}
		// Abort all transfers (endpoint control and other) and free pipe(s)
		uhd_ep_free(dev->address, 0xFF);

		// Disable all USB interfaces (this includes HUB interface)
		for (uint8_t i = 0; i < UHC_NB_UHI; i++) {
			uhc_uhis[i].uninstall(dev);
		}

		UHC_CONNECTION_EVENT(dev, false);
		dev->address = UHC_USB_ADD_NOT_VALID;
		// Free USB configuration descriptor buffer
		if (dev->conf_desc != NULL) {
			free(dev->conf_desc);
			dev->conf_desc = NULL;
		}
#ifdef USB_HOST_HUB_SUPPORT
		uhc_power_running -= dev->power;
		usb_printf("uhc_power_running = %i (%i)\n\r", uhc_power_running, dev->power);
		if (&g_uhc_device_root != dev) {
			// It is on a USB hub
			if (dev->prev)
			dev->prev->next = dev->next;
			if (dev->next)
			dev->next->prev = dev->prev;
		}
#endif
	}
}

/**
 * \brief Device enumeration step 1
 * Reset USB line.
 */
static void uhc_enumeration_step1(void)
{
	usb_printf("%s\n\r", __FUNCTION__);
	uhc_enumeration_reset(uhc_enumeration_step2);
}

/**
 * \brief Device enumeration step 2
 * Lets USB line in IDLE state during 20ms.
 */
static void uhc_enumeration_step2(void)
{
	usb_printf("%s\n\r", __FUNCTION__);
	uhc_enable_timeout_callback(20, uhc_enumeration_step3);
}

/**
 * \brief Device enumeration step 3
 * Reset USB line.
 */
static void uhc_enumeration_step3(void)
{
	usb_printf("%s\n\r", __FUNCTION__);
	uhc_enumeration_reset(uhc_enumeration_step4);
}

/**
 * \brief Device enumeration step 4
 * Lets USB line in IDLE state during 100ms.
 */
static void uhc_enumeration_step4(void)
{
	usb_printf("%s\n\r", __FUNCTION__);
	if (uhc_dev_enum->hub == 0)
	uhc_dev_enum->speed = uhd_get_speed();
	uhc_enable_timeout_callback(100, uhc_enumeration_step5);
}

/**
 * \brief Device enumeration step 5
 * Requests the USB device descriptor.
 * This setup request can be aborted
 * because the control endpoint size is unknown.
 */
static void uhc_enumeration_step5(void)
{
	usb_printf("%s\n\r", __FUNCTION__);
	usb_setup_req_t req;

	req.bmRequestType = USB_REQ_RECIP_DEVICE|USB_REQ_TYPE_STANDARD|USB_REQ_DIR_IN;
	req.bRequest = USB_REQ_GET_DESCRIPTOR;
	req.wValue = (USB_DT_DEVICE << 8);
	req.wIndex = 0;
	req.wLength = offsetof(uhc_device_t, dev_desc.bMaxPacketSize0)
			+ sizeof(uhc_dev_enum->dev_desc.bMaxPacketSize0);

	// After a USB reset, the reallocation is required
	uhd_ep_free(0, 0);
	if (!uhd_ep0_alloc(0, 64)) {
		uhc_enumeration_error(UHC_ENUM_HARDWARE_LIMIT);
		return;
	}
	if (!uhd_setup_request(0,
			&req,
			(uint8_t*)&uhc_dev_enum->dev_desc,
			sizeof(usb_dev_desc_t),
			NULL,
			uhc_enumeration_step6)) {
		uhc_enumeration_error(UHC_ENUM_MEMORY_LIMIT);
		return;
	}
}

/**
 * \brief Device enumeration step 6
 * End of Get device descriptor request.
 * Wait 20ms in IDLE state.
 *
 * \param add           USB address of the setup request
 * \param status        Transfer status
 * \param payload_trans Number of data transfered during DATA phase
 */
static void uhc_enumeration_step6(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans)
{
	usb_printf("%s\n\r", __FUNCTION__);
	(void)(add);
	if ((status != UHD_TRANS_NOERROR) || (payload_trans < 8)
			|| (uhc_dev_enum->dev_desc.bDescriptorType != USB_DT_DEVICE)) {
		uhc_enumeration_error((status == UHD_TRANS_DISCONNECT)?
				UHC_ENUM_DISCONNECT:UHC_ENUM_FAIL);
		return;
	}
	// Wait 20ms
	uhc_enable_timeout_callback(20, uhc_enumeration_step7);
}

/**
 * \brief Device enumeration step 7
 * Reset USB line.
 */
static void uhc_enumeration_step7(void)
{
	usb_printf("%s\n\r", __FUNCTION__);
	uhc_enumeration_reset(uhc_enumeration_step8);
}

/**
 * \brief Device enumeration step 8
 * Lets USB line in IDLE state during 100ms.
 */
static void uhc_enumeration_step8(void)
{
	usb_printf("%s\n\r", __FUNCTION__);
	// Wait 100ms
	uhc_enable_timeout_callback(100, uhc_enumeration_step9);
}

/**
 * \brief Device enumeration step 9
 * Send a Set address setup request.
 */
static void uhc_enumeration_step9(void)
{
	usb_printf("%s\n\r", __FUNCTION__);
	usb_setup_req_t req;

	req.bmRequestType = USB_REQ_RECIP_DEVICE
			| USB_REQ_TYPE_STANDARD | USB_REQ_DIR_OUT;
	req.bRequest = USB_REQ_SET_ADDRESS;
#ifdef USB_HOST_HUB_SUPPORT
	uint8_t usb_addr_free = 1;
	uhc_device_t *dev;

	// Search free address
	dev = &g_uhc_device_root;
	while (dev) {
		usb_printf("is address %i in use by device %08x? ", usb_addr_free, dev);
		if (dev->address == usb_addr_free) {
			usb_printf("yes!\n\r");
			dev = &g_uhc_device_root;
			++usb_addr_free;
			continue;
		}
		if (dev->next != NULL) {
			usb_printf("no, but there are more devices connected\n\r");
			dev = dev->next;
			continue;
		}
		usb_printf("no!\n\r");
		break;
	}

	req.wValue = usb_addr_free;
	uhc_dev_enum->address = usb_addr_free;
#else
	req.wValue = UHC_DEVICE_ENUM_ADD;
	uhc_dev_enum->address = UHC_DEVICE_ENUM_ADD;
#endif
	req.wIndex = 0;
	req.wLength = 0;

	usb_printf("UHC_DEVICE_ENUM_ADD = %d\n\r", UHC_DEVICE_ENUM_ADD);

	// After a USB reset, the reallocation is required
	uhd_ep_free(0, 0);
	if (!uhd_ep0_alloc(0, uhc_dev_enum->dev_desc.bMaxPacketSize0)) {
		uhc_enumeration_error(UHC_ENUM_HARDWARE_LIMIT);
		return;
	}

	if (!uhd_setup_request(0,
			&req,
			(uint8_t*)&uhc_dev_enum->dev_desc,
			sizeof(usb_dev_desc_t),
			NULL,
			uhc_enumeration_step10)) {
		uhc_enumeration_error(UHC_ENUM_MEMORY_LIMIT);
		return;
	}
}

/**
 * \brief Device enumeration step 10
 * End of Set address request.
 * Lets USB line in IDLE state during 20ms.
 *
 * \param add           USB address of the setup request
 * \param status        Transfer status
 * \param payload_trans Number of data transfered during DATA phase
 */
static void uhc_enumeration_step10(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans)
{
	usb_printf("%s\n\r", __FUNCTION__);
	(void)(add);
	(void)(payload_trans);
	if (status != UHD_TRANS_NOERROR) {
		uhc_enumeration_error((status == UHD_TRANS_DISCONNECT) ?
				UHC_ENUM_DISCONNECT : UHC_ENUM_FAIL);
		return;
	}
	// Wait 20ms
	uhc_enable_timeout_callback(20, uhc_enumeration_step11);
}

/**
 * \brief Device enumeration step 11
 * Updates USB host pipe with the new USB address.
 * Requests a complete USB device descriptor.
 */
static void uhc_enumeration_step11(void)
{
	usb_printf("%s\n\r", __FUNCTION__);
	usb_setup_req_t req;

	// Free address 0 used to start enumeration
	uhd_ep_free(0, 0);

	// Alloc control endpoint with the new USB address
	if (!uhd_ep0_alloc(UHC_DEVICE_ENUM_ADD,
			uhc_dev_enum->dev_desc.bMaxPacketSize0)) {
		uhc_enumeration_error(UHC_ENUM_HARDWARE_LIMIT);
		return;
	}
	// Send USB device descriptor request
	req.bmRequestType = USB_REQ_RECIP_DEVICE|USB_REQ_TYPE_STANDARD|USB_REQ_DIR_IN;
	req.bRequest = USB_REQ_GET_DESCRIPTOR;
	req.wValue = (USB_DT_DEVICE << 8);
	req.wIndex = 0;
	req.wLength = sizeof(usb_dev_desc_t);
	if (!uhd_setup_request(UHC_DEVICE_ENUM_ADD,
			&req,
			(uint8_t *) & uhc_dev_enum->dev_desc,
			sizeof(usb_dev_desc_t),
			NULL, uhc_enumeration_step12)) {
		uhc_enumeration_error(UHC_ENUM_MEMORY_LIMIT);
		return;
	}
}

/**
 * \brief Device enumeration step 12
 * Requests the first USB structure of the USB configuration descriptor.
 *
 * \param add           USB address of the setup request
 * \param status        Transfer status
 * \param payload_trans Number of data transfered during DATA phase
 */
static void uhc_enumeration_step12(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans)
{
	usb_printf("%s\n\r", __FUNCTION__);
	usb_setup_req_t req;
	uint8_t conf_num;
	(void)(add);

	if ((status != UHD_TRANS_NOERROR) || (payload_trans != sizeof(usb_dev_desc_t))
			|| (uhc_dev_enum->dev_desc.bDescriptorType != USB_DT_DEVICE)) {
		uhc_enumeration_error((status==UHD_TRANS_DISCONNECT)?
				UHC_ENUM_DISCONNECT:UHC_ENUM_FAIL);
		return;
	}
	// Choose USB device configuration
	if (uhc_dev_enum->dev_desc.bNumConfigurations > 1) {
		conf_num = UHC_DEVICE_CONF(uhc_dev_enum);
	} else {
		conf_num = 1;
	}

	Assert(uhc_dev_enum->conf_desc == NULL);

	uhc_dev_enum->conf_desc = malloc(sizeof(usb_conf_desc_t));
	if (uhc_dev_enum->conf_desc == NULL) {
		Assert(false);
		uhc_enumeration_error(UHC_ENUM_MEMORY_LIMIT);
		return;
	}
	// Send USB device descriptor request
	req.bmRequestType = USB_REQ_RECIP_DEVICE|USB_REQ_TYPE_STANDARD|USB_REQ_DIR_IN;
	req.bRequest = USB_REQ_GET_DESCRIPTOR;
	req.wValue = (USB_DT_CONFIGURATION << 8) | (conf_num - 1);
	req.wIndex = 0;
	req.wLength = sizeof(usb_conf_desc_t);
	if (!uhd_setup_request(UHC_DEVICE_ENUM_ADD,
			&req,
			(uint8_t *) uhc_dev_enum->conf_desc,
			sizeof(usb_conf_desc_t),
			NULL, uhc_enumeration_step13)) {
		uhc_enumeration_error(UHC_ENUM_MEMORY_LIMIT);
		return;
	}
}

/**
 * \brief Device enumeration step 13
 * Requests a complete Get configuration descriptor.
 *
 * \param add           USB address of the setup request
 * \param status        Transfer status
 * \param payload_trans Number of data transfered during DATA phase
 */
static void uhc_enumeration_step13(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans)
{
	usb_printf("%s\n\r", __FUNCTION__);
	uint8_t conf_num;
	uint16_t conf_size;
	uint16_t bus_power = 0;
	usb_setup_req_t req;
	(void)(add);

	if ((status != UHD_TRANS_NOERROR) || (payload_trans != sizeof(usb_conf_desc_t))
			|| (uhc_dev_enum->conf_desc->bDescriptorType != USB_DT_CONFIGURATION)) {
		uhc_enumeration_error((status == UHD_TRANS_DISCONNECT)?
				UHC_ENUM_DISCONNECT:UHC_ENUM_FAIL);
		return;
	}

	// Copy usb_conf_desc_t members
	uhc_dev_enum->bmAttributes = uhc_dev_enum->conf_desc->bmAttributes;
	uhc_dev_enum->bMaxPower = uhc_dev_enum->conf_desc->bMaxPower;

#ifdef USB_HOST_HUB_SUPPORT
	uhc_device_t *dev;
	dev = uhc_dev_enum;
	while (dev) {
		print_uhc_dev(dev);
		if (dev->bmAttributes & USB_CONFIG_ATTR_SELF_POWERED) {
			usb_printf("The device or a parent HUB is SELF power, then no power on root\n\r");
			bus_power = 0;
			break;
		}
		if (dev == (&g_uhc_device_root)) {
			bus_power = dev->bMaxPower * 2;
			usb_printf("bus_power = %i\n\r", bus_power);
			break; // End of USB tree
		}
		// Go to USB HUB parent
		usb_printf("Go to USB HUB parent : %08x\n\r", dev->hub);
		dev = dev->hub;
	}
#else
	if (!(uhc_dev_enum->bmAttributes
			&USB_CONFIG_ATTR_SELF_POWERED)) {
		bus_power = uhc_dev_enum->bMaxPower * 2;
	}
#endif
	if ((bus_power + uhc_power_running) > USB_HOST_POWER_MAX) {
		usb_printf("uhc_power_running = %i\n\r", uhc_power_running);
		// USB interfaces consumption too high
		UHC_ENUM_EVENT(uhc_dev_enum, UHC_ENUM_OVERCURRENT);

		// Abort enumeration, set line in suspend mode
		uhc_enumeration_suspend();
		uhc_enum_try = 0;
		return;
	}
#ifdef USB_HOST_HUB_SUPPORT
	uhc_dev_enum->power = bus_power;
	uhc_power_running += bus_power;
	usb_printf("uhc_power_running = %i\n\r", uhc_power_running);
#endif

	// Save information about USB configuration descriptor size
	conf_size = le16_to_cpu(uhc_dev_enum->conf_desc->wTotalLength);
	conf_num = uhc_dev_enum->conf_desc->bConfigurationValue;
	Assert(conf_num);
	// Re alloc USB configuration descriptor
	free(uhc_dev_enum->conf_desc);
	uhc_dev_enum->conf_desc = malloc(conf_size);
	if (uhc_dev_enum->conf_desc == NULL) {
		Assert(false);
		uhc_enumeration_error(UHC_ENUM_MEMORY_LIMIT);
		return;
	}
	// Send USB device descriptor request
	req.bmRequestType =
			USB_REQ_RECIP_DEVICE | USB_REQ_TYPE_STANDARD |
			USB_REQ_DIR_IN;
	req.bRequest = USB_REQ_GET_DESCRIPTOR;
	req.wValue = (USB_DT_CONFIGURATION << 8) | (conf_num - 1);
	req.wIndex = 0;
	req.wLength = conf_size;
	if (!uhd_setup_request(UHC_DEVICE_ENUM_ADD,
			&req,
			(uint8_t *) uhc_dev_enum->conf_desc,
			conf_size,
			NULL, uhc_enumeration_step14)) {
		uhc_enumeration_error(UHC_ENUM_MEMORY_LIMIT);
		return;
	}
}

/**
 * \brief Device enumeration step 14
 * Enable USB configuration, if unless one USB interface is supported by UHIs.
 *
 * \param add           USB address of the setup request
 * \param status        Transfer status
 * \param payload_trans Number of data transfered during DATA phase
 */
static void uhc_enumeration_step14(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans)
{
	usb_printf("%s\n\r", __FUNCTION__);
	usb_setup_req_t req;
	bool b_conf_supported = false;
	(void)(add);

	if ((status != UHD_TRANS_NOERROR)
			|| (payload_trans < sizeof(usb_conf_desc_t))
			|| (uhc_dev_enum->conf_desc->bDescriptorType != USB_DT_CONFIGURATION)
			|| (payload_trans != le16_to_cpu(uhc_dev_enum->conf_desc->wTotalLength))) {
		uhc_enumeration_error((status==UHD_TRANS_DISCONNECT)?
				UHC_ENUM_DISCONNECT:UHC_ENUM_FAIL);
		return;
	}

	print_conf_desc(uhc_dev_enum->conf_desc, true);
	
	// Check if at least one USB interface is supported by UHIs
	for (uint8_t i = 0; i < UHC_NB_UHI /*&& !b_conf_supported*/; i++) {
		switch (uhc_uhis[i].install(uhc_dev_enum, uhc_dev_enum->conf_desc)) {
		case UHC_ENUM_SUCCESS:
			usb_printf("  -> %s => UHC_ENUM_SUCCESS\n\r", __FUNCTION__);
			b_conf_supported = true;
			break;

		case UHC_ENUM_UNSUPPORTED:
			usb_printf("  -> %s => UHC_ENUM_UNSUPPORTED\n\r", __FUNCTION__);
			break;

		default:
			usb_printf("  -> %s => failure..\n\r", __FUNCTION__);
			// USB host hardware limitation
			// Free all endpoints
			uhd_ep_free(UHC_DEVICE_ENUM_ADD,0xFF);
			UHC_ENUM_EVENT(uhc_dev_enum,UHC_ENUM_HARDWARE_LIMIT);

			// Abort enumeration, set line in suspend mode
			uhc_enumeration_suspend();
			uhc_enum_try = 0;
			return;
		}
	}
	if (!b_conf_supported) {
		usb_printf("  -> %s => NO DRIVER SUPPORT!\n\r", __FUNCTION__);

		// No USB interface supported
		UHC_ENUM_EVENT(uhc_dev_enum, UHC_ENUM_UNSUPPORTED);
	print_uhc_dev(uhc_dev_enum);

		// Abort enumeration, set line in suspend mode
		uhc_enumeration_suspend();
		uhc_enum_try = 0;
		return;
	}

	usb_printf("  -> %s => Enable device configuration\n\r", __FUNCTION__);

	// Enable device configuration
	req.bmRequestType = USB_REQ_RECIP_DEVICE
			| USB_REQ_TYPE_STANDARD | USB_REQ_DIR_OUT;
	req.bRequest = USB_REQ_SET_CONFIGURATION;
	req.wValue = uhc_dev_enum->conf_desc->bConfigurationValue;
	req.wIndex = 0;
	req.wLength = 0;
	if (!uhd_setup_request(UHC_DEVICE_ENUM_ADD,
			&req,
			NULL,
			0,
			NULL, uhc_enumeration_step15)) {
		uhc_enumeration_error(UHC_ENUM_MEMORY_LIMIT);
		return;
	}
}

/**
 * \brief Device enumeration step 15
 * Enables UHI interfaces
 *
 * \param add           USB address of the setup request
 * \param status        Transfer status
 * \param payload_trans Number of data transfered during DATA phase
 */
static void uhc_enumeration_step15(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans)
{
	usb_printf("%s\n\r", __FUNCTION__);
	(void)(add);
	if ((status!=UHD_TRANS_NOERROR) || (payload_trans!=0)) {
		for(uint8_t i = 0; i < UHC_NB_UHI; i++) {
			uhc_uhis[i].uninstall(uhc_dev_enum);
		}
		uhc_enumeration_error((status == UHD_TRANS_DISCONNECT)?
				UHC_ENUM_DISCONNECT : UHC_ENUM_FAIL);
		return;
	}

	// Enable all UHIs supported
	for (uint8_t i = 0; i < UHC_NB_UHI; i++) {
		uhc_uhis[i].enable(uhc_dev_enum);
	}

	// Free USB configuration descriptor buffer
	if (uhc_dev_enum->conf_desc != NULL) {
		free(uhc_dev_enum->conf_desc);
		uhc_dev_enum->conf_desc = NULL;
	}

#ifdef USB_HOST_LPM_SUPPORT
	// Check LPM support
	if (g_uhc_device_root.dev_desc.bcdUSB >= LE16(USB_V2_1)) {
		// Device can support LPM
		// Start LPM support check
		uhc_enumeration_step16_lpm();
		return;
	}
	uhc_dev_enum->lpm_desc = NULL;
#endif

	uhc_enum_try = 0;

	UHC_ENUM_EVENT(uhc_dev_enum, UHC_ENUM_SUCCESS);
	print_uhc_dev(uhc_dev_enum);
}

#ifdef USB_HOST_LPM_SUPPORT
/**
 * \brief Requests the USB structure of the USB BOS descriptor
 */
static void uhc_request_bos_desc(uint16_t length,
		const uhd_callback_setup_end_t callback)
{
	usb_setup_req_t req;
	uhc_dev_enum->lpm_desc = malloc(length);
	if (uhc_dev_enum->lpm_desc == NULL) {
		Assert(false);
		uhc_enumeration_error(UHC_ENUM_MEMORY_LIMIT);
		return;
	}
	// Send USB device descriptor request
	req.bmRequestType = USB_REQ_RECIP_DEVICE|USB_REQ_TYPE_STANDARD|USB_REQ_DIR_IN;
	req.bRequest = USB_REQ_GET_DESCRIPTOR;
	req.wValue = USB_DT_BOS << 8;
	req.wIndex = 0;
	req.wLength = length;
	if (!uhd_setup_request(UHC_DEVICE_ENUM_ADD,
			&req,
			(uint8_t *) uhc_dev_enum->lpm_desc,
			length,
			NULL, callback)) {
		uhc_enumeration_error(UHC_ENUM_MEMORY_LIMIT);
		return;
	}
}

/**
 * \brief Device enumeration step 16 (LPM only)
 * Requests first part of the USB BOS descriptor.
 */
static void uhc_enumeration_step16_lpm(void)
{
	uhc_request_bos_desc(sizeof(usb_dev_bos_desc_t),
			uhc_enumeration_step17_lpm);
}


/**
 * \brief Device enumeration step 17 (LPM only)
 * Check LPM support through the BOS descriptor received and request the whole BOS descriptor.
 *
 * \param add           USB address of the setup request
 * \param status        Transfer status
 * \param payload_trans Number of data transfered during DATA phase
 */
static void uhc_enumeration_step17_lpm(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans)
{
	(void)(add);

	if (status == UHD_TRANS_STALL) {
		// No BOS, LPM not supported
		free(uhc_dev_enum->lpm_desc);
		uhc_dev_enum->lpm_desc = NULL;
	} else if ((status != UHD_TRANS_NOERROR)
			|| (payload_trans != sizeof(usb_dev_bos_desc_t))
			|| (uhc_dev_enum->lpm_desc->bos.bDescriptorType != USB_DT_BOS)) {
		// Descriptor error
		uhc_enumeration_error((status==UHD_TRANS_DISCONNECT)?
				UHC_ENUM_DISCONNECT:UHC_ENUM_FAIL);
		return;
	} else if (status == UHD_TRANS_NOERROR) {
		uint16_t len = le16_to_cpu(uhc_dev_enum->lpm_desc->bos.wTotalLength);
		free(uhc_dev_enum->lpm_desc);
		uhc_dev_enum->lpm_desc = NULL;
		// LPM descriptor is possible
		if (len > sizeof(usb_dev_lpm_desc_t)) {
			// Request the descriptors
			uhc_request_bos_desc(len, uhc_enumeration_step18_lpm);
			return;
		}
	}
	// Enumeration OK
	uhc_enum_try = 0;
	UHC_ENUM_EVENT(uhc_dev_enum, UHC_ENUM_SUCCESS);
}

/**
 * \brief Device enumeration step 18 (LPM only)
 * Check LPM support through the BOS descriptor received
 *
 * \param add           USB address of the setup request
 * \param status        Transfer status
 * \param payload_trans Number of data transfered during DATA phase
 */
static void uhc_enumeration_step18_lpm(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans)
{
	(void)(add);

	if (status == UHD_TRANS_STALL) {
		// No BOS, LPM not supported
		free(uhc_dev_enum->lpm_desc);
		uhc_dev_enum->lpm_desc = NULL;
	} else if ((status != UHD_TRANS_NOERROR)
			|| (payload_trans < sizeof(usb_dev_lpm_desc_t))
			|| (uhc_dev_enum->lpm_desc->bos.bDescriptorType != USB_DT_BOS)
			|| (payload_trans != le16_to_cpu(uhc_dev_enum->lpm_desc->bos.wTotalLength))) {
		uhc_enumeration_error((status==UHD_TRANS_DISCONNECT)?
				UHC_ENUM_DISCONNECT:UHC_ENUM_FAIL);
		return;
	} else if (status == UHD_TRANS_NOERROR) {
		// Check LPM support
		uint8_t *desc = (uint8_t *)uhc_dev_enum->lpm_desc;
		usb_dev_capa_ext_desc_t *ext_desc;
		uint8_t i;
		for (i = 0; i < uhc_dev_enum->lpm_desc->bos.bNumDeviceCaps; i ++) {
			desc += desc[0];
			ext_desc = (usb_dev_capa_ext_desc_t *)desc;
			if (ext_desc->bDescriptorType == USB_DT_DEVICE_CAPABILITY
					&& ext_desc->bDevCapabilityType == USB_DC_USB20_EXTENSION
					&& (ext_desc->bmAttributes & USB_DC_EXT_LPM)) {
				// LPM supported
				break;
			}
		}
		if (i >= uhc_dev_enum->lpm_desc->bos.bNumDeviceCaps) {
			// LPM not supported
			free(uhc_dev_enum->lpm_desc);
			uhc_dev_enum->lpm_desc = NULL;
		}
	}

	uhc_enum_try = 0;
	UHC_ENUM_EVENT(uhc_dev_enum, UHC_ENUM_SUCCESS);
}
#endif // USB_HOST_LPM_SUPPORT

/**
 * \brief Manage error during device enumeration
 *
 * \param status        Enumeration error occurred
 */
static void uhc_enumeration_error(uhc_enum_status_t status)
{
	if (status == UHC_ENUM_DISCONNECT) {
		uhc_enum_try = 0;
		return; // Abort enumeration process
	}
	uhd_ep_free(uhc_dev_enum->address, 0xFF);

	// Free USB configuration descriptor buffer
	if (uhc_dev_enum->conf_desc != NULL) {
		free(uhc_dev_enum->conf_desc);
		uhc_dev_enum->conf_desc = NULL;
	}
	uhc_dev_enum->address = 0;
	if (uhc_enum_try++ < UHC_ENUM_NB_TRY) {
		// Restart enumeration at beginning
		uhc_enumeration_step1();
		return;
	}
	// Abort enumeration, set line in suspend mode
	uhc_enumeration_suspend();
	UHC_ENUM_EVENT(uhc_dev_enum, status);
	uhc_enum_try = 0;
}

/**
 * \brief Enables or disables the remote wakeup feature
 * of all devices connected
 *
 * \param b_enable   true to enable remote wakeup feature, else disable.
 */
static void uhc_remotewakeup(bool b_enable)
{
	usb_setup_req_t req;
	uhc_device_t *dev;

	dev = &g_uhc_device_root;
	while(1) {
		if (dev->bmAttributes & USB_CONFIG_ATTR_REMOTE_WAKEUP) {
			if (b_enable) {
				req.bRequest = USB_REQ_SET_FEATURE;
			} else {
				req.bRequest = USB_REQ_CLEAR_FEATURE;
			}
			req.bmRequestType = USB_REQ_RECIP_DEVICE
					|USB_REQ_TYPE_STANDARD|USB_REQ_DIR_OUT;
			req.wValue = USB_DEV_FEATURE_REMOTE_WAKEUP;
			req.wIndex = 0;
			req.wLength = 0;
			uhd_setup_request(dev->address,&req,NULL,0,NULL,NULL);
		}
#ifdef USB_HOST_HUB_SUPPORT
		if (dev->next == NULL) {
			break;
		}
		dev = dev->next;
#else
		break;
#endif
	}
}

/**
 * \brief Callback used to signal the end of a setup request
 *
 * \param add           USB address of the setup request
 * \param status        Transfer status
 * \param payload_trans Number of data transfered during DATA phase
 */
static void uhc_setup_request_callback(
		usb_add_t add,
		uhd_trans_status_t status,
		uint16_t payload_trans)
{
	(void)(add);
	(void)(payload_trans);
	uhc_setup_request_finish_status = (status == UHD_TRANS_NOERROR);
	uhc_setup_request_finish = true;
}

static uhc_device_t* allocate_device()
{
	usb_printf("%s : %08x\n\r", __FUNCTION__, g_devices_in_use);
	for (uint32_t bit = 0; bit < USB_MAX_NUM_DEVICES; ++bit) {
		uint32_t mask = (1 << bit);
		if ((g_devices_in_use & mask) == 0) {
			g_devices_in_use |= mask;
			uhc_device_t* dev = &g_devices[bit];
			memset(dev, 0x00, sizeof(uhc_device_t));
			dev->address = UHC_USB_ADD_NOT_VALID;
			return dev;
		}
	}
	Assert(false && "no free devices!");
	return NULL;
}

static void free_device(uhc_device_t* device)
{
	usb_printf("%s : %08x in %08x\n\r", __FUNCTION__, device, g_devices_in_use);
	for (uint32_t bit = 0; bit < USB_MAX_NUM_DEVICES; ++bit) {
		uint32_t mask = (1 << bit);
		if (&g_devices[bit] == device) {
			Assert((g_devices_in_use & mask) != 0);
			g_devices_in_use &= ~mask;
			return;
		}
	}
	Assert(false && "device not found!");
}

/**
 * \name Callbacks used by USB Host Driver (UHD) to notify events
 * @{
 */
void uhc_notify_connection(bool b_plug)
{
	usb_printf("%s : %s\n\r", __FUNCTION__, b_plug ? "connect" : "disconnect");
	if (b_plug) {
		Assert(g_uhc_device_root.address == UHC_USB_ADD_NOT_VALID);
		memset(&g_uhc_device_root, 0x00, sizeof(g_uhc_device_root));
		g_uhc_device_root.address = UHC_USB_ADD_NOT_VALID;
	} else {
		if (g_uhc_device_root.address == UHC_USB_ADD_NOT_VALID) {
			// Already disconnected
			// Ignore the noise from host stop process
			return;
		}

		// the root device is gone - make sure everything is disconnected
		for (uint32_t bit = 0; bit < USB_MAX_NUM_DEVICES; ++bit) {
			uint32_t mask = (1 << bit);
			if ((g_devices_in_use & mask) != 0) {
				uhc_device_t* dev = &g_devices[bit];
				free_device(dev);
				uhc_connection_tree(false, dev);
			}
		}
	}
	// Device connection on root
	uhc_connection_tree(b_plug, &g_uhc_device_root);
}

void uhc_notify_connection_hub(bool b_plug, uhc_device_t* hub, uint8_t port, uhd_speed_t speed)
{
	usb_printf("%s : %s hub=%08x port = %i speed = %i\n\r", __FUNCTION__, b_plug ? "connect" : "disconnect", hub, port, speed);
	print_uhc_dev(hub);


	Assert(g_uhc_device_root.address != UHC_USB_ADD_NOT_VALID);
	uhc_device_t* dev = NULL;

	usb_printf("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% devices\n\r");
	uhc_device_t* prev = NULL;
	for(dev = &g_uhc_device_root; dev; dev = dev->next) {
		Assert(prev == dev->prev);
		print_uhc_dev(dev);
		prev = dev;
	}
	usb_printf("%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% ^^^^^^^\n\r");


	if (b_plug) {
		dev = allocate_device();
		dev->hub = hub;
		dev->hub_port = port;

		dev->next = hub->next;
		dev->prev = hub;
		if (hub->next)
			hub->next->prev = dev;
		hub->next = dev;
	} else {
		for (dev = hub->next; dev; dev = dev->next)
			if (dev->hub_port == port){
				print_uhc_dev(dev);
				break;
			}
	}

	if(dev == NULL) {
		usb_printf("spurious hub disconnect -ignoring\n\r");
		return;
	}

	if (b_plug) {
		dev->speed = speed;
	} else {

		// if this device is a hub - remove all devices connected to it
		for (uint32_t bit = 0; bit < USB_MAX_NUM_DEVICES; ++bit) {
			uint32_t mask = (1 << bit);
			if ((g_devices_in_use & mask) == 0) {
				continue;
			}

			// trace all the way up to the root
			for (uhc_device_t* child = &g_devices[bit]; child; child = child->hub){
				if (child->hub == dev) {
					free_device(child);
					uhc_connection_tree(false, child);
				}
			}
		}

		free_device(dev);
	}

	uhc_connection_tree(b_plug, dev);
}

bool uhc_is_enumerating()
{
	return uhc_enum_try != 0;
}

void uhc_notify_sof(bool b_micro)
{
	// Call all UHIs
	for (uint8_t i = 0; i < UHC_NB_UHI; i++) {
		if (uhc_uhis[i].sof_notify != NULL) {
			uhc_uhis[i].sof_notify(b_micro);
		}
	}

	if (!b_micro) {
		// Manage SOF timeout
		if (uhc_sof_timeout) {
			if (--uhc_sof_timeout == 0) {
				uhc_sof_timeout_callback();
			}
		}
	}
}

void uhc_notify_resume(void)
{
	uhc_remotewakeup(false);
	UHC_WAKEUP_EVENT();
}

#ifdef USB_HOST_LPM_SUPPORT
void uhc_notify_resume_lpm(void)
{
	UHC_WAKEUP_EVENT();
}
#endif

//! @}


/**
 * \name Functions to control the USB host stack
 *
 * @{
 */
void uhc_start(void)
{
	kprintf("%s\n\r", __FUNCTION__);

	memset(&g_uhc_device_root, 0x00, sizeof(g_uhc_device_root));
	g_uhc_device_root.address = UHC_USB_ADD_NOT_VALID;
	uhc_sof_timeout = 0; // No callback registered on a SOF timeout
	asf_uhd_enable();
}

void uhc_stop(bool b_id_stop)
{
	// Stop UHD
	asf_uhd_disable(b_id_stop);
}

void uhc_suspend(bool b_remotewakeup)
{
	if (uhc_enum_try) {
		// enumeration on-going, the USB suspend can't be done
		return;
	}

	if (b_remotewakeup) {
		uhc_remotewakeup(true);
	}
	// Suspend all USB devices
	uhd_suspend();
}

bool uhc_is_suspend(void)
{
	if (g_uhc_device_root.address == UHC_USB_ADD_NOT_VALID) {
		return true;
	}
	return uhd_is_suspend();
}

void uhc_resume(void)
{
	if (!uhc_is_suspend()) {
		return;
	}
	// Resume all USB devices
	uhd_resume();
}

#ifdef USB_HOST_LPM_SUPPORT
bool uhc_suspend_lpm(bool b_remotewakeup, uint8_t hird)
{
	if (uhc_enum_try) {
		// enumeration on-going, the USB suspend can't be done
		return false;
	}

	// Check LPM support
	if (uhc_dev_enum->lpm_desc == NULL) {
		// Device cannot support LPM
		return false;
	}

	// Suspend all USB devices
	return uhd_suspend_lpm(b_remotewakeup, hird);
}
#endif // USB_HOST_LPM_SUPPORT

//! @}


/**
 * \name User functions to manage the devices
 *
 * @{
 */
uint8_t uhc_get_device_number(void)
{
#ifdef USB_HOST_HUB_SUPPORT
	uint8_t nb_dev = 0;
	uhc_device_t *dev;

	if (g_uhc_device_root.address != UHC_USB_ADD_NOT_VALID) {
		dev = &g_uhc_device_root;
		while (nb_dev++) {
			if (dev->next == NULL) {
				break;
			}
			dev = dev->next;
		}
	}
	return nb_dev;
#else
	return (g_uhc_device_root.address != UHC_USB_ADD_NOT_VALID) ? 1 : 0;
#endif
}

char *uhc_dev_get_string_manufacturer(uhc_device_t * dev)
{
	if (!dev->dev_desc.iManufacturer) {
		return NULL; // No manufacturer string available
	}
	return uhc_dev_get_string(dev, dev->dev_desc.iManufacturer);
}

char *uhc_dev_get_string_product(uhc_device_t * dev)
{
	if (!dev->dev_desc.iProduct) {
		return NULL; // No product string available
	}
	return uhc_dev_get_string(dev, dev->dev_desc.iProduct);
}

char *uhc_dev_get_string_serial(uhc_device_t * dev)
{
	if (!dev->dev_desc.iSerialNumber) {
		return NULL; // No serial string available
	}
	return uhc_dev_get_string(dev, dev->dev_desc.iSerialNumber);
}

char *uhc_dev_get_string(uhc_device_t * dev, uint8_t str_id)
{
	usb_setup_req_t req;
	usb_str_desc_t str_header;
	usb_str_lgid_desc_t *str_desc;
	char *string;
	uint8_t i;
	(void)(dev);

	uhd_ep_free(dev->address, 0);
	if (!uhd_ep0_alloc(dev->address,
			dev->dev_desc.bMaxPacketSize0)) {
		return NULL;
	}

	req.bmRequestType = USB_REQ_RECIP_DEVICE|USB_REQ_TYPE_STANDARD|USB_REQ_DIR_IN;
	req.bRequest = USB_REQ_GET_DESCRIPTOR;
	req.wValue = (USB_DT_STRING << 8) | str_id;
	req.wIndex = 0;
	req.wLength = sizeof(usb_str_desc_t);

	// Get the size of string
	uhc_setup_request_finish = false;
	if (!uhd_setup_request(dev->address,
			&req,
			(uint8_t*)&str_header,
			sizeof(usb_str_desc_t),
			NULL,
			uhc_setup_request_callback)) {
		return NULL;
	}
	while (!uhc_setup_request_finish);
	if (!uhc_setup_request_finish_status) {
		return NULL;
	}
	// Get the size of string
	str_desc = malloc(str_header.bLength);
	if (str_desc == NULL) {
		return NULL;
	}
	req.wLength = str_header.bLength;
	uhc_setup_request_finish = false;
	if (!uhd_setup_request(dev->address,
			&req,
			(uint8_t*)str_desc,
			str_header.bLength,
			NULL,
			uhc_setup_request_callback)) {
		return NULL;
	}
	while (!uhc_setup_request_finish);
	if (!uhc_setup_request_finish_status) {
		free(str_desc);
		return NULL;
	}
	// The USB strings are "always" in ASCII format, then translate it.
	str_header.bLength = (str_header.bLength - 2) / 2; // Number of character
	string = malloc(str_header.bLength + 1); // +1 for NULL terminal
	if (string == NULL) {
		free(str_desc);
		return NULL;
	}
	for (i = 0; i < str_header.bLength; i++) {
		string[i] = le16_to_cpu(str_desc->string[i]) & 0xFF;
	}
	string[i] = 0;
	free(str_desc);

	return string;
}

uint16_t uhc_dev_get_power(uhc_device_t* dev)
{
	return dev->bMaxPower * 2;
}

uhd_speed_t uhc_dev_get_speed(uhc_device_t * dev)
{
	return dev->speed;
}

bool uhc_dev_is_high_speed_support(uhc_device_t* dev)
{
	usb_setup_req_t req;
	usb_dev_qual_desc_t qualifier;

	if (dev->speed == UHD_SPEED_HIGH) {
		return true;
	}
	if (dev->speed == UHD_SPEED_FULL) {
		req.bmRequestType = USB_REQ_RECIP_DEVICE
				| USB_REQ_TYPE_STANDARD | USB_REQ_DIR_IN;
		req.bRequest = USB_REQ_GET_DESCRIPTOR;
		req.wValue = (USB_DT_DEVICE_QUALIFIER << 8);
		req.wIndex = 0;
		req.wLength = sizeof(qualifier);

		// Get the size of string
		uhc_setup_request_finish = false;
		if (!uhd_setup_request(0,
				&req,
				(uint8_t*)&qualifier,
				sizeof(qualifier),
				NULL,
				uhc_setup_request_callback)) {
			return NULL;
		}
		while (!uhc_setup_request_finish);
		return uhc_setup_request_finish_status;
	}
	return false; // Low speed device
}

void uhc_dev_reset(uhc_device_t *dev)
{
	uint32_t i;

	uhc_enum_try = 1;
#ifdef USB_HOST_HUB_SUPPORT
	uhc_dev_enum = dev;
#endif
	for (i = 0; i < UHC_NB_UHI; i++) {
		uhc_uhis[i].uninstall(uhc_dev_enum);
	}
	if (uhc_dev_enum->conf_desc) {
		free(uhc_dev_enum->conf_desc);
		uhc_dev_enum->conf_desc = NULL;
	}
	uhc_enumeration_step3();
}

//! @}

//! @}
