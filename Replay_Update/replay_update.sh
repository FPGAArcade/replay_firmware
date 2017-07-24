#!/bin/sh
echo "###################################################################"
echo "#                                                                 #"
echo "#      === FPGAARCADE.COM === REPLAY BOARD UPDATER ===            #"
echo "#                                                                 #"
echo "#    == Stay calm and follow this procedure thoroughly ==         #"
echo "#                                                                 #"
echo "# 1: If the board is switched on, switch it off first (!)         #"
echo "# 2: Connect a micro USB cable between board an PC                #"
echo "# 3: Then hold the replay menu button down and switch board on    #"
echo "# 4: The board will be detected (patience!), then flashing starts #"
echo "# 5: Release the menu button when you see the indicator dots '..' #"
echo "# 6: Wait until flashing is finished and this screen disappears   #"
echo "# 7: The board will automatically reboot after a few seconds      #"
echo "#                                                                 #"
echo "#    == If the replay board does not reboot/work afterwards ==    #"
echo "#                                                                 #"
echo "# A: Check you have a proper SDCARD (/w loader files on) inserted #"
echo "# B: Switch the board off for min. 5 seconds, then on again       #"
echo "# C: If it still does not work, try again above update procedure  #"
echo "#                                                                 #"
echo "###################################################################"

HOST=$(uname -s)
USBDL="usbdl_$HOST.elf"

echo
echo "Host system       = $HOST"
echo "Usbdl application = $USBDL"
echo

pushd FW > /dev/null

if [ ! -f $USBDL ]; then
    echo "ERROR : USBDL executable not found"
    exit 1
fi

if [ -f bootrom.bin ]; then
echo ./$USBDL full main.s19
	./$USBDL full main.s19
else
echo ./$USBDL load main.s19
	./$USBDL load main.s19
fi

echo
echo "Done"

popd > /dev/null
