#!/usr/bin/env bash

# Based on the 'Travis CI Arduino Init Script' https://github.com/adafruit/travis-ci-arduino

# define colors
GRAY='\033[1;30m'; RED='\033[0;31m'; LRED='\033[1;31m'; GREEN='\033[0;32m'; LGREEN='\033[1;32m'; ORANGE='\033[0;33m'; YELLOW='\033[1;33m'; BLUE='\033[0;34m'; LBLUE='\033[1;34m'; PURPLE='\033[0;35m'; LPURPLE='\033[1;35m'; CYAN='\033[0;36m'; LCYAN='\033[1;36m'; LGRAY='\033[0;37m'; WHITE='\033[1;37m';

FAIL_MSG="""$LRED""ERROR!"
PASS_MSG="""$LGREEN""OK!"

export ARDUINO_PATH="$PWD/.arduino"
export ARDUINO_IDE_VERSION="1.8.9"
export ARDUINO_PACKAGE="arduino"
export ARDUINO_PLATFORM="samd"
export ARDUINO_BOARD="mkrvidor4000"
export ARDUINO_BOARD_URL="http://downloads.arduino.cc/packages/package_staging_index.json"
export ARDUINO_BOARD_FULL="${ARDUINO_PACKAGE}:${ARDUINO_PLATFORM}:${ARDUINO_BOARD}"
export ARDUINO_BOARD_VERSION="1.8.3"
export ARDUINO_PACKAGE_NAME="${ARDUINO_PACKAGE}:${ARDUINO_PLATFORM}:${ARDUINO_BOARD_VERSION}"

echo -e "${LGRAY}\n########################################################################";
echo -e "${YELLOW}INSTALLING ARDUINO IDE"
echo -e "${LGRAY}########################################################################";

echo -e "${LGRAY}ARDUINO_PATH         = ${ORANGE}${ARDUINO_PATH}"
echo -e "${LGRAY}ARDUINO_IDE_VERSION  = ${ORANGE}${ARDUINO_IDE_VERSION}"
echo -e "${LGRAY}ARDUINO_BOARD_URL    = ${ORANGE}${ARDUINO_BOARD_URL}"
echo -e "${LGRAY}ARDUINO_BOARD_FULL   = ${ORANGE}${ARDUINO_BOARD_FULL}"
echo -e "${LGRAY}ARDUINO_PACKAGE_NAME = ${ORANGE}${ARDUINO_PACKAGE_NAME}"
echo -e "${LGRAY}########################################################################";

echo -ne "${LGRAY}ARDUINO IDE STATUS: "

# remove cache if version changed
if [ ! -f "$ARDUINO_PATH/$ARDUINO_IDE_VERSION" ] && [ -f "$ARDUINO_PATH/arduino" ]; then
  echo -ne "${ORANGE}REMOVING OLD VERSION... "
  echo "Y" | rm -r "${ARDUINO_PATH}"
  if [ $? -ne 0 ]; then echo -ne "${FAIL_MSG} "; else echo -ne "${PASS_MSG} "; fi
fi

