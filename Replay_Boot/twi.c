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


#include "twi.h"
#include "hardware/twi.h"
#include "messaging.h"

#define TWI_TIMEOUTMAX 50000

void TWI_Wait_Tx(void)
{
    uint32_t timeout = 0;

    while ( !TWI_ByteSent() && (++timeout < TWI_TIMEOUTMAX) );

    if (timeout == TWI_TIMEOUTMAX) {
        ERROR("TWI:Tx timeout.");
    }
}

void TWI_Wait_Rx(void)
{
    uint32_t timeout = 0;

    while ( !TWI_ByteReceived() && (++timeout < TWI_TIMEOUTMAX) );

    if (timeout == TWI_TIMEOUTMAX) {
        ERROR("TWI:Rx timeout.");
    }
}

void TWI_Wait_Cmpl(void)
{
    uint32_t timeout = 0;

    while ( !TWI_TransferComplete() && (++timeout < TWI_TIMEOUTMAX) );

    if (timeout == TWI_TIMEOUTMAX) {
        ERROR("TWI:Completion timeout.");    // note this is debug only, no error handler
    }
}

//
uint8_t Read_THS7353(uint8_t address)
{
    DEBUG(2, "TWI:THS7353 Read");
    uint8_t  readData;
    // device address is 0101100 = x2C
    // assume idle on entry
    // phase1
    TWI_StartWrite(0x2C, 0, 0, address);
    TWI_Stop();  // Set Stop
    TWI_Wait_Tx();
    TWI_Wait_Cmpl();
    // phase2
    TWI_StartRead(0x2C, 0, 0);
    TWI_Stop(); // set stop bit

    TWI_Wait_Rx();
    readData = TWI_ReadByte();
    TWI_Wait_Cmpl();

    return readData;
}

void Write_THS7353(uint8_t address, uint8_t data)
{
    DEBUG(2, "TWI:THS7353 Write");
    TWI_StartWrite(0x2C, 1, address, data);
    TWI_Wait_Tx();

    TWI_Stop();  // Set Stop
    TWI_Wait_Tx();
    TWI_Wait_Cmpl();
}

uint8_t Read_CDCE906(uint8_t address)
{
    DEBUG(2, "TWI:CDCE906 Read");

    // device address is 1101001 = x69
    // comand = 128 + address
    uint8_t readData;
    uint8_t command = 0x80 | (address & 0x7F);

    TWI_StartWrite(0x69, 0, 0, command);

    TWI_Wait_Tx();
    TWI_Wait_Cmpl();

    TWI_StartRead(0x69, 0, 0);
    TWI_Stop(); // set stop bit

    TWI_Wait_Rx();
    readData = TWI_ReadByte();
    TWI_Wait_Cmpl();
    return readData;
}

