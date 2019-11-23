rem $Id: compile.bat 218 2013-12-16 21:39:20Z wolfgang.scherr $
if exist c:\freeware\set_path_arm.bat call c:\freeware\set_path_arm

copy ..\..\Replay_Update\FW\bootrom.bin sdcard\bootrom.bin
copy ..\..\Replay_Boot\build\main.bin sdcard\main.bin
copy rApp_template_NTSC_30FPS.ini sdcard\rApp_NTSC_30FPS.ini
copy rApp_template_NTSC_60FPS.ini sdcard\rApp_NTSC_60FPS.ini
copy rApp_template_PAL_25FPS.ini sdcard\rApp_PAL_25FPS.ini
copy rApp_template_PAL_50FPS.ini sdcard\rApp_PAL_50FPS.ini
make -C ../../../tools/genupd
cd sdcard
..\..\..\..\tools\genupd\genupd.exe >> rApp_NTSC_30FPS.ini
..\..\..\..\tools\genupd\genupd.exe >> rApp_NTSC_60FPS.ini
..\..\..\..\tools\genupd\genupd.exe >> rApp_PAL_25FPS.ini
..\..\..\..\tools\genupd\genupd.exe >> rApp_PAL_50FPS.ini
cd ..
pause
