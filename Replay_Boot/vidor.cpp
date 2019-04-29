#if defined(__SAMD21G18A__)

#include "hardware_vidor/embedded.cpp"
#include "hardware_vidor/hardware.cpp"

extern "C" {
#include "usb/msc.c"
}

#include "filesys/ff_format.c"

#endif
