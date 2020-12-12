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

#ifndef FILEIO_DRV_H_INCLUDED
#define FILEIO_DRV_H_INCLUDED

#include "fileio.h"
#include "board.h"
#include "fullfat.h"

void FileIO_Drv08_Process(uint8_t ch, fch_t handle[2][FCH_MAX_NUM], uint8_t status); // ata
void FileIO_Drv01_Process(uint8_t ch, fch_t handle[2][FCH_MAX_NUM], uint8_t status); // floppy
void FileIO_Drv02_Process(uint8_t ch, fch_t handle[2][FCH_MAX_NUM], uint8_t status); // tape
void FileIO_Drv00_Process(uint8_t ch, fch_t handle[2][FCH_MAX_NUM], uint8_t status); // generic

uint8_t FileIO_Drv08_InsertInit(uint8_t ch, uint8_t drive_number, fch_t* drive, char* ext);
uint8_t FileIO_Drv02_InsertInit(uint8_t ch, uint8_t drive_number, fch_t* drive, char* ext);
uint8_t FileIO_Drv01_InsertInit(uint8_t ch, uint8_t drive_number, fch_t* drive, char* ext);
uint8_t FileIO_Drv00_InsertInit(uint8_t ch, uint8_t drive_number, fch_t* drive, char* ext);


#endif
