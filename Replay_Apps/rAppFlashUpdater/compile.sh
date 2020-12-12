#!/bin/sh
set -ex
make clean && make
cp build/main.bin sdcard/rApp_ARM.bin
cp ../../loader_embedded/loader.bin sdcard/rApp_FPGA.bin
