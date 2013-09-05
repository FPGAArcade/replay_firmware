@echo off
echo ################################################################
echo #                                                              #
echo #      === FPGAARCADE.COM === REPLAY BOARD UPLOADER ===        #
echo #                                                              #
echo #    == Stay calm and follow this procedure thoroughly ==      #
echo #                                                              #
echo # 1: If the board is on, switch it off first                   #
echo # 2: Connect a micro USB cable between board an PC             #
echo # 3: Then hold the replay menu button down and switch board on #
echo # 4: The board will be detected (no rush!), then upload starts #
echo # 5: Release the menu button when you see the indicator dots   #
echo # 6: Wait until upload is finished and this screen disappears  #
echo # 7: The board will automatically reboot after a few seconds   #
echo #                                                              #
echo ################################################################
FW\usbdl load FW\main.s19

