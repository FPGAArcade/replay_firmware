// TODO autogenerate these..
#include "buildnum.h"
#include "replayhand.h"
#include "replay_ini.h"

#include "VidorUtils.h"

/*

$ quartus_cpf.exe -c TEXT_Demo.sof TEXT_Demo.ttf
$ go run make_composite_binary.go -i TEXT_Demo.ttf:1:512 -o loader.h > signature.h

*/

__attribute__ ((used, section(".fpga_bitstream_signature")))
const unsigned char signatures[4096] = {
#include "signature.h"
};
__attribute__ ((used, section(".fpga_bitstream")))
const unsigned char bitstream[] = {
#include "loader.h"
};

extern "C" void core_test();
extern "C" uint8_t pin_fpga_done;

extern "C" void JTAG_StartEmbeddedCore()
{
    VidorUtils utils;
    utils.begin();

    delay(1000);	// TODO : figure out a proper way to determine that the core is running..
    pin_fpga_done = true;

    core_test();
}
