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
#ifndef USB_H_INCLUDED
#define USB_H_INCLUDED

#include "config.h"
#include "usb/msc.h"

void USB_MountMassStorage(status_t* current_status);
void USB_UnmountMassStorage(status_t* current_status);
void USB_Update(status_t* current_status);

#endif // USB_H_INCLUDED