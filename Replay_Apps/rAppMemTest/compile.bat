rem $Id: compile.bat 218 2013-12-16 21:39:20Z wolfgang.scherr $
if exist c:\freeware\set_path_arm.bat call c:\freeware\set_path_arm
make clean && make
copy build\main.bin sdcard\rApp_ARM.bin
copy ..\..\loader_embedded\loader.bin sdcard\rApp_FPGA.bin
make -C ../../tools/genupd
copy rApp_template.ini sdcard\rApp.ini
cd sdcard
..\..\..\tools\genupd\genupd.exe >> rApp.ini
cd ..
