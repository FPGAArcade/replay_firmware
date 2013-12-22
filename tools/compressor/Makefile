# libs must be AFTER the source arguments in gcc, otherwise link wil fail!!!
#
# should work with recent MinGW setups
#

LIBS   = -lkernel32 -luser32 -ladvapi32 -luuid -lsetupapi

all: compressor.exe

compressor.exe: compressor.c 
	gcc -o compressor.exe compressor.c $(LIBS)
	cp ../../../hw/replay/cores/loader/sdcard/loader.bin .
	./compressor.exe > ../../arm_sw/Replay_boot/loader.c

clean:
	rm compressor.exe
