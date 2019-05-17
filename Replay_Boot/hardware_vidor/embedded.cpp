#define INCBIN_PREFIX _binary_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "../hardware_host/incbin.h"

INCBIN(buildnum, "build/buildnum");
INCBIN(replay_ini, "../../../hw/replay/cores/loader_embedded/sdcard/replay.ini");

#include "board_driver_jtag.h"

extern "C" {
#include "../messaging.h"
}

/*

$ quartus_cpf.exe -c TEXT_Demo.sof TEXT_Demo.ttf
$ go run make_composite_binary.go -i TEXT_Demo.ttf:1:512 -o loader.h > signature.h

*/

#define no_data    0xFF, 0xFF, 0xFF, 0xFF, \
    0xFF, 0xFF, 0xFF, 0xFF, \
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, \
    0xFF, 0xFF, 0xFF, 0xFF, \
    0x00, 0x00, 0x00, 0x00  \

#define NO_BOOTLOADER   no_data
#define NO_APP        no_data
#define NO_USER_DATA    no_data

    __attribute__ ((used, section(".fpga_bitstream_signature")))
    const unsigned char signatures[4096] = {
#include "signature.h"
    };
    __attribute__ ((used, section(".fpga_bitstream")))
    const unsigned char bitstream[] = {
#include "loader.h"
    };

    extern "C" uint8_t pin_fpga_done;
    extern void enableFpgaClock();

    extern "C" void JTAG_StartEmbeddedCore()
    {
        enableFpgaClock();

        if (jtagInit() < 0) {
            ERROR("JTAG init failed!");
            return;
        }

        jtagBootApp();

        delay(1000);

        jtagDeinit();

        DEBUG(1, "Embedded core started.");
        pin_fpga_done = TRUE;
    }
