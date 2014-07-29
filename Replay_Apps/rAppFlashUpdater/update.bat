rem $Id: compile.bat 218 2013-12-16 21:39:20Z wolfgang.scherr $
if exist c:\freeware\set_path_arm.bat call c:\freeware\set_path_arm

copy ..\..\..\arm_sw\Replay_Boot\build\main.bin sdcard\main.bin
copy rApp_template.ini sdcard\rApp.ini
cd sdcard
..\..\..\..\tools\genupd\genupd.exe >> rApp.ini
cd ..
pause
