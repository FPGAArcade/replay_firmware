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

#if 0 // defined(ARDUINO_SAMD_MKRVIDOR4000)

#include "filesys/ff_blk.c"
#include "filesys/ff_crc.c"
#include "filesys/ff_dir.c"
#include "filesys/ff_error.c"
#include "filesys/ff_fat.c"
#include "filesys/ff_file.c"
//#include "filesys/ff_format.c"
//#undef FF_PartitionCount
#include "filesys/ff_hash.c"
#include "filesys/ff_ioman.c"
#include "filesys/ff_memory.c"
#include "filesys/ff_safety.c"
#include "filesys/ff_string.c"
#include "filesys/ff_time.c"
#include "filesys/ff_unaligned.c"
#include "filesys/ff_unicode.c"

#endif
