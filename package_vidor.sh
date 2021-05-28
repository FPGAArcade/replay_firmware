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

staging_path=`mktemp -d`

echo "Staging zip in ${staging_path}"

LIBPATH="${staging_path}/replay_mkrvidor4000"

GITTAG=`git describe --tag --always`
LIBZIP="replay_mkrvidor4000_build_${GITTAG}.zip"

cp -r "Replay_Boot/build/replay_mkrvidor4000" "${LIBPATH}"

# Zip it.
echo "Zipping firmware apps"
pushd "${staging_path}"
zip -r "${LIBZIP}" replay_mkrvidor4000
popd

mv  "${staging_path}/${LIBZIP}" .

echo "*** Complete ***"
echo "New Vidor 4000 library zipped to ${LIBZIP}"
echo "Temporary dir ${staging_path} may now be removed."
