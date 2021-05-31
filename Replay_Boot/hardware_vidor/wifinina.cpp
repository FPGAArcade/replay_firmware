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

#define ENABLE_NINA_BT 0

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
    // NINA CS high during reset == WIFI mode
    // NINA CS low during reset == BT mode
    const uint8_t cs = wifi ? HIGH : LOW;
    digitalWrite(PIN_NINA_CS_L, cs);

    digitalWrite(PIN_NINA_RST_L, LOW);
    digitalWrite(PIN_NINA_GPIO0, HIGH);     // "Normal Boot from internal Flash"
    delay(10);
    digitalWrite(PIN_NINA_RST_L, HIGH);
    // takes about 500ms for NINA to reach the point where it samples BT/WIFI
    delay(500);
    digitalWrite(PIN_NINA_GPIO0, LOW);     // default LOW to not conflict with WIFI IRQ

    digitalWrite(PIN_NINA_CS_L, HIGH);
}

static void EnableNina()
{
    SPI.beginTransaction(SPISettings(6000000, MSBFIRST, SPI_MODE0));
    digitalWrite(PIN_NINA_CS_L, LOW);
}

static void DisableNina()
{
    digitalWrite(PIN_NINA_CS_L, HIGH);
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

Uart Serial2(&sercom3, PIN_NINA_RX, PIN_NINA_TX,
#if ENABLE_NINA_BT
             PAD_HCI_RX, PAD_HCI_TX, PIN_NINA_GPIO0, PIN_NINA_RDY_L
#else
             PAD_NINA_RX, PAD_NINA_TX
#endif
            );

void SERCOM3_Handler()
{
    Serial2.IrqHandler();
}

static void NINA_UpdateWIFI();
static void NINA_UpdateBT();

extern "C" void NINA_Update()
{
#if ENABLE_NINA_BT
    NINA_UpdateBT();
#else
    NINA_UpdateWIFI();
#endif
}

static void NINA_UpdateWIFI()
{
    static uint8_t _init = 0;

    if (_init == 0) {
        ResetNina(true);

        pinPeripheral(PIN_NINA_RX, PIO_SERCOM);
        pinPeripheral(PIN_NINA_TX, PIO_SERCOM);
        Serial2.begin(115200);
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

    while (Serial2.available()) {
        SerialUSB.write(Serial2.read());
    }
}

#if ENABLE_NINA_BT
// Requires patching ArduinoBLE to enable MKR Vidor 4000 support
#include <ArduinoBLE.h>
/*
diff --git a/src/utility/HCIUartTransport.cpp b/src/utility/HCIUartTransport.cpp
index 67bc173..da5e997 100644
--- a/src/utility/HCIUartTransport.cpp
+++ b/src/utility/HCIUartTransport.cpp
@@ -21,7 +21,8 @@

 #include "HCIUartTransport.h"

-#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_AVR_UNO_WIFI_REV2)
+#if defined(ARDUINO_SAMD_MKRWIFI1010) || defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_SAMD_MKRVIDOR4000)
+extern Uart Serial2;
 #define SerialHCI Serial2
 #elif defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_NANO_RP2040_CONNECT)
 // SerialHCI is already defined in the variant
@@ -93,7 +94,7 @@ size_t HCIUartTransportClass::write(const uint8_t* data, size_t length)
   return result;
 }

-#if defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_NANO_RP2040_CONNECT)
+#if defined(ARDUINO_AVR_UNO_WIFI_REV2) || defined(ARDUINO_NANO_RP2040_CONNECT) || defined(ARDUINO_SAMD_MKRVIDOR4000)
 HCIUartTransportClass HCIUartTransport(SerialHCI, 119600);
 #else
 HCIUartTransportClass HCIUartTransport(SerialHCI, 912600);
*/

static void NINA_UpdateBT()
{
    static uint8_t _init = 0;

    if (_init == 0) {
        ResetNina(false);

        pinPeripheral(PIN_NINA_RX, PIO_SERCOM);
        pinPeripheral(PIN_NINA_TX, PIO_SERCOM);
        pinPeripheral(PIN_NINA_GPIO0, PIO_SERCOM);  // RTS
        pinPeripheral(PIN_NINA_RDY_L, PIO_SERCOM);  // CTS

        // begin initialization
        if (!BLE.begin()) {
            WARNING("Failed to start BLE!");
            return;
        }

        DEBUG(1, "Start BLE Central scan");
        BLE.scan();

        _init++;
    }

    // check if a peripheral has been discovered
    BLEDevice peripheral = BLE.available();

    if (peripheral) {
        INFO("BLE: %s %lddB", peripheral.address().c_str(), peripheral.rssi());
        if (peripheral.hasLocalName()) {
            INFO("     %s", peripheral.localName().c_str());
        }
    }
}
#endif
