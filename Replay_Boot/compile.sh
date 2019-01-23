#!/bin/bash

#cp buildnum.elf buildnum

echo "Building firmware"
make

echo "Generating rApp Updater ini files"
cp build/main.bin ../Replay_Apps/rAppFlashUpdater/sdcard

pushd ../Replay_Apps/rAppFlashUpdater > /dev/null

# Generate and append firmware crc to each available template ini
for template in *_template_*.ini ; do
  new_ini=${template/_template_/_}

  cp ${template} sdcard/${new_ini}

  # genupd expects firmware to be in current dir
  cd sdcard
  ../../../../tools/genupd/genupd.elf >> ${new_ini}
  cd ..
done

# Zip up firmware and update app
echo "Zipping firmware and rAppUpdater"
now=`date +%Y%m%d_%H-%M`
fw_zip="Firmware_rAppUpdater_${now}.zip"
zip -r "${fw_zip}" sdcard

popd > /dev/null

mv  ../Replay_Apps/rAppFlashUpdater/"${fw_zip}" .

echo "*** Complete ***"
echo "New Firmware + rAppUpdater zipped to ${fw_zip}"

