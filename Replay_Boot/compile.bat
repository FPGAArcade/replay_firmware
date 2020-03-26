if exist c:\freeware\set_path_arm.bat call c:\freeware\set_path_arm
make clean && make
copy /Y /B build\main.s19 ..\Replay_Update\FW
