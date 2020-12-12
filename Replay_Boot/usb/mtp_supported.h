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

#pragma once
#include "ptp.h"
#include <stdint.h>

typedef uint16_t (*MtpOperationHandler)(PTPUSBBulkContainer*);

typedef uint16_t MtpContainerType;
typedef uint16_t MtpOperationCode;
typedef uint32_t MtpTransactionId;

typedef uint32_t MtpSessionId;
typedef uint32_t MtpStorageId;

typedef uint16_t MtpObjectFormat;
typedef uint32_t MtpObjectHandle;

#define MTP_INVALID_ID 0xffffffff

typedef struct _Operation {
    MtpOperationCode        code;
    MtpOperationHandler     func;
    const char* name;
} Operation;

