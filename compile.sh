#!/bin/sh
set -ex
cd Replay_Boot && ./compile.sh && cd ..
cd Replay_Apps && ./compile.sh && cd ..
