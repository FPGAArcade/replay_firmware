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

#include "../board.h"
#include "../hardware/ssc.h"
#include "jtag.h"

extern uint8_t pin_fpga_done;
static uint32_t written = 0;

// SSC
void SSC_Configure_Boot(void)
{
}

void SSC_EnableTxRx(void)
{
    pin_fpga_done = FALSE;
    written = 0;
    JTAG_Init();
    JTAG_Reset();
    JTAG_StartBitstream();
}

void SSC_DisableTxRx(void)
{
    if (JTAG_EndBitstream()) {
        ERROR("Failed to configure FPGA (%d bytes written)", written);
        return;
    }

    if (written < BITSTREAM_LENGTH) {
        WARNING("Short bitstream (%d of %d bytes)", written, BITSTREAM_LENGTH);
    }

    DEBUG(0, "Successfully configured FPGA over JTAG");
    pin_fpga_done = TRUE;
    enableFpgaClock();
}

void SSC_Write(uint32_t frame)
{
    (void)frame;
}

void SSC_WaitDMA(void)
{
}

void SSC_WriteBufferSingle(void* pBuffer, uint32_t length, uint32_t wait)
{
    (void)wait; // no async support

    if (length == 0) {
        return;
    }

    written += length;
    bool done = written == BITSTREAM_LENGTH;

    FPGA_WriteBitstream((uint8_t*)pBuffer, length, done);
}
