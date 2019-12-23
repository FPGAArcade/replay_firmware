#!/bin/bash
set -e

function require {
  name=$1

  if ! hash ${name} 2>/dev/null ; then
    echo "ERROR: ${name} required but not found on path. Aborting."
    exit 1
  fi
}

# Sanity check build requirements just in case someone sets this up on Jenkins and forgets :)
require make
require arm-none-eabi-gcc

echo "Building firmware"
make clean && make

echo "Copying firmware"
cp build/main.s19 ../Replay_Update/FW/
