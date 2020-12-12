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
#include "messaging.h"

static size_t Write8bits(PTPUSBBulkContainer* ptp, size_t offset, uint8_t data)
{
    if (offset + sizeof(uint8_t) < sizeof(ptp->payload.data)) {
        ptp->payload.data[offset++] = data;
    } else {
        DEBUG(1, "PTP : no more space (uint8_t)");
    }
    return offset;
}
static size_t Write16bits(PTPUSBBulkContainer* ptp, size_t offset, uint16_t data)
{
    if (offset + sizeof(uint16_t) < sizeof(ptp->payload.data)) {
        ptp->payload.data[offset++] = data;
        ptp->payload.data[offset++] = data >> 8;
    } else {
        DEBUG(1, "PTP : no more space (uint16_t)");
    }
    return offset;
}
static size_t Write32bits(PTPUSBBulkContainer* ptp, size_t offset, uint32_t data)
{
    if (offset + sizeof(uint32_t) < sizeof(ptp->payload.data)) {
        ptp->payload.data[offset++] = data;
        ptp->payload.data[offset++] = data >> 8;
        ptp->payload.data[offset++] = data >> 16;
        ptp->payload.data[offset++] = data >> 24;
    } else {
        DEBUG(1, "PTP : no more space (uint32_t)");
    }
    return offset;
}
static size_t Write64bits(PTPUSBBulkContainer* ptp, size_t offset, uint32_t data_hi, uint32_t data_lo)
{
    if (offset + sizeof(uint64_t) < sizeof(ptp->payload.data)) {
        ptp->payload.data[offset++] = data_lo;
        ptp->payload.data[offset++] = data_lo >> 8;
        ptp->payload.data[offset++] = data_lo >> 16;
        ptp->payload.data[offset++] = data_lo >> 24;
        ptp->payload.data[offset++] = data_hi;
        ptp->payload.data[offset++] = data_hi >> 8;
        ptp->payload.data[offset++] = data_hi >> 16;
        ptp->payload.data[offset++] = data_hi >> 24;
    } else {
        DEBUG(1, "PTP : no more space (uint64_t)");
    }
    return offset;
}
static size_t WriteString(PTPUSBBulkContainer* ptp, size_t offset, const char* data)
{
    size_t len = strlen(data) + 1;
    if (offset + (1+2*len) < sizeof(ptp->payload.data)) {
        ptp->payload.data[offset++] = len;
        do {
            if (*data & 0x80)
                DEBUG(1, "PTP : UTF8 strings are not handled!");
            ptp->payload.data[offset++] = *data;
            ptp->payload.data[offset++] = 0;
        } while(*data++);
    } else {
        DEBUG(1, "PTP : no more space (string)");
    }
    return offset;
}
static size_t WriteVariable(PTPUSBBulkContainer* ptp, size_t offset, uint16_t datatype, size_t data)
{
    if (datatype == PTP_DTC_UINT8) {
//        DEBUG(1, "PTP : 0x%02X / '%c'", (uint8_t) data, (uint8_t) data);
        offset = Write8bits(ptp, offset, (uint8_t) data);
    } else if (datatype == PTP_DTC_UINT16) {
//        DEBUG(1, "PTP : 0x%04X", (uint16_t) data);
        offset = Write16bits(ptp, offset, (uint16_t) data);
    } else if (datatype == PTP_DTC_UINT32) {
//        DEBUG(1, "PTP : 0x%08X", data);
        offset = Write32bits(ptp, offset, data);
    } else if (datatype == PTP_DTC_UINT64) {
        uint64_t value = data == 0 ? 0x0 : *(uint64_t*) data;
//        DEBUG(1, "PTP : 0x%08X%08X", value >> 32, value & 0xffffffff);
        offset = Write64bits(ptp, offset, value >> 32, value & 0xffffffff);
    } else if (datatype == PTP_DTC_STR) {
        const char* value = data == 0 ? "" : (const char*) data;
//        DEBUG(1, "PTP : %s", value);
        offset = WriteString(ptp, offset, value);
    } else
        DEBUG(1, "PTP : unknown datatype");
    return offset;
}

static size_t Read16bits(PTPUSBBulkContainer* ptp, size_t offset, uint16_t* data)
{
    uint16_t v;
    if (offset + sizeof(uint16_t) <= ptp->length - PTP_USB_BULK_HDR_LEN) {
        v  =  ptp->payload.data[offset++];
        v |= (ptp->payload.data[offset++] << 8);
        if (data)
            *data = v;
    } else {
        DEBUG(1, "PTP : no more data (uint16_t)");
    }
    return offset;
}
static size_t Read32bits(PTPUSBBulkContainer* ptp, size_t offset, uint32_t* data)
{
    uint32_t v;
    if (offset + sizeof(uint32_t) <= ptp->length - PTP_USB_BULK_HDR_LEN) {
        v  =  ptp->payload.data[offset++];
        v |= (ptp->payload.data[offset++] << 8);
        v |= (ptp->payload.data[offset++] << 16);
        v |= (ptp->payload.data[offset++] << 24);
        if (data)
            *data = v;
    } else {
        DEBUG(1, "PTP : no more data (uint32_t)");
    }
    return offset;
}
__attribute__ ((unused))
static size_t Read64bits(PTPUSBBulkContainer* ptp, size_t offset, uint32_t* data_hi, uint32_t* data_lo) 
{
    uint32_t v;
    if (offset + sizeof(uint64_t) <= ptp->length - PTP_USB_BULK_HDR_LEN) {
        v  =  ptp->payload.data[offset++];
        v |= (ptp->payload.data[offset++] << 8);
        v |= (ptp->payload.data[offset++] << 16);
        v |= (ptp->payload.data[offset++] << 24);
        if (data_lo)
            *data_lo = v;
        v  =  ptp->payload.data[offset++];
        v |= (ptp->payload.data[offset++] << 8);
        v |= (ptp->payload.data[offset++] << 16);
        v |= (ptp->payload.data[offset++] << 24);
        if (data_hi)
            *data_hi = v;
    } else {
        DEBUG(1, "PTP : no more data (uint64_t)");
    }
    return offset;
}
static size_t ReadString(PTPUSBBulkContainer* ptp, size_t offset, char* data)
{
    uint8_t v;
    uint8_t len = ptp->payload.data[offset++];
    if (offset + (2*len) <= ptp->length - PTP_USB_BULK_HDR_LEN) {
        while(len--) {
            v = ptp->payload.data[offset++];
            if (ptp->payload.data[offset++])
                DEBUG(1, "PTP : UTF8 strings are not handled!");
            if (data)
                *data++ = v;
        } 
    } else {
        DEBUG(1, "PTP : no more data (string)");
    }
    if (data)
        *data++ = 0;     // just in case..
    return offset;
}
