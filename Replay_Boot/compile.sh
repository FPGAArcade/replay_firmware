#!/bin/bash

cp buildnum.elf buildnum
make
cp build/main.bin ../Replay_Apps/rAppFlashUpdater/sdcard
cd ../Replay_Apps/rAppFlashUpdater/sdcard
cp ../rApp_template.ini rApp.ini
../../../../tools/genupd/genupd.elf >> rApp.ini
cp rApp.ini replay.ini

