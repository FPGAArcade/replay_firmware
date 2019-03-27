#include <stdint.h>
#include "exfat-test.h"
#include "messaging.h"

// from fileio_drv08_ata.c
FF_ERROR Drv08_HardFileSeek(fch_t* pDrive, void* pDesc, uint32_t lba);

void exfat_compare_sectors(fch_t* pDrive)
{
    uint8_t buf[512];
    uint64_t size = FF_Size(pDrive->fSource);
    const uint32_t sectors = (uint32_t)((size + sizeof(buf) - 1) / sizeof(buf));

    for (uint32_t seq = 0; seq < sectors; ++seq) {

        // create non-sequential reads by encoding part of the lba as gray code
        uint32_t a = (seq >> 8) & 0xff;
        uint32_t b = a & 0x80000000;

        for (int i = 31; i > 0; --i) {
            uint8_t v0 = (a >> (i - 0)) & 1;
            uint8_t v1 = (a >> (i - 1)) & 1;
            b |= ((v0 ^ v1) << (i - 1));

        }

        const uint32_t lba = (seq & 0xffff00ff) | (b << 8);
        Drv08_HardFileSeek(pDrive, pDrive->pDesc, lba);

        FF_T_SINT32 read = FF_Read(pDrive->fSource, sizeof(buf), 1, buf);

        if (read != sizeof(buf)) {
            DEBUG(0, "short read @ %d\n", lba);
            break;
        }

        uint64_t offset = (uint64_t)lba * 512 / 4;

        if ((lba & 0xff) == 0) {
            DEBUG(0, "@ %08x (%u kbytes, value = %08x)", (uint32_t)lba, (uint32_t)(offset * 4 / 1024), (uint32_t)offset);
        }

        for (int p = 0; p < sizeof(buf); ) {
            uint32_t v = 0;
            v |= buf[p++];
            v <<= 8;
            v |= buf[p++];
            v <<= 8;
            v |= buf[p++];
            v <<= 8;
            v |= buf[p++];

            if ((uint32_t)offset != v) {
                DEBUG(0, "mismatch read in sector %d ; %08x != %08x\n", lba, (uint32_t)offset, v);
            }

            offset++;
        }

    }
}
