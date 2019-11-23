#!/bin/sh
set -e
make clean
make
cp build/main.bin sdcard/rApp_ARM.bin
cp ../../../../hw/replay/cores/loader/sdcard/loader.bin sdcard/rApp_FPGA.bin
