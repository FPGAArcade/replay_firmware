#!/bin/sh

GIT=`git describe --tag --always --dirty="+"`

set -ex
cd Replay_Boot && ./compile.sh && cd ..
cd Replay_Apps && ./compile.sh && cd ..

set +ex
echo
echo Build DONE : "${GIT}"
echo
