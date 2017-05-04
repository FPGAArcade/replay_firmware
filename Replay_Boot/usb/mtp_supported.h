#pragma once
#include "ptp.h"

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

