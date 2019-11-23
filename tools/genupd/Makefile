# libs must be AFTER the source arguments in gcc, otherwise link wil fail!!!
#
# should work with recent MinGW setups
#

LIBS   = -lkernel32 -luser32 -ladvapi32 -luuid -lsetupapi

all: genupd.exe

genupd.exe: genupd.c 
	gcc -o genupd.exe genupd.c $(LIBS)

linux: genupd.c 
	gcc -std=c99 -o genupd.elf genupd.c

clean:
	rm -f genupd.exe genupd.elf
