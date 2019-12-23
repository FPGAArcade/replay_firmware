#!/bin/sh
set -ex
make clean && make
make -C ../../../tools/genupd clean linux
cp build/main.bin sdcard/rApp_ARM.bin
cp ../../../../hw/replay/cores/loader_embedded/sdcard/loader.bin sdcard/rApp_FPGA.bin
cp rApp_template.ini sdcard/rApp.ini
cd sdcard
../../../../tools/genupd/genupd.elf >> rApp.ini || true
cd ..
