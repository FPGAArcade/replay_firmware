/*--------------------------------------------------------------------
 *                       Replay Firmware
 *                      www.fpgaarcade.com
 *                     All rights reserved.
 *
 *                     admin@fpgaarcade.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *--------------------------------------------------------------------
 *
 * Copyright (c) 2021, The FPGAArcade community (see AUTHORS.txt)
 *
 */

/*
  The latest offical NINA firmware (1.1.0) does not support BT (nor does it 
  have the recent bug fixes).

  Instead download it from: https://github.com/arduino/nina-fw/releases

  And flash it using the EspressIf ESP32 IDF tools:

  $ python3 $IDF_PATH/components/esptool_py/esptool/esptool.py              \
            --no-stub                                                       \
            --before default_reset --after hard_reset write_flash           \
            -z --flash_mode dio --flash_freq 40m --flash_size detect 0x0    \
            ~/Downloads/NINA_W102-v1.4.5-UNO-WIFI-REV2.bin

  (You need to be running the SerialNINAPassthrough.ino example at this point)

  Note that we're using the UNI-WIFI-REV2 firmware. There is a reason for that...
  This firmware configures the BT interface on the DEBUG/FLASH UART, while
  keeping the SPI interface (for WIFI) the same as the SDCARD/FPGA.

  ( see https://github.com/arduino/nina-fw/blob/master/main/sketch.ino.cpp#L121-L122 )

  This way we can use SERCOM3 to talk to it, if we run it in BT mode.
*/

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

static void ResetNina(bool wifi)
{
    // PIN_NINA_CS_L == PIN_FPGA_CTRL0 + PIN_FPGA_CTRL1
    // NINA CS high during reset == WIFI mode
    // NINA CS low during reset == BT mode
    const uint8_t cs = wifi ? HIGH : LOW;
    digitalWrite(PIN_FPGA_CTRL1, cs);
    digitalWrite(PIN_FPGA_CTRL0, cs);

    digitalWrite(PIN_NINA_RST_L, LOW);
    delay(10);
    digitalWrite(PIN_NINA_RST_L, HIGH);
    // takes about 500ms for NINA to reach the point where it samples BT/WIFI
    delay(500);

    digitalWrite(PIN_FPGA_CTRL1, HIGH);
    digitalWrite(PIN_FPGA_CTRL0, HIGH);
}

static void EnableNina()
{
    SPI.beginTransaction(SPISettings(6000000, MSBFIRST, SPI_MODE0));
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

static const uint8_t* SPI_NINA(uint8_t* cmd, size_t length, uint8_t* buffer, size_t buffer_size)
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
            uint8_t* p = &buffer[0];
            size_t left = buffer_size;

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
                if (len) {
                    SPI_DMA(0, 0, len);
                }
            }

            if (rSPI(0xff) != NINA_END) {
                DEBUG(1, "NINA:CMD Error!");
            }

        } else {
            DEBUG(1, "NINA:CMD Error!");
        }

        DisableNina();
    }
    return buffer;
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
    uint8_t response[32];
    SPI_NINA(cmd, sizeof(cmd), response, sizeof(response));
}

static const char* getVersion()
{
    uint8_t cmd[] = {
        NINA_BEGIN,
        NINA_GET_VERSION,
        0/*params*/,
        NINA_END
    };
    uint8_t response[32];
    return (const char*)SPI_NINA(cmd, sizeof(cmd), response, sizeof(response));
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
    uint8_t response[32];
    SPI_NINA(cmd, sizeof(cmd), response, sizeof(response));
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
    uint8_t response[32];
    SPI_NINA(cmd, sizeof(cmd), response, sizeof(response));
}

}

static Uart SerialNina(&sercom3, PIN_NINA_RX, PIN_NINA_TX, PAD_NINA_RX, PAD_NINA_TX);

void SERCOM3_Handler()
{
    SerialNina.IrqHandler();
}

extern "C" void NINA_Update()
{
    static uint8_t _init = 0;

    if (_init == 0) {
        ResetNina(true);

        pinPeripheral(PIN_NINA_RX, PIO_SERCOM);
        pinPeripheral(PIN_NINA_TX, PIO_SERCOM);
        SerialNina.begin(115200);
        SerialUSB.begin(115200);

        _init++;
    }

    // Check if NINA is available and ready
    if (!IO_Input_L(PIN_NINA_RDY_L)) {
        return;
    }

    if (_init == 1) {
        nina::setDebug(debuglevel >= 1);
        INFO("NINA: %s\n", nina::getVersion());

        nina::pinMode(0, INPUT);    // Set GPIO0 as INPUT
        nina::pinMode(PIN_LED_G_NINA, OUTPUT);
        nina::pinMode(PIN_LED_B_NINA, OUTPUT);

        _init++;
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
