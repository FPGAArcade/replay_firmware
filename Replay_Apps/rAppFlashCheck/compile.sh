#!/bin/sh
set -ex
make clean && make
cp build/main.bin sdcard/rApp_ARM.bin
cp ../../loader_embedded/loader.bin sdcard/rApp_FPGA.bin
cp rApp_template.ini sdcard/rApp.ini
make -C ../../tools/loaderline clean linux
cd sdcard
../../../tools/loaderline/loaderline.elf >> rApp.ini
cd ..
diff -rupN rApp_template.ini sdcard/rApp.ini || true
