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
#include "../hardware/twi.h"
#include "../hardware/io.h"
#include "../hardware/timer.h"
#include "../messaging.h"

static uint8_t twi_offset = 0;
static uint8_t twi_memory[256];

void TWI_Configure(void)
{
}

void TWI_Stop(void)
{
    DEBUG(2, "%s - ", __FUNCTION__);
}

uint8_t TWI_ReadByte(void)
{
    DEBUG(2, "%s - ", __FUNCTION__);
    return twi_memory[twi_offset];
}

void TWI_WriteByte(uint8_t byte)
{
    DEBUG(2, "%s %02X - ", __FUNCTION__, byte);
    twi_memory[twi_offset] = byte;
}

uint8_t TWI_ByteReceived(void)
{
    DEBUG(2, "%s - ", __FUNCTION__);
    return 1;
}

uint8_t TWI_ByteSent(void)
{
    DEBUG(2, "%s - ", __FUNCTION__);
    return 1;
}

uint8_t TWI_TransferComplete(void)
{
    DEBUG(2, "%s\r\n", __FUNCTION__);
    return 1;
}

void TWI_StartRead(uint8_t DevAddr, uint8_t IntAddrSize, uint16_t IntAddr)
{
    DEBUG(2, "%s  %02x %02x %04x    - ", __FUNCTION__, DevAddr, IntAddrSize, IntAddr);
}

void TWI_StartWrite(uint8_t DevAddr, uint8_t IntAddrSize, uint16_t IntAddr, uint8_t WriteData)
{
    DEBUG(2, "%s %02x %02x %04x %02x - ", __FUNCTION__, DevAddr, IntAddrSize, IntAddr, WriteData);

    if (IntAddr) {
        twi_offset = IntAddr;
        twi_memory[twi_offset] = WriteData;

    } else {
        twi_offset = WriteData;
    }
}
