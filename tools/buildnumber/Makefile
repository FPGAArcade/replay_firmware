# libs must be AFTER the source arguments in gcc, otherwise link wil fail!!!
#
# should work with recent MinGW setups
#

LIBS   = -lkernel32 -luser32 -ladvapi32 -luuid -lsetupapi

all: buildnum.exe

buildnum.exe: buildnum.c 
	gcc -o buildnum.exe buildnum.c $(LIBS)
	cp buildnum.exe ../../arm_sw/Replay_boot

clean:
	rm buildnum.exe
