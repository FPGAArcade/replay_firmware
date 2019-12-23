rem $Id$
if exist c:\freeware\set_path_arm.bat call c:\freeware\set_path_arm
make clean && make
copy build\main.bin sdcard\rApp_ARM.bin
copy ..\..\..\..\hw\replay\cores\loader_embedded\sdcard\loader.bin sdcard\rApp_FPGA.bin
copy rApp_template.ini sdcard\rApp.ini
make -C ../../../tools/loaderline
cd sdcard
..\..\..\..\tools\loaderline\loaderline.exe >> rApp.ini
cd ..
