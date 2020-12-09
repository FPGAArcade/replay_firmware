# libs must be AFTER the source arguments in gcc, otherwise link wil fail!!!
#
# should work with recent MinGW setups
#

# setup host
ifeq ($(OS),Windows_NT)
	HOST = Windows
	APP_SUFFIX = exe
else
	HOST := $(shell uname -s)
	APP_SUFFIX = elf
endif

LIBS   = -lkernel32 -luser32 -ladvapi32 -luuid -lsetupapi

all: buildnum_$(HOST).$(APP_SUFFIX)

%.exe: buildnum.c 
	gcc -o $@ $< $(LIBS)
	cp $@ ../../arm_sw/Replay_boot

%.elf: buildnum.c
	gcc -o $@ $<
	cp $@ ../../arm_sw/Replay_Boot

clean:
	rm -f *.elf *.exe

