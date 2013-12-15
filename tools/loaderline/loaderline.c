#include<Windows.h>
#include<stdio.h>
#include<stdint.h>
// Checksum Generator for Replay Apps / INI lines
//
// $Id$
//
// (c) W. Scherr ws_arcade <at> pin4.at
// www.pin4.at
//
// Use at your own risk, all rights reserved
//
// $id:$
//

main() {
    FILE *rAppFile;
    uint32_t sum=0;
    uint32_t len=0;
    uint32_t val;

    rAppFile=fopen("rApp_ARM.bin","rb");
    if (!rAppFile) {
        printf("# file not readable - generate dummy line\n");
        printf("LAUNCH = 0x00000000,0x0000,0x00000000\n");
    } else {
      while (!feof(rAppFile)) {
        if (fread(&val,sizeof(val),1,rAppFile)) {
          len+=4;
          sum+=val;
        };
      }
      fclose(rAppFile);
      printf("# file has %d byte\n",len);
      printf("ROM = rApp_ARM.bin,0x%04lx,0x00000000\n",len);
      printf("LAUNCH = 0x00000000,0x%04lx,0x%08lx\n",len,sum);
    }
}
