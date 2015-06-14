rem $Id$
if exist c:\freeware\set_path_arm.bat call c:\freeware\set_path_arm
make
copy build\main.bin sdcard\rApp_ARM.bin
pause
