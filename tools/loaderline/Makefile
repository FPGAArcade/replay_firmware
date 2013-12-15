# libs must be AFTER the source arguments in gcc, otherwise link wil fail!!!
#
# should work with recent MinGW setups
#

LIBS   = -lkernel32 -luser32 -ladvapi32 -luuid -lsetupapi

all: loaderline.exe

loaderline.exe: loaderline.c 
	gcc -o loaderline.exe loaderline.c $(LIBS)

clean:
	rm loaderline.exe
