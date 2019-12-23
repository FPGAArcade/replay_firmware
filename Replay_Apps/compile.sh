#!/bin/sh
set -ex
cd rAppFlashCheck && ./compile.sh && cd ..
cd rAppFlashUpdater && ./compile.sh && cd ..
cd rAppFlashUpdater && ./update.sh && cd ..
cd rAppMemTest && ./compile.sh && cd ..