void Write_CDCE906(uint8_t address, uint8_t data)
{
    DEBUG(2, "TWI:CDCE906 Write");

    uint8_t command = 0x80 | (address & 0x7F);

    TWI_StartWrite(0x69, 0, 0, command);
    TWI_Wait_Tx();

    TWI_WriteByte(data);
    TWI_Stop();  // Set Stop
    TWI_Wait_Tx();
    TWI_Wait_Cmpl();
}
//
//
//
void Configure_VidBuf(uint8_t chan, uint8_t stc, uint8_t lpf, uint8_t mode)
{
    // 7..6 AC-STC filter (0=9Mhz, 1 = 15MHz, 2 = 35MHz, 3 = 35MHz)
    // 5    chan select, 1 = chan B (not used)
    // 4..3 LPF (0=9MHz SD PAL/NSTC, 1 = 16MHz, 480/576P, 2 = 35MHz 720P, 3= bypass 1080P)
    // 2..0 7,6,5 STC with high,mid,low bias
    //      4     AC bias
    //      3     DC biac + 250mV
    //      2     DC biac
    //      1     Mute
    //      0     Off
    DEBUG(2, "AC-STC: ");

    switch (stc) {
        case 0:
            DEBUG(2, "9MHz");
            break;

        case 1:
            DEBUG(2, "15MHz");
            break;

        case 2:
            DEBUG(2, "35MHz");
            break;

        case 3:
            DEBUG(2, "35MHz");
            break;

        default:
            DEBUG(2, "unknown");
            break;
    }

    DEBUG(2, "LPF: ");

    switch (lpf) {
        case 0:
            DEBUG(2, "9MHz");
            break;

        case 1:
            DEBUG(2, "16MHz");
            break;

        case 2:
            DEBUG(2, "35MHz");
            break;

        case 3:
            DEBUG(2, "bypass");
            break;

        default:
            DEBUG(2, "unknown");
            break;
    }

    DEBUG(2, "BIAS: ");

    switch (mode) {
        case 0:
            DEBUG(2, "off");
            break;

        case 1:
            DEBUG(2, "mute");
            break;

        case 2:
            DEBUG(2, "DC");
            break;

        case 3:
            DEBUG(2, "DC+250mV");
            break;

        case 4:
            DEBUG(2, "AC");
            break;

        default:
            DEBUG(2, "unknown");
            break;
    }

    uint8_t command = ((stc & 0x03) << 6 ) | ((lpf & 0x03) << 3) | (mode & 0x07);


    Write_THS7353(chan, command);

    if (Read_THS7353(chan) != command) {
        ERROR("TWI:THS7353 chan:%d Config failure.", chan);

    } else {
        DEBUG(2, "TWI:THS7353 chan:%d Config OK.", chan);
    }
}
//
//

