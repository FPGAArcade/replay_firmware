#!/bin/sh
set -ex
make clean && make
cp build/main.bin sdcard/rApp_ARM.bin
cp ../../../../hw/replay/cores/loader_embedded/sdcard/loader.bin sdcard/rApp_FPGA.bin
