#!/bin/bash
set -ex
cd Replay_Boot

# build Replay firmware
make clean && make -j2

# build Vidor firmware
export VIDOR=1
make clean && make -j2
unset VIDOR

# build HOSTED
export HOSTED=1
make clean && make -j2
unset HOSTED

# create sdcard.bin for HOSTED operations
./create_sdcard.sh
