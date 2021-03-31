#!/bin/bash
set -ex
dd if=/dev/zero of=sdcard.bin bs=32768 count=4096
mkfs.fat -F 32 sdcard.bin
mcopy -i sdcard.bin ../loader_embedded/* ::/
mdir -i sdcard.bin
