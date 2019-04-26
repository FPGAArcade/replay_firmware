/******************************************************************************
*                                                                             *
* HDMI Text Screen demo - Core Test                                           *
*                                                                             *
******************************************************************************/

#include <Arduino.h>
#include "jtag.h"

class SPI_Out
{
  private:
    int SPI_MOSI, SPI_CLK, SPI_CS;

  public:
    SPI_Out(int MOSI, int CLK, int CS)
    {
        SPI_MOSI = MOSI;
        SPI_CLK = CLK;
        SPI_CS = CS;
        pinMode(SPI_MOSI, OUTPUT);
        pinMode(SPI_CLK, OUTPUT);
        pinMode(SPI_CS, OUTPUT);
        digitalWrite(SPI_MOSI, LOW);
        digitalWrite(SPI_CLK, LOW);
        digitalWrite(SPI_CS, LOW);
    }

    void StartTransfert()
    {
        digitalWrite(SPI_CS, HIGH);
    }

    void StopTransfert()
    {
        digitalWrite(SPI_CS, LOW);
    }

    void Send(unsigned char Data)
    {
        for (int i = 0; i < 8; i++) {
            digitalWrite(SPI_MOSI, ((Data & 0x80) == 0x80) ? HIGH : LOW);
            digitalWrite(SPI_CLK, HIGH);
            digitalWrite(SPI_CLK, LOW);
            Data = Data << 1;
        }

        {
            static uint8_t byte = 0;

            if (!byte++) {
                Serial1.print(".");
            }
        }
    }

};

static SPI_Out gSPI(PIN_SPI_MOSI, PIN_SPI_SCK, 7);

void TEXT_SetLocation(int Row, int Col)
{
    unsigned short Addr = Row * 80 + Col;
    gSPI.StartTransfert();
    gSPI.Send(0x82);
    gSPI.Send((Addr >> 8) & 0xFF);
    gSPI.Send(Addr & 0xFF);
    gSPI.StopTransfert();
}

void TEXT_SetAttribute(int Blink, int Foreground, int Background)
{
    gSPI.StartTransfert();
    gSPI.Send(0x83);
    gSPI.Send(Blink & 0x03);
    gSPI.Send(((Foreground << 4) & 0xF0) | (Background & 0x0F));
    gSPI.StopTransfert();
}

void TEXT_Print(char* Str)
{
    gSPI.StartTransfert();
    gSPI.Send(0x81);

    for (int i = 0; Str[i] != 0; i++) {
        gSPI.Send(Str[i]);
    }

    delay(5);
    gSPI.StopTransfert();
}

void TEXT_PrintAt(int Row, int Col, const char* Str)
{
    unsigned short Addr = Row * 80 + Col;
    gSPI.StartTransfert();
    gSPI.Send(0x82);
    gSPI.Send((Addr >> 8) & 0xFF);
    gSPI.Send(Addr & 0xFF);
    gSPI.Send(0x81);

    for (int i = 0; Str[i] != 0; i++) {
        gSPI.Send(Str[i]);
    }

    delay(5);
    gSPI.StopTransfert();
}

void TEXT_Repeat(char Ch, int Num)
{
    gSPI.StartTransfert();
    gSPI.Send(0x84);
    gSPI.Send((Num >> 8) & 0x0F);
    gSPI.Send(Num & 0xFF);
    gSPI.Send(Ch);
    delay(5);
    gSPI.StopTransfert();
}

void TEXT_ClearScreen(int Color)
{
    gSPI.StartTransfert();
    gSPI.Send(0x82);                 // Set Location (0,0)
    gSPI.Send(0x00);
    gSPI.Send(0x00);
    gSPI.Send(0x83);                 // Set Background = Color
    gSPI.Send(0x00);
    gSPI.Send(Color & 0x0F);
    gSPI.Send(0x84);                 // Write 3600 spaces
    gSPI.Send(0x0E);
    gSPI.Send(0x0F);
    gSPI.Send(' ');
    delay(5);
    gSPI.StopTransfert();
}

void TEXT_PrintCharAt(int Row, int Col, char Ch)
{
    unsigned short Addr = Row * 80 + Col;
    gSPI.StartTransfert();
    gSPI.Send(0x82);
    gSPI.Send((Addr >> 8) & 0xFF);
    gSPI.Send(Addr & 0xFF);
    gSPI.Send(0x81);
    gSPI.Send(Ch);
    delay(5);
    gSPI.StopTransfert();
}

void core_test()
{
    uint64_t current_ms = millis();

    DEBUG(0, "Writing core test over SPI...");
    SPI.end();

    // demux SPI pins
    PMUX(PIN_SPI_MISO, 0);
    PMUX(PIN_SPI_SCK, 0);
    PMUX(PIN_SPI_MOSI, 0);

    TEXT_ClearScreen(0);

    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            TEXT_SetAttribute(0, 15, 7);
            TEXT_PrintCharAt(y + 3, x + 3, y * 16 + x);
        }

    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            TEXT_SetAttribute(1, x, y);
            TEXT_PrintCharAt(y + 3, x + 22, y * 16 + x);
        }

    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            TEXT_SetAttribute(2, x, y);
            TEXT_PrintCharAt(y + 3, x + 42, y * 16 + x);
        }

    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++) {
            TEXT_SetAttribute(3, x, y);
            TEXT_PrintCharAt(y + 3, x + 61, y * 16 + x);
        }

    TEXT_SetAttribute(0, 9, 0);
    TEXT_PrintCharAt(0, 0, 0x1B);
    TEXT_Repeat(0x00, 77);
    TEXT_PrintCharAt(0, 79, 0x1C);

    for (int y = 1; y < 44; y++) {
        TEXT_PrintCharAt(y, 0, 0x0C);
        TEXT_PrintCharAt(y, 79, 0x10);
    }

    TEXT_PrintCharAt(21, 0, 0x19);
    TEXT_Repeat(0x04, 77);
    TEXT_PrintCharAt(21, 79, 0x1F);
    TEXT_PrintCharAt(44, 0, 0x19);
    TEXT_Repeat(0x04, 77);
    TEXT_PrintCharAt(44, 79, 0x1F);
    TEXT_SetAttribute(0, 10, 0);
    TEXT_PrintAt(24, 22, "#     # ### ######  ####### ###### ");
    TEXT_PrintAt(25, 22, "#     #  #  #     # #     # #     #");
    TEXT_PrintAt(26, 22, "#     #  #  #     # #     # #     #");
    TEXT_PrintAt(27, 22, "#     #  #  #     # #     # ###### ");
    TEXT_PrintAt(28, 22, " #   #   #  #     # #     # #   #  ");
    TEXT_PrintAt(29, 22, "  # #    #  #     # #     # #    # ");
    TEXT_PrintAt(30, 22, "   #    ### ######  ####### #     #");
    TEXT_SetAttribute(0, 12, 0);
    TEXT_PrintAt(32, 22, "  #         ###     ###     ###  ");
    TEXT_PrintAt(33, 22, "  #    #   #   #   #   #   #   # ");
    TEXT_PrintAt(34, 22, "  #    #  #     # #     # #     #");
    TEXT_PrintAt(35, 22, "  #    #  #     # #     # #     #");
    TEXT_PrintAt(36, 22, "  ####### #     # #     # #     #");
    TEXT_PrintAt(37, 22, "       #   #   #   #   #   #   # ");
    TEXT_PrintAt(38, 22, "       #    ###     ###     ###  ");

    Serial1.println("!");

    DEBUG(0, "%s() done in %d ms", __FUNCTION__, (uint32_t)(millis() - current_ms));

    SPI.begin();
}
