# libs must be AFTER the source arguments in gcc, otherwise link wil fail!!!
#
# should work with recent MinGW setups
#

LIBS   = -lkernel32 -luser32 -ladvapi32 -luuid -lsetupapi

all: genupd.exe

genupd.exe: genupd.c 
	gcc -o genupd.exe genupd.c $(LIBS)

clean:
	rm genupd.exe
