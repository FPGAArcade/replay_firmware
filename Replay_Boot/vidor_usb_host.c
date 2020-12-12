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

#if defined(ARDUINO_SAMD_MKRVIDOR4000)

#include "hardware_vidor/usb_host/uhc.c"
#include "hardware_vidor/usb_host/uhi_hid.c"
#include "hardware_vidor/usb_host/uhi_hid_keyboard.c"
#include "hardware_vidor/usb_host/uhi_hid_mouse.c"
#include "hardware_vidor/usb_host/uhi_hub.c"
#include "hardware_vidor/usb_host/usb.c"
#include "hardware_vidor/usb_host/usb_debug.c"
#include "hardware_vidor/usb_host/usb_host_uhd.c"

#endif
