#include "board.h"

#if defined(ARDUINO_SAMD_MKRVIDOR4000)

#include "hardware_vidor/embedded.cpp"
#include "hardware_vidor/hardware.cpp"

extern "C" {
#include "hardware_vidor/board_driver_jtag.c"
#include "usb/msc.c"
}

//#include "filesys/ff_format.c"

#endif
