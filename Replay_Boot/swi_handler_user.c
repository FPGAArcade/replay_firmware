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

/* User SWI Handler for SWI's not handled in swi_handler.S */

#include <stdio.h>
#include "board.h"

unsigned long SWI_Handler_User(unsigned long reg0,
                               unsigned long reg1,
                               unsigned long reg2,
                               unsigned long swi_num )
{

    return 0;
}
