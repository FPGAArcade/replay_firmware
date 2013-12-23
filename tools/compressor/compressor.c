#include<Windows.h>
#include<stdio.h>

#include"miniz.c"

// File compressor
//
// (c) W. Scherr ws_arcade <at> pin4.at
// www.pin4.at
//
// Use at your own risk, all rights reserved
//
// $id:$
//
#include "stdint.h"

int main(int argc, char *argv[]) {
  FILE *inputFile;
  char *filename;

  if (argc>1) {
    filename = argv[1];
  } else {
    fprintf(stderr,"No filename given!\n");
    exit(-1);
  }

  printf("  // %s\n",filename);
  inputFile=fopen(filename,"rb");
  if (!inputFile) {
      fprintf(stderr,"Can't read file!\n");
      exit(-2);
  } else {
    uint32_t cmp_len, cmp_sum=0;
    int cmp_status;
    uint8_t fBuf[8192];
    uint8_t pBuf[8192];
    uint32_t src_len, src_sum=0;
    uint32_t max=0;

    printf("  const uint8_t loader[] = {\n");
    while (!feof(inputFile)) {
      int i;
      src_len=fread(fBuf,1,sizeof(fBuf),inputFile);
      src_sum+=src_len;
      cmp_len = compressBound(src_len);
      cmp_status = compress(pBuf, (mz_ulong *)&cmp_len, fBuf, src_len);
      if (cmp_status) {
        fprintf(stderr,"Error %d!\n",cmp_status);
        exit(-3);
      }
      cmp_sum+=cmp_len;
/*
      if ((pBuf[0]==0x78) && (pBuf[1]==0x01) && (pBuf[2]==0xed) && (pBuf[3]==0x99)) {
        // 4 header bytes can be compressed, we show this as well!
        printf("    0x%02x,0x%02x, ",0xC0 | ((cmp_len>>8)&0xFF),cmp_len&0xFF);
        i=4;
        cmp_sum-=2;
      } else if ((pBuf[0]==0x78) && (pBuf[1]==0x01) && (pBuf[2]==0xed) && (pBuf[3]==0x59)) {
        // 4 header bytes can be compressed, we show this as well!
        printf("    0x%02x,0x%02x, ",0x80 | ((cmp_len>>8)&0xFF),cmp_len&0xFF);
        i=4;
        cmp_sum-=2;
      } else if ((pBuf[0]==0x78) && (pBuf[1]==0x01) && (pBuf[2]==0xed)) {
        // 3 header bytes can be compressed, we show this as well!
        printf("    0x%02x,0x%02x, ",0x40 | ((cmp_len>>8)&0xFF),cmp_len&0xFF);
        i=3;
        cmp_sum-=1;
      } else if ((pBuf[0]==0x78) && (pBuf[1]==0x01)) {
        // 2 header bytes can be compressed, we show this as well!
        printf("    0x%02x,0x%02x, ",0x20 | ((cmp_len>>8)&0xFF),cmp_len&0xFF);
        i=2;
      } else {
*/
        // header bytes additionally needed
        cmp_sum+=2;
        printf("    0x%02x,0x%02x, ",0x00 | (cmp_len>>8)&0xFF,cmp_len&0xFF);
        i=0;
/*
      }
*/
      while (i<cmp_len) {
        printf("0x%02x,",pBuf[i++]);
      }
      printf(" // %d -> %d\n",src_len,cmp_len);
      if (cmp_len>max) max=cmp_len;
    }
    printf("};  // overall compression: %dk -> %dk (lagest junk: %d)\n",src_sum>>10,cmp_sum>>10,max);
      fclose(inputFile);
  }
}
