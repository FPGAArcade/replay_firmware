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
require cat
require date
require zip
require mktemp
require make

# Staging dir is not automatically removed just in case anything goes
# wrong with pathing... It should be created in /tmp and go away on reboot.
staging_path=`mktemp -d`

echo "Staging zip in ${staging_path}"

# rAppUpdater
cp -r Replay_Apps/rAppFlashUpdater/sdcard ${staging_path}/rAppUpdater

# USBDL
cp -r Replay_Update ${staging_path}/usbUpdater

# reset work dir output
git checkout HEAD -- Replay_Apps/*/sdcard
git checkout HEAD -- Replay_Update/FW/main.s19

# create description
now=`date +%Y%m%d`_`git describe --tag --always --dirty --long`
fw_zip="Firmware_Replay1_${now}.zip"

# Readme info
cat << EOF > ${staging_path}/readme.txt
Replay1 Firmware (${now})

To update your replay firmware via a USB cable connected to the USB header
pins or mini usb socket on the board, refer to the instructions at the top
of the usbUpdater/replay_update.bat or usbUpdater/replay_update.sh file.

To update your replay firmware via sdcard, transfer the contents of the
rAppUpdater/ folder to your sdcard, boot the replay, load the rApp ini
file most suitable to your display setup then follow the on screen instructions.
EOF

# Zip it.
echo "Zipping firmware apps"
pushd "${staging_path}"
zip -r "${fw_zip}" rAppUpdater usbUpdater readme.txt
popd

mv  "${staging_path}/${fw_zip}" .

echo "*** Complete ***"
echo "New Firmware with rAppUpdater and USBDL zipped to ${fw_zip}"
echo "Temporary dir ${staging_path} may now be removed."
