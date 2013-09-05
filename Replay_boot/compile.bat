if exist c:\freeware\set_path_arm.bat call c:\freeware\set_path_arm
make
copy /Y /B ..\..\bootloader\replay_loader_v1.2\usbdl.exe ..\Replay_Update\FW
copy /Y /B build\main.s19 ..\Replay_Update\FW