void Configure_ClockGen(const clockconfig_t* config)
{
    uint32_t addr;
    uint8_t  ok = TRUE;
    uint8_t  mem[24];
    // repack from lightly packed ClockConfig struct into registers
    // see datasheet for register details
    //  register -1
    uint8_t  powerdown = 0;  // 0:powerdown
    uint8_t  pll_conf[3] = {0, 0, 0}; // 1:PLL1_bypass 0:PLL1_VFO
    uint8_t  y_sel[6]    = {0, 1, 2, 3, 4, 5}; // static divider selection

    // calculate VFO
    uint32_t vco_freq[3];
    uint32_t op_freq;

    for (addr = 0; addr < 3; addr++) {
        switch (addr) {
            case 0  :
                vco_freq[addr] = (27000 * config->pll1_n) / config->pll1_m;
                break;

            case 1  :
                vco_freq[addr] = (27000 * config->pll2_n) / config->pll2_m;
                break;

            case 2  :
                vco_freq[addr] = (27000 * config->pll3_n) / config->pll3_m;
                break;

            default :
                vco_freq[addr] = 0;
        }

        DEBUG(2, "PLL%d VCO Freq ~ %6d KHz", addr + 1, vco_freq[addr]);

        if (vco_freq[addr] > 190000) {
            pll_conf[addr] |= 1;
        }

        if (vco_freq[addr] > 300000) {
            ERROR(" !! OUT OF RANGE !!");
        }

        if (vco_freq[addr] <  80000) {
            ERROR(" !! OUT OF RANGE !!");
        }
    }

    // px sel (divider source selection)
    // px div (divider) 0-127 divider value
    // yx sel (0= divider0 .. 5= divider5) 0x1x = on
    DEBUG(0, "PLL clock outputs :");

    for (addr = 0; addr < 6; addr++) {
        switch (config->p_sel[addr]) {
            case 0  :
                op_freq = 27000;
                break;

            case 1  :
                op_freq = vco_freq[0];
                break;

            case 2  :
                op_freq = vco_freq[1];
                break;

            case 4  :
                op_freq = vco_freq[2];
                break;

            default :
                op_freq = 0;
        }

        op_freq = op_freq / config->p_div[addr];

        if (config->y_sel[addr] == 0) {
            DEBUG(0, " %d :      OFF", addr);

        } else {
            DEBUG(0, " %d : ~ %6d KHz", addr, op_freq);
        }
    }

    DEBUG(3, "PLL config %d %d %d", pll_conf[0], pll_conf[1], pll_conf[2]);

    mem[ 1 - 1] = (EBITS(config->pll1_m, 0, 8));
    mem[ 2 - 1] = (EBITS(config->pll1_n, 0, 8));
    mem[ 3 - 1] = (EBITS(pll_conf[0], CLOCKCONFIG_PLL1_BYPASS, 1) << 7) |
                  (EBITS(pll_conf[1], CLOCKCONFIG_PLL2_BYPASS, 1) << 6) |
                  (EBITS(pll_conf[2], CLOCKCONFIG_PLL3_BYPASS, 1) << 5) |
                  (EBITS(config->pll1_n, 8, 4) << 1) |
                  (EBITS(config->pll1_m, 8, 1));
    mem[ 4 - 1] = (EBITS(config->pll2_m, 0, 8));
    mem[ 5 - 1] = (EBITS(config->pll2_n, 0, 8));
    mem[ 6 - 1] = (EBITS(pll_conf[0], CLOCKCONFIG_PLL1_VFO, 1) << 7) |
                  (EBITS(pll_conf[1], CLOCKCONFIG_PLL2_VFO, 1) << 6) |
                  (EBITS(pll_conf[2], CLOCKCONFIG_PLL3_VFO, 1) << 5) |
                  (EBITS(config->pll2_n, 8, 4) << 1) |
                  (EBITS(config->pll2_m, 8, 1));
    mem[ 7 - 1] = (EBITS(config->pll3_m, 0, 8));
    mem[ 8 - 1] = (EBITS(config->pll3_n, 0, 8));
    mem[ 9 - 1] = (EBITS(config->p_sel[0], 0, 3) << 5) |
                  (EBITS(config->pll3_n, 8, 4) << 1) |
                  (EBITS(config->pll3_m, 8, 1));
    mem[10 - 1] = (EBITS(config->p_sel[1], 0, 3) << 5);

    mem[11 - 1] = (EBITS(config->p_sel[3], 0, 3) << 3) |
                  (EBITS(config->p_sel[2], 0, 3) << 0);
    mem[12 - 1] = (EBITS(powerdown, CLOCKCONFIG_PD, 1) << 6) |
                  (EBITS(config->p_sel[5], 0, 3) << 3) |
                  (EBITS(config->p_sel[4], 0, 3) << 0);

    mem[13 - 1] = (EBITS(config->p_div[0], 0, 7));
    mem[14 - 1] = (EBITS(config->p_div[1], 0, 7));
    mem[15 - 1] = (EBITS(config->p_div[2], 0, 7));
    mem[16 - 1] = (EBITS(config->p_div[3], 0, 7));
    mem[17 - 1] = (EBITS(config->p_div[4], 0, 7));
    mem[18 - 1] = (EBITS(config->p_div[5], 0, 7));

    mem[19 - 1] = (0x03 << 4) |
                  (EBITS(config->y_sel[0], 0, 1) << 3) |
                  (EBITS(y_sel[0], 0, 3));
    mem[20 - 1] = (0x03 << 4) |
                  (EBITS(config->y_sel[1], 0, 1) << 3) |
                  (EBITS(y_sel[1], 0, 3));
    mem[21 - 1] = (0x03 << 4) |
                  (EBITS(config->y_sel[2], 0, 1) << 3) |
                  (EBITS(y_sel[2], 0, 3));
    mem[22 - 1] = (0x03 << 4) |
                  (EBITS(config->y_sel[3], 0, 1) << 3) |
                  (EBITS(y_sel[3], 0, 3));
    mem[23 - 1] = (0x03 << 4) |
                  (EBITS(config->y_sel[4], 0, 1) << 3) |
                  (EBITS(y_sel[4], 0, 3));
    mem[24 - 1] = (0x03 << 4) |
                  (EBITS(config->y_sel[5], 0, 1) << 3) |
                  (EBITS(y_sel[5], 0, 3));

    for (addr = 0; addr < 24; addr++) {
        Write_CDCE906(addr + 1, mem[addr]);
    }

    // debug, read back and check
    for (addr = 0; addr < 24; addr++) {
        if (Read_CDCE906(addr + 1) != mem[addr]) {
            ok = FALSE;
        }
    }

    if (!ok) {
        ERROR("TWI:Clock Configure failure.");

    } else {
        DEBUG(1, "TWI:Clock Configure OK.");
    }

    //
    // M divider 1..511  N divider 1..4095  m <= n
    //  80 <= fvco <= 200 vf0=0
    // 180 <= fvco <= 300 vfo=1
    //
    //  input clock = 27MHz.
    //  output = (27 * n / m) / p. example m=375 n=2048 p=12 output = 12.288MHz. VCO = 147.456 MHz
    //
    // px sel (divider source selection)
    // 0 = bypass(input clock) 1 = pll1 2 = pll2 4 = pll3
    //
    // px div (divider) 0-127 divider value
    //
    // yx sel (0= divider0 .. 5= divider5) 0x1x = on
    //
    // y0 A0 FPGA DRAM        PLL1
    // y1 A1 Codec (pal/ntsc) 14.318180 NTSC/ 17.734475 PLL1 (off for HD)
    // y2 B0 FPGA sysclk      PLL2
    // y3 B1 Expansion Main -- off
    // y4 C0 FPGA video 27MHz/74.25MHz bypass SD/ PLL3 HD
    // y5 C1 Expansion Small
    //

    // PAL  17.73447                M= 46 N= 423 Fvco=248   p=14 output = 17.73447
    // PAL  28.37516 x 2 = 56.75032 M=481 N=4044 Fvco=227   p=4  output = 56.75052 error = 3.5ppm
    // NTSC 14.31818                M= 33 N= 280 Fvco=229   p=16 output = 14.31818
    // NTSC 28.63636 x 2 = 57.27272 M= 33 N= 280 Fvco=229   p=4  output = 57.27272
    // 50                           M= 27 N= 250 Fvco=250   p=5  output = 50
    // 133                          M= 27 N= 133 Fvco=133   p=1  output = 133    (vfo=0)
    // 74.25                        M= 2  N=11   Fvco=148.5 p=2  output = 74.25  (vfo=0)
}

