#!/bin/bash
set -ex

# Update the bootloader and firmware binaries
cp ../../Replay_Update/FW/bootrom.bin sdcard/bootrom.bin
cp ../../Replay_Boot/build/main.bin sdcard/main.bin

# Make sure genupd.elf is built
make -C ../../tools/genupd clean linux

# Generate and append firmware crc to each available template ini
for template in *_template_*.ini ; do
  new_ini=${template/_template_/_}

  cp ${template} sdcard/${new_ini}

  # genupd expects firmware to be in current dir
  cd sdcard
  ../../../tools/genupd/genupd.elf >> ${new_ini}
  cd ..
done

# Output changes (for verification)
diff -rupN rApp_template_NTSC_30FPS.ini sdcard/rApp_NTSC_30FPS.ini | echo
diff -rupN rApp_template_NTSC_60FPS.ini sdcard/rApp_NTSC_60FPS.ini | echo
diff -rupN rApp_template_PAL_25FPS.ini sdcard/rApp_PAL_25FPS.ini | echo
diff -rupN rApp_template_PAL_50FPS.ini sdcard/rApp_PAL_50FPS.ini | echo

# Output CRC32 for binaries (for verification)
echo
crc32 sdcard/*.bin
