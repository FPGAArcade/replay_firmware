#!/bin/sh
make
cp build/main.bin sdcard/rApp_ARM.bin
cp ../../../../hw/replay/cores/loader/sdcard/loader.bin sdcard/rApp_FPGA.bin
cp rApp_template.ini sdcard/rApp.ini
make -C ../../../tools/loaderline clean linux
cd sdcard
../../../../tools/loaderline/loaderline.elf >> rApp.ini
cd ..
diff -rupN rApp_template.ini sdcard/rApp.ini