# if not already cached, download and install arduino IDE
if [ ! -f "$ARDUINO_PATH/arduino" ]; then
  if [ "$OSTYPE" == "linux-gnu" ]; then
    echo -ne "${ORANGE}DOWNLOADING... \n${LCYAN}"
    curl -f -# http://downloads.arduino.cc/arduino-${ARDUINO_IDE_VERSION}-linux64.tar.xz -o arduino.tar.xz 2>&1
    RET=$?
    echo -ne "${ORANGE}                    DOWNLOADED "
    if [ $RET -ne 0 ]; then echo -ne "${FAIL_MSG} "; else echo -ne "${PASS_MSG} "; fi
    echo -ne "${ORANGE}UNPACKING... "
    [ ! -d "$ARDUINO_PATH/" ] && mkdir "$ARDUINO_PATH"
    tar xf arduino.tar.xz -C "$ARDUINO_PATH/" --strip-components=1
    if [ $? -ne 0 ]; then echo -e "${FAIL_MSG}"; else echo -e "${PASS_MSG}"; fi
    touch "$ARDUINO_PATH/$ARDUINO_IDE_VERSION"
    if [ -f arduino.tar.xz ]; then rm arduino.tar.xz; fi
  elif [ "$OSTYPE" == "msys" ]; then
    echo -ne "${ORANGE}DOWNLOADING... \n${LCYAN}"
    curl -f -# http://downloads.arduino.cc/arduino-${ARDUINO_IDE_VERSION}-windows.zip -o arduino.zip 2>&1
    RET=$?
    echo -ne "${ORANGE}                    DOWNLOADED "
    if [ $RET -ne 0 ]; then echo -ne "${FAIL_MSG} "; else echo -ne "${PASS_MSG} "; fi
    echo -ne "${ORANGE}UNPACKING... "
    [ ! -d "$ARDUINO_PATH/" ] && mkdir "$ARDUINO_PATH"
    unzip -q arduino.zip -d .arduino
    if [ $? -ne 0 ]; then echo -e "${FAIL_MSG}"; else echo -e "${PASS_MSG}"; fi
    mv .arduino/arduino-${ARDUINO_IDE_VERSION}/* .arduino/
    rm -r .arduino/arduino-${ARDUINO_IDE_VERSION}
    touch "$ARDUINO_PATH/$ARDUINO_IDE_VERSION"
    if [ -f arduino.zip ]; then rm arduino.zip; fi
  else
    echo -ne "${ORANGE}Unknown host system! "
    echo -e "${FAIL_MSG}"
    exit 1
  fi
else
  echo -e "${ORANGE}CACHED ${PASS_MSG}"
fi

# make the IDE portable
mkdir -p "$ARDUINO_PATH/portable"

# add the IDE to the path
export PATH="$ARDUINO_PATH:$PATH"

echo -e "${LGRAY}\n########################################################################";
echo -e "${YELLOW}INSTALLING ${ARDUINO_BOARD_FULL} (${ARDUINO_BOARD_VERSION})"
echo -e "${LGRAY}########################################################################";

echo -ne "${LGRAY}ADD BOARD URL: "
BOARD_URL_OUTPUT=$(arduino --get-pref boardsmanager.additional.urls 2>/dev/null)
if [ x"${BOARD_URL_OUTPUT}" != x"${ARDUINO_BOARD_URL}" ]; then
  echo -ne "${ORANGE}${ARDUINO_BOARD_URL} "
  BOARD_URL_OUTPUT=$(arduino --pref "boardsmanager.additional.urls=${ARDUINO_BOARD_URL}" --save-prefs 2>&1)
  if [ $? -ne 0 ]; then echo -e "${FAIL_MSG}"; else echo -e "${PASS_MSG}"; fi
else
  echo -e "${ORANGE}CACHED ${PASS_MSG}"
fi

echo -ne "${LGRAY}INSTALLING BOARD: "
VERSION_OUTPUT=$(basename `arduino --get-pref runtime.platform.path 2>/dev/null`)
if [ x"${VERSION_OUTPUT}" != x"${ARDUINO_BOARD_VERSION}" ]; then
  echo -ne "${ORANGE}${ARDUINO_PACKAGE_NAME} "
  INSTALL_BOARD=$(arduino --install-boards ${ARDUINO_PACKAGE_NAME} 2>&1)
  if [ $? -ne 0 ]; then echo -e "${FAIL_MSG}"; else echo -e "${PASS_MSG}"; fi
else
  echo -e "${ORANGE}CACHED ${PASS_MSG}"
fi

echo -ne "${LGRAY}SWITCHING BOARD: "
PACKAGE_OUTPUT=$(arduino --get-pref target_package 2>/dev/null)
PLATFORM_OUTPUT=$(arduino --get-pref target_platform 2>/dev/null)
BOARD_OUTPUT=$(arduino --get-pref board 2>/dev/null)
if [ x"${PACKAGE_OUTPUT}:${PLATFORM_OUTPUT}:${BOARD_OUTPUT}" != x"${ARDUINO_BOARD_FULL}" ]; then
  echo -ne "${ORANGE}${ARDUINO_BOARD_FULL} "
  BOARD_OUTPUT=$(arduino --board ${ARDUINO_BOARD_FULL} --save-prefs 2>&1)
  if [ $? -ne 0 ]; then echo -e "${FAIL_MSG}"; else echo -e "${PASS_MSG}"; fi
else
  echo -e "${ORANGE}CACHED ${PASS_MSG}"
fi

echo -e "${LGRAY}\n########################################################################";
echo -e "${YELLOW}DONE!"
echo -e "${LGRAY}########################################################################";
