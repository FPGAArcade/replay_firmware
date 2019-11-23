#if defined(__linux) || defined(__APPLE__)
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

// reset with data = 0x00000000; length = 0xffffffff
static unsigned int feed_crc32(uint8_t* data, uint32_t length)
{
    static uint32_t crc;

    if (data == 0 && length == 0xffffffff) {
        crc = length;
        length = 0;
    }

    for (uint32_t i = 0; i < length; ++i) {
        uint32_t byte = *data++;
        crc = crc ^ byte;

        for (int j = 7; j >= 0; j--) {    // Do eight times.
            uint32_t mask = -(crc & 1);
            crc = (crc >> 1) ^ (0xEDB88320 & mask);
        }
    }

    return ~crc;
}

int main() {
    FILE *binFile;
    uint32_t sum=0;
    uint32_t len=0;
    uint32_t val=0;
    uint32_t ret=0;

    sum=0;len=0;
    binFile=fopen("bootrom.bin","rb");
    if (!binFile) {
        printf("# bootrom file not readable\n");
        ++ret;
    } else {
      feed_crc32(0x0,0xffffffff);
      while (!feof(binFile)) {
        if (fread(&val,sizeof(val),1,binFile)) {
          len+=4;
          sum=feed_crc32(&val, sizeof(val));
        };
      }
      fclose(binFile);
      printf("# bootrom file is %d bytes, crc32 is %08x\n",len,sum);
      printf("ROM = bootrom.bin,0x%06lx,0x00100000\n",len);
      printf("DATA = 0x00,0x00,0x10,0x00, 0x%02X,0x%02X,0x%02X,0x%02X, 0x%02X,0x%02X,0x%02X,0x%02X, 0x000FFE00,12\n",
             (len&0xFF),((len>>8)&0xFF),((len>>16)&0xFF),((len>>24)&0xFF),
             (sum&0xFF),((sum>>8)&0xFF),((sum>>16)&0xFF),((sum>>24)&0xFF) );
    }

    sum=0;len=0;
    binFile=fopen("main.bin","rb");
    if (!binFile) {
        printf("# main loader file not readable\n");
        ++ret;
    } else {
      feed_crc32(0x0,0xffffffff);
      while (!feof(binFile)) {
        if (fread(&val,sizeof(val),1,binFile)) {
          len+=4;
          sum=feed_crc32(&val, sizeof(val));
        };
      }
      fclose(binFile);
      printf("# main loader file is %d bytes, crc32 is %08x\n",len,sum);
      printf("ROM = main.bin,0x%06lx,0x00102000\n",len);
      printf("DATA = 0x00,0x20,0x10,0x00, 0x%02X,0x%02X,0x%02X,0x%02X, 0x%02X,0x%02X,0x%02X,0x%02X, 0x000FFE0C,12\n",
             (len&0xFF),((len>>8)&0xFF),((len>>16)&0xFF),((len>>24)&0xFF),
             (sum&0xFF),((sum>>8)&0xFF),((sum>>16)&0xFF),((sum>>24)&0xFF) );
    }

    sum=0;len=0;
    binFile=fopen("rApp_ARM.bin","rb");
    if (!binFile) {
        printf("# file not readable\n");
        ++ret;
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
    return ret;
}
