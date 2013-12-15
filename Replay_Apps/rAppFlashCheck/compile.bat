rem $Id$
if exist c:\freeware\set_path_arm.bat call c:\freeware\set_path_arm
make
copy build\main.bin sdcard\rApp_ARM.bin
copy ..\..\..\..\hw\replay\cores\loader\sdcard\loader.bin sdcard\rApp_FPGA.bin
