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
mkdir -p "${LIBPATH}"

BUILDNUM=`od -An -vtu4 Replay_Boot/buildfile.num | tr -d [:space:]`
LIBZIP="replay_mkrvidor4000.zip"

cat << EOF > "${LIBPATH}/README.md"
# FPGAArcade Replay ARM firmware.
EOF

cat << EOF > "${LIBPATH}/library.properties"
name=Replay MKR Vidor 4000
version=0.0.${BUILDNUM}
author=eriquesnk@gmail.com
maintainer=eriquesnk@gmail.com
sentence=FPGAArcade Replay ARM firmware.
paragraph=Supports Replay FPGA cores for the Vidor.
category=Other
url=https://github.com/FPGAArcade/replay_release
architectures=*
precompiled=true
ldflags=-lreplay
EOF

mkdir -p "${LIBPATH}/examples/Replay_Boot"
mkdir -p "${LIBPATH}/src/cortex-m0plus"

cp "Replay_Boot/Replay_Boot.ino" "${LIBPATH}/examples/Replay_Boot"
cp "Replay_Boot/replay.h" "${LIBPATH}/src"
cp "Replay_Boot/build/libreplay.a" "${LIBPATH}/src/cortex-m0plus"

# Zip it.
echo "Zipping firmware apps"
pushd "${staging_path}"
zip -r "${LIBZIP}" replay_mkrvidor4000
popd

mv  "${staging_path}/${LIBZIP}" .

echo "*** Complete ***"
echo "New Vidor 4000 library zipped to ${LIBZIP}"
echo "Temporary dir ${staging_path} may now be removed."
