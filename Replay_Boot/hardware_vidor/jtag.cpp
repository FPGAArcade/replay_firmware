// Taken from https://github.com/Giako68/TEXT_DEMO

/******************************************************************************
*                                                                             *
* FILE: jtag.cpp                                                              *
*                                                                             *
******************************************************************************/

#include <Arduino.h>
#include "jtag.h"
#include "SPI.h"

#define PIN_TCK PIN_SPI1_SCK
#define PIN_TMS PIN_SPI1_SS
#define PIN_TDI PIN_SPI1_MOSI
#define PIN_TDO PIN_SPI1_MISO

#define SPI_JTAG SPI1

static int JTAG_Tick(int tms, int tdi)
{
    digitalWrite(PIN_TCK, LOW);
    digitalWrite(PIN_TMS, tms);
    digitalWrite(PIN_TDI, tdi);
    digitalWrite(PIN_TCK, HIGH);
    return (digitalRead(PIN_TDO));
}

static void JTAG_TMSPath(int num, int path)
{
    for (int i = 0; i < num; i++) {
        JTAG_Tick(((path & 0x00000001) ? HIGH : LOW), LOW);
        path = path >> 1;
    }
}

static void JTAG_ShiftIR()
{
    JTAG_Reset();
    JTAG_TMSPath(5, 0b00110);
}

static int JTAG_SendIR(int data)
{
    int Ret = 0, M = 0x00000001;
    JTAG_ShiftIR();

    for (int i = 0; i < 9; i++, M <<= 1)
        if (JTAG_Tick(LOW, (data & M) ? HIGH : LOW)) {
            Ret |= M;
        }

    if (JTAG_Tick(HIGH, (data & M) ? HIGH : LOW)) {
        Ret |= M;
    }

    return (Ret);
}

static unsigned char JTAG_SendDR8(int num, unsigned char data, int tms)
{
    unsigned char Ret = 0, M = 0x01;

    for (int i = 0; i < (num - 1); i++, M <<= 1)
        if (JTAG_Tick(LOW, (data & M) ? HIGH : LOW)) {
            Ret |= M;
        }

    if (JTAG_Tick(tms, (data & M) ? HIGH : LOW)) {
        Ret |= M;
    }

    return (Ret);
}

static void JTAG_SendBuffer(const uint8_t* data, size_t size)
{
    digitalWrite(PIN_TCK, LOW);

    PMUX(PIN_TCK, 1);
    PMUX(PIN_TDI, 1);

    for (size_t i = 0; i < size; i++) {
        SPI_JTAG.transfer(*data++);
    }

    PMUX(PIN_TCK, 0);
    PMUX(PIN_TDI, 0);
}

static void WaitTick(unsigned long Delay, int tms)
{
    unsigned long Stop;
    Stop = micros() + Delay;

    while (Stop > micros()) {
        JTAG_Tick(tms, LOW);
    }
}

// ----------------------------------------------------------------------------

void JTAG_Init()
{
    DEBUG(0, "JTAG_Init()");
    pinMode(PIN_TCK, OUTPUT);
    pinMode(PIN_TMS, OUTPUT);
    pinMode(PIN_TDI, OUTPUT);
    pinMode(PIN_TDO, INPUT);
    digitalWrite(PIN_TCK, LOW);
    digitalWrite(PIN_TMS, LOW);
    digitalWrite(PIN_TDI, LOW);

    SPI_JTAG.begin();
    SPI_JTAG.beginTransaction(DEFAULT_SPI_SETTINGS);

    // "patch" SPI settings to 24MHz, LSB_FIRST, SPI_MODE0
    PERIPH_SPI1.disableSPI();
    PERIPH_SPI1.initSPI(PAD_SPI_TX, PAD_SPI_RX, SPI_CHAR_SIZE_8_BITS, LSB_FIRST);
    PERIPH_SPI1.initSPIClock(SERCOM_SPI_MODE_0, 24000000);
    PERIPH_SPI1.enableSPI();

    // disable mux to allow bit-banging
    PMUX(PIN_TCK, 0);
    PMUX(PIN_TDI, 0);
}

void JTAG_Reset()
{
    DEBUG(0, "JTAG_Reset()");

    for (int i = 0; i < 5; i++) {
        JTAG_Tick(HIGH, LOW);
    }
}

int JTAG_StartBitstream()
{
    DEBUG(0, "JTAG_StartBitstream()");
    JTAG_SendIR(0x002);
    JTAG_TMSPath(2, 0b01);  // EXIT1_IR --> IDLE
    WaitTick(1000, LOW);    // Wait 1ms in IDLE
    JTAG_TMSPath(3, 0b001); // IDLE --> SHIFT_DR
    return 0;
}

void FPGA_WriteBitstream(uint8_t* buffer, uint32_t length, uint8_t done)
{
    if (done) {
        length--;
    }

    JTAG_SendBuffer(buffer, length);

    if (done) {
        JTAG_SendDR8(8, buffer[length], LOW);
    }
}

int JTAG_EndBitstream()
{
    unsigned char Check[135];
    DEBUG(0, "JTAG_EndBitstream()");

    JTAG_SendIR(0x004);
    JTAG_TMSPath(2, 0b01);  // EXIT1_IR --> IDLE
    WaitTick(5, LOW);       // Wait 5us in IDLE
    JTAG_TMSPath(3, 0b001); // IDLE --> SHIFT_DR

    for (int i = 0; i < 134; i++) {
        Check[134 - i] = JTAG_SendDR8(8, 0x00, LOW);
    }

    Check[0] = JTAG_SendDR8(8, 0x00, HIGH);

    if ((Check[83] & 0x02) != 0x02) {
        DEBUG(0, "JTAG check failed!");
        JTAG_Reset();
        return (-1);
    }

    DEBUG(0, "JTAG check ok!");

    JTAG_SendIR(0x003);
    JTAG_TMSPath(2, 0b01);  // EXIT1_IR --> IDLE
    WaitTick(4147, LOW);    // Wait 4147us in IDLE
    JTAG_SendIR(0x3FF);
    JTAG_TMSPath(2, 0b01);  // EXIT1_IR --> IDLE
    WaitTick(1000, LOW);    // Wait 1ms in IDLE

    return (0);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
