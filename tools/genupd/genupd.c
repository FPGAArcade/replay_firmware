#ifdef __linux
#else
#include<Windows.h>
#endif
#include<stdio.h>
#include<stdint.h>
// Checksum/Data INI-Line Generator for Replay Flash Updater
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
    FILE *binFile;
    uint32_t sum=0;
    uint32_t len=0;
    uint32_t val=0;

    sum=0;len=0;
    binFile=fopen("bootrom.bin","rb");
    if (!binFile) {
        printf("# bootrom file not readable\n");
    } else {
      while (!feof(binFile)) {
        if (fread(&val,sizeof(val),1,binFile)) {
          len+=4;
          sum+=val;
        };
      }
      fclose(binFile);
      printf("# bootrom file has %d byte\n",len);
      printf("ROM = bootrom.bin,0x%06lx,0x00100000\n",len);
      printf("DATA = 0x00,0x00,0x10,0x00, 0x%02X,0x%02X,0x%02X,0x%02X, 0x%02X,0x%02X,0x%02X,0x%02X, 0x000FFE00,12\n",
             (len&0xFF),((len>>8)&0xFF),((len>>16)&0xFF),((len>>24)&0xFF),
             (sum&0xFF),((sum>>8)&0xFF),((sum>>16)&0xFF),((sum>>24)&0xFF) );
    }

    sum=0;len=0;
    binFile=fopen("main.bin","rb");
    if (!binFile) {
        printf("# main loader file not readable\n");
    } else {
      while (!feof(binFile)) {
        if (fread(&val,sizeof(val),1,binFile)) {
          len+=4;
          sum+=val;
        };
      }
      fclose(binFile);
      printf("# main loader file has %d byte\n",len);
      printf("ROM = main.bin,0x%06lx,0x00102000\n",len);
      printf("DATA = 0x00,0x20,0x10,0x00, 0x%02X,0x%02X,0x%02X,0x%02X, 0x%02X,0x%02X,0x%02X,0x%02X, 0x000FFE0C,12\n",
             (len&0xFF),((len>>8)&0xFF),((len>>16)&0xFF),((len>>24)&0xFF),
             (sum&0xFF),((sum>>8)&0xFF),((sum>>16)&0xFF),((sum>>24)&0xFF) );
    }

    sum=0;len=0;
    binFile=fopen("rApp_ARM.bin","rb");
    if (!binFile) {
        printf("# file not readable\n");
    } else {
      while (!feof(binFile)) {
        if (fread(&val,sizeof(val),1,binFile)) {
          len+=4;
          sum+=val;
        };
      }
      fclose(binFile);
      printf("# rApp file has %d byte\n",len);
      printf("ROM = rApp_ARM.bin,0x%04lx,0x00000000\n",len);
      printf("LAUNCH = 0x00000000,0x%04lx,0x%08lx\n",len,sum);
    }
}
