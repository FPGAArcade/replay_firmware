#!/bin/sh
cp ../../Replay_Update/FW/bootrom.bin sdcard/bootrom.bin
cp ../../Replay_Boot/build/main.bin sdcard/main.bin
cp rApp_template_NTSC_30FPS.ini sdcard/rApp_NTSC_30FPS.ini
cp rApp_template_NTSC_60FPS.ini sdcard/rApp_NTSC_60FPS.ini
cp rApp_template_PAL_25FPS.ini sdcard/rApp_PAL_25FPS.ini
cp rApp_template_PAL_50FPS.ini sdcard/rApp_PAL_50FPS.ini
make -C ../../../tools/genupd clean linux
cd sdcard
../../../../tools/genupd/genupd.elf >> rApp_NTSC_30FPS.ini
../../../../tools/genupd/genupd.elf >> rApp_NTSC_60FPS.ini
../../../../tools/genupd/genupd.elf >> rApp_PAL_25FPS.ini
../../../../tools/genupd/genupd.elf >> rApp_PAL_50FPS.ini
cd ..

diff -rupN rApp_template_NTSC_30FPS.ini sdcard/rApp_NTSC_30FPS.ini
diff -rupN rApp_template_NTSC_60FPS.ini sdcard/rApp_NTSC_60FPS.ini
diff -rupN rApp_template_PAL_25FPS.ini sdcard/rApp_PAL_25FPS.ini
diff -rupN rApp_template_PAL_50FPS.ini sdcard/rApp_PAL_50FPS.ini
echo
crc32 sdcard/*.bin
