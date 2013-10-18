#include<Windows.h>
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
    SYSTEMTIME sysTime;

    buildFile=fopen("buildfile.num","rb");
    if (!buildFile) {
        buildNum=0;
    } else {
        fread(&buildNum,sizeof(buildNum),1,buildFile);
        fclose(buildFile);
        buildNum = (buildNum+1)%1000;
    }

    GetLocalTime(&sysTime);
    printf("%04d%02d%02d_%03d",sysTime.wYear,sysTime.wMonth,sysTime.wDay,buildNum);

    buildFile=fopen("buildfile.num","wb");
    if (buildFile) {
        fwrite(&buildNum,sizeof(buildNum),1,buildFile);
        fclose(buildFile);
    }
}
