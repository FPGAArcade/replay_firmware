rem $Id: compile.bat 218 2013-12-16 21:39:20Z wolfgang.scherr $
if exist c:\freeware\set_path_arm.bat call c:\freeware\set_path_arm
make
copy build\main.bin sdcard\rApp_ARM.bin
rem copy ..\..\..\..\hw\replay\cores\loader\sdcard\loader.bin sdcard\rApp_FPGA.bin
copy rApp_template.ini sdcard\rApp.ini
cd sdcard
..\..\..\..\tools\genupd\genupd.exe >> rApp.ini
pause