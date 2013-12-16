rem $Id$
if exist c:\freeware\set_path_arm.bat call c:\freeware\set_path_arm
make
copy build\main.bin sdcard\rApp_ARM.bin
rem copy ..\..\..\bootloader\replay_loader_v1.2\bootrom.bin sdcard\bootrom.bin
rem copy ..\..\..\arm_sw\Replay_Boot\build\main.bin sdcard\main.bin
rem copy ..\..\..\..\hw\replay\cores\loader\sdcard\loader.bin sdcard\rApp_FPGA.bin
copy rApp_template.ini sdcard\rApp.ini
cd sdcard
..\..\..\..\tools\genupd\genupd.exe >> rApp.ini
pause