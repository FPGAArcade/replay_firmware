#
# Makefile for Linux / OSX with gcc
#
all: compress_loader

compressor: compressor.c
	gcc -o compressor compressor.c

compress_loader: compressor
	./compressor ../../../hw/replay/cores/loader_embedded/sdcard/loader.bin > ../../arm_sw/Replay_boot/loader.c

clean:
	rm compressor
