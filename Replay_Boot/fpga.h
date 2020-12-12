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

#ifndef FPGA_H_INCLUDED
#define FPGA_H_INCLUDED

#include "board.h"
#include "fullfat.h"
#include "config.h"


#define kDRAM_PHASE 0x4A
//#define kDRAM_PHASE 0x5E
#define kDRAM_SEL   0x02

uint8_t FPGA_Default(void);
uint8_t FPGA_Config(FF_FILE* pFile);

uint8_t FPGA_DramTrain(void);
uint8_t FPGA_DramEye(uint8_t mode);
uint8_t FPGA_ProdTest(void);
void    FPGA_ClockMon(status_t* current_status);

void    FPGA_ExecMem(uint32_t base, uint16_t len, uint32_t checksum);
void	FPGA_DecompressToDRAM(char* buffer, uint32_t size, uint32_t base);
void	FPGA_WriteEmbeddedToFile(FF_FILE* file);

typedef size_t (*inflate_read_func_ptr)(void* buffer, size_t len, void* context);
typedef size_t (*inflate_write_func_ptr)(const void* buffer, size_t len, void* context);
size_t zlib_inflate(inflate_read_func_ptr read_func, void* const read_context, const size_t read_buffer_size, inflate_write_func_ptr write_func, void* const write_context, int flags);
size_t gunzip(inflate_read_func_ptr read_func, void* const read_context, inflate_write_func_ptr write_func, void* const write_context);

#endif
