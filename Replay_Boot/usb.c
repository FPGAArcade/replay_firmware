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

#include "usb.h"
#include "messaging.h"
#include "fileio.h"
#include "usb/msc.h"

extern FF_IOMAN* pIoman;

void USB_MountMassStorage(status_t* current_status)
{
    DEBUG(1, "MOUNTING USB! HOLD ON TO YOUR HATS!");

    if (FF_Mounted(pIoman)) {

        DEBUG(1, "Unmount partition..");

        for (int ch = 0; ch < 2; ++ch) {
            for (int drive = 0; drive < FCH_MAX_NUM; ++drive) {
                DEBUG(1, "eject %d / %d..", ch, drive);

                if (!FileIO_FCh_GetInserted(ch, drive)) {
                    continue;
                }

                DEBUG(1, "Ejecting '%s'...", FileIO_FCh_GetName(ch, drive));
                FileIO_FCh_Eject(ch, drive);
            }
        }

        // Make sure it's 'clean'...
        FileIO_FCh_Init();

        // Let's hope nobody is holding any file handles open at this time..

        FF_FlushCache(pIoman);
        FF_ERROR ret = FF_UnmountPartition(pIoman);

        if (ret != FF_ERR_NONE) {
            WARNING("Failed to unmount partition!");
            DEBUG(1, "FF_UnmountPartition return %08x", ret);
            return;
        }

        current_status->fs_mounted_ok = FALSE;
        DEBUG(1, "Partition unmounted!");
    }

    MSC_Start();

    current_status->usb_mounted = 1;
}

void USB_UnmountMassStorage(status_t* current_status)
{
    DEBUG(1, "UNMOUNTING USB! RE-INIT FILESYSTEM!");

    MSC_Stop();

    FileIO_FCh_Init();

    // CFG_card_start() will take care of remouting the sdcard..
    current_status->usb_mounted = 0;
}

void USB_Update(status_t* current_status)
{
    if (current_status->usb_mounted) {
        MSC_Poll();
    }
}
