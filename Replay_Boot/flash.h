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
#ifndef FLASH_H_INCLUDED
#define FLASH_H_INCLUDED

#include "config.h"

uint8_t FLASH_VerifySRecords(const char* filename, uint32_t* crc_file, uint32_t* crc_flash);
uint8_t FLASH_RebootAndFlash(const char* filename);

#endif // FLASH_H_INCLUDED