void UpdatePLL(uint32_t pll, uint32_t m, uint32_t n, uint32_t pll_sel, uint32_t div, uint32_t y)
{
    // pll = {1,2,3}
    // m = {1,511}
    // n = {1,4095}
    // div = {1,127}
    // y = {0,5}
/*
1. Disable Y output
2. Bypass PLLx
3. Write new N/M values
4. Disable bypass
5. Set source PLL (Switch A)
6. Write new Ydiv value
7. Wait some arbitrary amount of time
8. Re-enable Y (Switch B)
*/
    DEBUG(0, "SetPLL(pll = %d, m = %d, n= %d, pll_sel = %d, div = %d, y = %d)", pll, m, n, pll_sel, div, y);

    // This PLL bit position is used in both register 3 and register 6
    const uint8_t pll_conf_bit = 1 << (8-pll);

    DEBUG(2, "Reading regs before switch");
    for (uint8_t addr = 0; addr < 24; addr++) {
        Read_CDCE906(addr+1);
    }

    DEBUG(2, "Making the switch");

    // Disable Y (keep nominal slew, non-inverted; Y div selection is reset)
    Write_CDCE906(19 + y, 0x30);

    // Bypass PLLx (read/modify/write to enable VCO bypass in the PLL MUX)
    uint8_t reg3 = Read_CDCE906(3);
    reg3 |= pll_conf_bit;
    Write_CDCE906(3, reg3);

    // Update M/N
    uint8_t lo_m = m & 0xff;        // separate lower bytes
    uint8_t lo_n = n & 0xff;

    uint8_t hi_m = (m >> 8) & 1;    // isolate upper bits (1 for M, 4 for N)
    uint8_t hi_n = (n >> (8-1)) & (0xf << 1); // and move into place

    uint8_t mn_base = 1+(pll-1)*3;  // base register for PLL M/N settings

    // read/modify/write keeping the upper 3 bits, which holds MUX/fVCOsel/PLL selection
    uint8_t hi = Read_CDCE906(mn_base+2) & 0xe0;

    Write_CDCE906(mn_base+0, lo_m);
    Write_CDCE906(mn_base+1, lo_n);
    Write_CDCE906(mn_base+2, hi | hi_n | hi_m);

    // Update VCO filter (read/modify/write, above 190Mhz we enable the fVCO filter)
    const uint32_t fvco = 27000 * n / m;
    DEBUG(0, "freq = %d", fvco/div);
    uint8_t reg6 = Read_CDCE906(6);
    reg6 &= ~pll_conf_bit;
    reg6 |= (fvco > 190000) ? pll_conf_bit : 0;
    Write_CDCE906(6, reg6);

    // Update source PLL (setting is split over registers 9 through 12)
    uint8_t regPx = 9+y;
    uint8_t shiftPx = 5;
    switch(y)
    {
        case 2: // reg 11
        case 3: // reg 11
        case 4: // reg 12
        case 5: // reg 12
            regPx = 11+y/4; // write reg 11/12
            shiftPx = ((y&1) ? 3 : 0); // odd Px in upper position
          break;
    }

    // Read/modify/write, updating the SWAPxy /Px
    uint32_t valPx = Read_CDCE906(regPx);
    valPx &= ~(0x7 << shiftPx);
    valPx |= (pll_sel << shiftPx);
    Write_CDCE906(regPx, valPx);

    // Update Ydiv
    Write_CDCE906(13 + y, (div & 0x7f));

    // let clock settle
    Timer_Wait(100);

    // Disable bypass PLLx (read/modify/write to mask the VCO bypass bit)
    reg3 = Read_CDCE906(3);
    reg3 &= ~pll_conf_bit;
    Write_CDCE906(3, reg3);

    // let clock settle
    Timer_Wait(100);

    // Enable Y (nominal slew, non-inverted, static Ydiv selection, Yx enabled)
    Write_CDCE906(19 + y, 0x30 | 0x08 | (y & 7));

    // let clock settle
    Timer_Wait(100);

    DEBUG(2, "Reading regs after switch");
    for (uint8_t addr = 0; addr < 24; addr++) {
        Read_CDCE906(addr+1);
    }

    DEBUG(1, "Switch done");
}

