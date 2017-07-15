/*
 WWW.FPGAArcade.COM

 REPLAY Retro Gaming Platform
 No Emulation No Compromise

 All rights reserved
 Mike Johnson Wolfgang Scherr

 SVN: $Id:

--------------------------------------------------------------------

 Redistribution and use in source and synthezised forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

 Redistributions in synthesized form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.

 Neither the name of the author nor the names of other contributors may
 be used to endorse or promote products derived from this software without
 specific prior written permission.

 THIS CODE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.

 You are responsible for any legal issues arising from your use of this code.

 The latest version of this file can be found at: www.FPGAArcade.com

 Email support@fpgaarcade.com
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

typedef size_t (*inflate_read_func_ptr)(void* buffer, size_t len, void* context);
typedef size_t (*inflate_write_func_ptr)(const void* buffer, size_t len, void* context);
size_t zlib_inflate(inflate_read_func_ptr read_func, void* const read_context, const size_t read_buffer_size, inflate_write_func_ptr write_func, void* const write_context, int flags);
size_t gunzip(inflate_read_func_ptr read_func, void* const read_context, inflate_write_func_ptr write_func, void* const write_context);

#endif
