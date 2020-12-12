/*--------------------------------------------------------------------
 *                           buildnum
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
 * Build-Number-Generator
 *
 * Copyright (c) 2020, W. Scherr, ws_arcade <at> pin4.at (www.pin4.at)
 *
 */

#if !defined(WIN32)
#include <time.h>
#else
#include <Windows.h>
#endif
#include <stdio.h>

int main() {
    FILE *buildFile;
    int buildNum;
    char buildDate[10];

#if !defined(WIN32)
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(buildDate, "%04d%02d%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
#else
    SYSTEMTIME sysTime;
    GetLocalTime(&sysTime);
    sprintf(buildDate, "%04d%02d%02d",sysTime.wYear,sysTime.wMonth,sysTime.wDay);
#endif

    buildFile=fopen("buildfile.num","rb");
    if (!buildFile) {
        buildNum=0;
    } else {
        fread(&buildNum,sizeof(buildNum),1,buildFile);
        fclose(buildFile);
        buildNum = (buildNum+1)%1000;
    }

    printf("%s_%03d%c", buildDate, buildNum, 0);

    buildFile=fopen("buildfile.num","wb");
    if (buildFile) {
        fwrite(&buildNum,sizeof(buildNum),1,buildFile);
        fclose(buildFile);
    }
}
