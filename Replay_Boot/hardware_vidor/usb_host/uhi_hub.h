// ADD REPLAY LICENSE!

#pragma once

#include "conf_usb_host.h"
#include "usb_protocol.h"
#include "usb_protocol_hub.h"
#include "uhi.h"

#define UHI_HUB { \
	.install = uhi_hub_install, \
	.enable = uhi_hub_enable, \
	.uninstall = uhi_hub_uninstall, \
	.sof_notify = uhi_hub_update \
}

extern uhc_enum_status_t uhi_hub_install(uhc_device_t* dev, usb_conf_desc_t* conf_desc);
extern void uhi_hub_enable(uhc_device_t* dev);
extern void uhi_hub_uninstall(uhc_device_t* dev);
extern void uhi_hub_update(bool b_micro);

extern void uhi_hub_suspend(uhc_device_t* dev);
extern void uhi_hub_send_reset(uhc_device_t* dev, uhd_callback_reset_t callback);
