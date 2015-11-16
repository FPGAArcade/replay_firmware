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
#ifndef TWI_H_INCLUDED
#define TWI_H_INCLUDED
#include "board.h"

#define CLOCKCONFIG_PD          (0)
#define CLOCKCONFIG_PLL3_BYPASS (1)
#define CLOCKCONFIG_PLL3_VFO    (0)
#define CLOCKCONFIG_PLL2_BYPASS (1)
#define CLOCKCONFIG_PLL2_VFO    (0)
#define CLOCKCONFIG_PLL1_BYPASS (1)
#define CLOCKCONFIG_PLL1_VFO    (0)

typedef struct
{
  uint16_t pll1_m;
  uint16_t pll1_n;
  uint16_t pll2_m;
  uint16_t pll2_n;
  uint16_t pll3_m;
  uint16_t pll3_n;

  uint8_t p_sel[6];
  uint8_t p_div[6];
  uint8_t y_sel[6];
} clockconfig_t;

typedef struct
{
  uint8_t reg1C;
  uint8_t reg1D;
  uint8_t reg1E;
  uint8_t reg1F;
  uint8_t reg20;
  uint8_t reg21;
  uint8_t reg23;
  uint8_t reg31;
  uint8_t reg33;
  uint8_t reg34;
  uint8_t reg35;
  uint8_t reg36;
  uint8_t reg37;
  uint8_t reg48;
  uint8_t reg49;
  uint8_t reg56;
 } vidconfig_t;

void TWI_Wait_Tx(void);
void TWI_Wait_Rx(void);
void TWI_Wait_Cmpl(void);

uint8_t Read_THS7353(uint8_t Address);
void    Write_THS7353(uint8_t Address, uint8_t Data);
uint8_t Read_CDCE906(uint8_t Address);
void    Write_CDCE906(uint8_t Address, uint8_t Data);

void Configure_VidBuf(uint8_t chan, uint8_t stc, uint8_t lpf, uint8_t mode);
void Configure_ClockGen(const clockconfig_t *config);

void Write_CH7301(uint8_t address, uint8_t data);
uint8_t Read_CH7301(uint8_t address);
void Configure_CH7301(const vidconfig_t *config);
#endif
