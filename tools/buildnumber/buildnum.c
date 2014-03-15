#ifdef __linux
#include <time.h>
#else
#include<Windows.h>
#endif
#include<stdio.h>
// Build-Number-Generator
//
// (c) W. Scherr ws_arcade <at> pin4.at
// www.pin4.at
//
// Use at your own risk, all rights reserved
//
// $id:$
//

main() {
    FILE *buildFile;
    int buildNum;
    char buildDate[10];

#ifdef __linux
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

    printf("%s_%03d", buildDate, buildNum);

    buildFile=fopen("buildfile.num","wb");
    if (buildFile) {
        fwrite(&buildNum,sizeof(buildNum),1,buildFile);
        fclose(buildFile);
    }
}
