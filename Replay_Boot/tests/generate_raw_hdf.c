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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main(int argc, const char* argv[])
{
    int num_gbs = 4;	// default 4GiB

    if (argc > 1) {
        num_gbs = atoi(argv[1]);
    }

    if (num_gbs == 0 || num_gbs > 256) {
        return 1;
    }

    char filename[256];
    sprintf(filename, "raw_%igb.hdf", num_gbs);

    printf("Generating %i GiB file '%s' ... \n", num_gbs, filename);

    const uint64_t size = (uint64_t)num_gbs * 1024 * 1024 * 1024 / sizeof(uint32_t);

    FILE* f = fopen(filename, "wb");

    if (f) {
        uint8_t sector[512];
        uint32_t offset = 0;

        for (uint64_t i = 0; i < size; ++i) {
            uint32_t v = (uint32_t)i;

            uint8_t a = (v >>  0) & 0xff;
            uint8_t b = (v >>  8) & 0xff;
            uint8_t c = (v >> 16) & 0xff;
            uint8_t d = (v >> 24) & 0xff;

            sector[offset++] = d;
            sector[offset++] = c;
            sector[offset++] = b;
            sector[offset++] = a;

            if (offset == sizeof(sector)) {
                fwrite(sector, 1, sizeof(sector), f);
                offset = 0;
                printf("%3.2f%%\r", (float)(100.0 * (double)i / (double)size));
            }
        }

        fclose(f);
    }

    return 0;
}
