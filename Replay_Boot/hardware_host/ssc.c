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
#include "hardware/ssc.h"

extern uint8_t pin_fpga_prog_l;
extern uint8_t pin_fpga_done;

static uint32_t written = 0;

// SSC
void SSC_Configure_Boot(void)
{
}

void SSC_EnableTxRx(void)
{
    written = 0;

    if (pin_fpga_prog_l) {
        pin_fpga_init_l = FALSE;

    } else {
        pin_fpga_init_l = TRUE;
    }
}

void SSC_DisableTxRx(void)
{
    if (written > BITSTREAM_LENGTH) {
        pin_fpga_done = TRUE;
    }
}

void SSC_Write(uint32_t frame)
{
    written ++;
}

void SSC_WaitDMA(void)
{
}

void SSC_WriteBufferSingle(void* pBuffer, uint32_t length, uint32_t wait)
{
    written += length;
}
