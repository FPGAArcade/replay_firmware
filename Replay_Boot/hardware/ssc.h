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

#ifndef HARDWARE_SSC_H_INCLUDED
#define HARDWARE_SSC_H_INCLUDED

void SSC_Configure_Boot(void);
void SSC_EnableTxRx(void);
void SSC_DisableTxRx(void);
void SSC_Write(uint32_t frame);
void SSC_WaitDMA(void);
void SSC_WriteBufferSingle(void* pBuffer, uint32_t length, uint32_t wait);

#endif
