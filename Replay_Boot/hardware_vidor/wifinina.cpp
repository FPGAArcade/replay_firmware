#include "SPI.h"
#include <wiring_private.h>

extern "C" {

#include "messaging.h"
#include "hardware/io.h"
#include "hardware/spi.h"
#include "osd.h"
}

#define NINA_BEGIN          (0xe0)
#define NINA_END            (0xee)
#define NINA_SET_DEBUG      (0x1a)
#define NINA_GET_VERSION    (0x37)
#define NINA_PIN_MODE       (0x50)
#define NINA_DIGITAL_WRITE  (0x51)

extern "C" void SPI_DMA(const void* out, void* in, uint16_t length);

static void EnableNina()
{
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    digitalWrite(PIN_FPGA_CTRL0, LOW);
    digitalWrite(PIN_FPGA_CTRL1, LOW);
}

static void DisableNina()
{
    digitalWrite(PIN_FPGA_CTRL0, HIGH);
    digitalWrite(PIN_FPGA_CTRL1, HIGH);
    SPI.endTransaction();
}

static bool Wait(uint32_t pin, uint32_t val)
{
    HARDWARE_TICK timeout = Timer_Get(10);      // 10 ms timeout

    while (IO_Input_H(pin) != val) {
        if (Timer_Check(timeout)) {
            DEBUG(1, "NINA:RDY/ACK Timeout!");
            DisableNina();
            return false;
        }
    }

    return true;
}

static bool Wait(uint8_t c)
{
    HARDWARE_TICK timeout = Timer_Get(10);      // 10 ms timeout

    while (rSPI(0xff) != c) {
        if (Timer_Check(timeout)) {
            DEBUG(1, "NINA:CMD Timeout!");
            DisableNina();
            return false;
        }
    }

    return true;
}

static uint8_t response[32];

static const uint8_t* SPI_NINA(uint8_t* cmd, size_t length)
{
    if (length < 3) {
        return 0;
    }

    uint8_t op = cmd[1] & 0x7f;
    length = (length + 3) & (~0x3);

    // send command
    {
        if (!Wait(PIN_NINA_RDY_L, LOW)) {
            return 0;
        }

        EnableNina();

        if (!Wait(PIN_NINA_RDY_L, HIGH)) {
            return 0;
        }

        SPI_DMA(cmd, NULL, length);

        DisableNina();
    }

    // receive response
    {
        if (!Wait(PIN_NINA_RDY_L, LOW)) {
            return 0;
        }

        EnableNina();

        if (!Wait(PIN_NINA_RDY_L, HIGH)) {
            return 0;
        }

        if (!Wait(NINA_BEGIN)) {
            return 0;
        }

        if (rSPI(0xff) == (op | 0x80)) {
            uint8_t num_params = rSPI(0xff);
            uint8_t* p = &response[0];
            size_t left = sizeof(response);

            while (num_params--) {
                uint8_t len = rSPI(0xff);

                // only copy what we have space for.. 
                uint8_t copy = min(len, left);
                len -= copy;
                left -= copy;

                while (copy--) {
                    *p++ = rSPI(0xff);
                }

                // .. and sink the rest
                if (len)
                    SPI_DMA(0, 0, len);
            }

            if (rSPI(0xff) != NINA_END) {
                DEBUG(1, "NINA:CMD Error!");
            }

        } else {
            DEBUG(1, "NINA:CMD Error!");
        }

        DisableNina();
    }
    return response;
}

namespace nina
{
static void setDebug(uint8_t debug)
{
    uint8_t cmd[] = {
        NINA_BEGIN,
        NINA_SET_DEBUG,
        1/*params*/,
        sizeof(debug), debug,
        NINA_END
    };
    SPI_NINA(cmd, sizeof(cmd));
}

static const char* getVersion()
{
    uint8_t cmd[] = {
        NINA_BEGIN,
        NINA_GET_VERSION,
        0/*params*/,
        NINA_END
    };
    return (const char*)SPI_NINA(cmd, sizeof(cmd));
}

static void pinMode(uint8_t pin, uint8_t mode)
{
    uint8_t cmd[] = {
        NINA_BEGIN,
        NINA_PIN_MODE,
        2/*params*/,
        sizeof(pin), pin,
        sizeof(mode), mode,
        NINA_END
    };
    SPI_NINA(cmd, sizeof(cmd));
}

static void digitalWrite(uint8_t pin, uint8_t val)
{
    uint8_t cmd[] = {
        NINA_BEGIN,
        NINA_DIGITAL_WRITE,
        2/*params*/,
        sizeof(pin), pin,
        sizeof(val), val,
        NINA_END
    };
    SPI_NINA(cmd, sizeof(cmd));
}

}

static Uart SerialNina(&sercom3, PIN_NINA_RX, PIN_NINA_TX, PAD_NINA_RX, PAD_NINA_TX);

void SERCOM3_Handler()
{
    SerialNina.IrqHandler();
}

extern "C" void NINA_Update()
{
    // Check if NINA is available and ready
    if (!IO_Input_L(PIN_NINA_RDY_L)) {
        return;
    }

    static uint8_t _init = 0;

    if (!_init) {
        pinPeripheral(PIN_NINA_RX, PIO_SERCOM);
        pinPeripheral(PIN_NINA_TX, PIO_SERCOM);
        SerialNina.begin(115200);
        SerialUSB.begin(115200);

        INFO("NINA: %s\n", nina::getVersion());
        nina::setDebug(debuglevel>=1);

        nina::pinMode(PIN_LED_G_NINA, OUTPUT);
        nina::pinMode(PIN_LED_B_NINA, OUTPUT);

        _init = 1;
    }

    {
        SPI_EnableOsd();
        rSPI(OSDCMD_READSTAT);
        uint8_t x = rSPI(0);
        SPI_DisableOsd();

        const uint8_t power = x & STF_POWER_LED;
        const uint8_t drive = x & STF_DRIVE_LED;

        static uint8_t pwr = 0xff;
        static uint8_t drv = 0xff;

        if (power != pwr) {
            nina::digitalWrite(PIN_LED_G_NINA, power);
            pwr = power;
        }

        if (drive != drv) {
            nina::digitalWrite(PIN_LED_B_NINA, drive);
            drv = drive;
        }
    }

    while (SerialNina.available()) {
        SerialUSB.write(SerialNina.read());
    }
}
