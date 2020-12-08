rem $Id$
if exist c:\freeware\set_path_arm.bat call c:\freeware\set_path_arm
make clean && make
copy build\main.bin sdcard\rApp_ARM.bin
copy ..\..\loader_embedded\loader.bin sdcard\rApp_FPGA.bin

