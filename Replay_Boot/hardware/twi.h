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

#ifndef HARDWARE_TWI_H_INCLUDED
#define HARDWARE_TWI_H_INCLUDED

void TWI_Configure(void);
void TWI_Stop(void);
uint8_t TWI_ReadByte(void);
void TWI_WriteByte(uint8_t byte);
uint8_t TWI_ByteReceived(void);
uint8_t TWI_ByteSent(void);
uint8_t TWI_TransferComplete(void);
void TWI_StartRead(uint8_t DevAddr, uint8_t IntAddrSize, uint16_t IntAddr);
void TWI_StartWrite(uint8_t DevAddr, uint8_t IntAddrSize, uint16_t IntAddr, uint8_t WriteData);

#endif