void Write_CH7301(uint8_t address, uint8_t data)
{
    uint8_t rab = 0x80 | (address & 0x7F);

    // device address is 1110110 = x76
    TWI_StartWrite(0x76, 1, rab, data);
    TWI_Wait_Tx();

    TWI_Stop();  // Set Stop
    TWI_Wait_Tx();
    TWI_Wait_Cmpl();
}

uint8_t Read_CH7301(uint8_t address)
{
    uint8_t rab = 0x80 | (address & 0x7F);
    uint8_t readData;

    TWI_StartWrite(0x76, 0, 0, rab);

    TWI_Wait_Tx();
    TWI_Wait_Cmpl();

    TWI_StartRead(0x76, 0, 0);
    TWI_Stop(); // set stop bit

    TWI_Wait_Rx();
    readData = TWI_ReadByte();
    TWI_Wait_Cmpl();
    return readData;
}

void Configure_CH7301(const vidconfig_t* config)
{
    // first of all, read ID register to make sure device is present
    uint8_t readData = Read_CH7301(0x4B);

    if (readData == 0x17) {
        DEBUG(2, "TWI:CH7301 found OK.");

    } else {
        DEBUG(2, "TWI:**** CH7301 not found. ****");
        return;
    }

    Write_CH7301(0x1C, config->reg1C);
    Write_CH7301(0x1D, config->reg1D);
    Write_CH7301(0x1E, config->reg1E);
    Write_CH7301(0x1F, config->reg1F);

    Write_CH7301(0x20, config->reg20);
    Write_CH7301(0x21, config->reg21);
    Write_CH7301(0x23, config->reg23);

    Write_CH7301(0x31, config->reg31);
    Write_CH7301(0x33, config->reg33);
    Write_CH7301(0x34, config->reg34);
    Write_CH7301(0x35, config->reg35);
    Write_CH7301(0x36, config->reg36);
    Write_CH7301(0x37, config->reg37);

    Write_CH7301(0x48, config->reg48);
    Write_CH7301(0x49, config->reg49);
    Write_CH7301(0x56, config->reg56);

}

