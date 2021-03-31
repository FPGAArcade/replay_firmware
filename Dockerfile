FROM ubuntu:20.10

RUN apt-get update
RUN apt-get install -y make git curl
RUN apt-get install -y gcc-arm-none-eabi
RUN apt-get install -y gcc g++ libncurses-dev
RUN apt-get install -y zip libarchive-zip-perl
RUN apt-get install -y dosfstools mtools valgrind

ARG REPO=https://github.com/FPGAArcade/replay_firmware.git
RUN cd /root                                                   && \
    git clone --depth 1 $REPO                                  && \
    ./replay_firmware/Replay_Boot/install_arduino_vidor.sh     && \
    rm -rf replay_firmware

ENV ARDUINO_ROOT=/root/.arduino
