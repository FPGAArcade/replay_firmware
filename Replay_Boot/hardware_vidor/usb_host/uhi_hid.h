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

typedef struct {
	int8_t	usage;	// negative == USAGE_PAGE, positive = USAGE, zero = PAD
	uint8_t	size;	// number of bits
} uhi_hid_report_object_t;

typedef struct {
	uint8_t	report_id;	// 0xff when not used
	uint8_t	num_objects;
	uhi_hid_report_object_t objects[16];
} uhi_hid_report_data_t;

extern uint8_t uhi_hid_report_buffer[512];

uint8_t parse_hid_report(uint8_t* hid_report, uint16_t hid_report_size, uhi_hid_report_data_t* report_data);
uint32_t extract_report_object(const uint8_t* buffer, uint32_t bit_offset, uint32_t bit_size);
