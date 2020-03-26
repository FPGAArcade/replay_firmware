/*
 WWW.FPGAArcade.COM

 REPLAY Retro Gaming Platform
 No Emulation No Compromise

 All rights reserved
 Mike Johnson Wolfgang Scherr

 SVN: $Id:

--------------------------------------------------------------------

 Redistribution and use in source and synthezised forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

 Redistributions in synthesized form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.

 Neither the name of the author nor the names of other contributors may
 be used to endorse or promote products derived from this software without
 specific prior written permission.

 THIS CODE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.

 You are responsible for any legal issues arising from your use of this code.

 The latest version of this file can be found at: www.FPGAArcade.com

 Email support@fpgaarcade.com
*/
//
// Support routines for the FPGA including file transfer, system control and DRAM test
//

#include "config.h"
#include "fpga.h"
#include "fileio.h"
#include "hardware/io.h"
#include "hardware/ssc.h"
#include "hardware/timer.h"
#include "osd.h"  // for keyboard input to DRAM debug only
#include "messaging.h"
#include "hardware/spi.h"

#ifndef FPGA_DISABLE_EMBEDDED_CORE
// Bah! But that's how it is proposed by this lib...
#define TINFL_HEADER_FILE_ONLY
#include "tinfl.c"
// ok, so it is :-)

extern char _binary_build_loader_start;
extern char _binary_build_loader_end;

static size_t read_embedded_core(void* buffer, size_t len, void* context)
{
    static uint32_t secCount = 0;
    uint32_t offset = *(uint32_t*)context;
    uint32_t size = &_binary_build_loader_end - &_binary_build_loader_start;

    if (offset + len > size) {
        len = size - offset;
    }

    memcpy(buffer, &_binary_build_loader_start + offset, len);
    *(uint32_t*)context += len;

    // showing some progress...
    if (!((secCount++ >> 4) & 3)) {
        ACTLED_ON;

    } else {
        ACTLED_OFF;
    }

    return len;
}
static size_t write_embedded_core(const void* buffer, size_t len, void* context)
{
    SSC_WriteBufferSingle((void*)buffer, len, 1);
    return len;
}

#endif

const uint8_t kMemtest[128] = {
    0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,
    0xAA, 0xAA, 0x55, 0x55, 0x55, 0x55, 0xAA, 0xAA, 0xAA, 0x55, 0x55, 0xAA, 0x55, 0xAA, 0xAA, 0x55,
    0x00, 0x01, 0x00, 0x02, 0x00, 0x04, 0x00, 0x08, 0x00, 0x10, 0x00, 0x20, 0x00, 0x40, 0x00, 0x80,
    0x01, 0x00, 0x02, 0x00, 0x04, 0x00, 0x08, 0x00, 0x10, 0x00, 0x20, 0x00, 0x40, 0x00, 0x80, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x0E, 0xEE, 0xEE, 0x44, 0x4D,
    0x11, 0x11, 0xEE, 0xEC, 0xAA, 0x55, 0x55, 0xAB, 0xBB, 0xBB, 0x00, 0x0A, 0xFF, 0xFF, 0x80, 0x09,
    0x80, 0x08, 0x00, 0x08, 0xAA, 0xAA, 0x55, 0x57, 0x55, 0x55, 0xCC, 0x36, 0x33, 0xCC, 0xAA, 0xA5,
    0xAA, 0xAA, 0x55, 0x54, 0x00, 0x00, 0xFF, 0xF3, 0xFF, 0xFF, 0x00, 0x02, 0x00, 0x00, 0xFF, 0xF1
};


//
// General
//

uint8_t FPGA_Default(void) // embedded in FW, something to start with
{

#if defined(ARDUINO_SAMD_MKRVIDOR4000)
    extern void JTAG_StartEmbeddedCore();
    JTAG_StartEmbeddedCore();
    return 0;
#endif

#ifdef FPGA_DISABLE_EMBEDDED_CORE
    WARNING("FPGA:Embedded core not available!");
    return 1;
#else
    HARDWARE_TICK time = Timer_Get(0);

    // set PROG low to reset FPGA (open drain)
    IO_DriveLow_OD(PIN_FPGA_PROG_L); //AT91C_BASE_PIOA->PIO_OER = PIN_FPGA_PROG_L;

    SSC_EnableTxRx(); // start to drive config outputs
    Timer_Wait(1);
    IO_DriveHigh_OD(PIN_FPGA_PROG_L);  //AT91C_BASE_PIOA->PIO_ODR = PIN_FPGA_PROG_L;
    Timer_Wait(2);

    // check INIT is high
    if (IO_Input_L(PIN_FPGA_INIT_L)) {
        WARNING("FPGA:INIT is not high after PROG reset.");
        return 1;
    }

    // check DONE is low
    if (IO_Input_H(PIN_FPGA_DONE)) {
        WARNING("FPGA:DONE is high before configuration.");
        return 1;
    }

    SSC_WaitDMA();

    // send FPGA data with SSC DMA in parallel to reading the file
    uint32_t read_offset = 0;
    size_t size = gunzip(read_embedded_core, &read_offset, write_embedded_core, NULL);
    (void)size; // unused-variable warning/error
    Assert(size == BITSTREAM_LENGTH);

    // some extra clocks
    SSC_Write(0x00);
    //
    SSC_DisableTxRx();
    ACTLED_OFF;
    Timer_Wait(1);

    // check DONE is high
    if (!IO_Input_H(PIN_FPGA_DONE) ) {
        WARNING("FPGA:Failed to config FPGA with fallback image. Fatal, system will reboot");
        return 1;

    } else {
        time = Timer_Get(0) - time;

        DEBUG(0, "FPGA configured with fallback image in %d ms.", Timer_Convert(time));
    }

    return 0;
#endif // #ifdef FPGA_DISABLE_EMBEDDED_CORE
}

uint8_t FPGA_Config(FF_FILE* pFile) // assume file is open and at start
{
    uint32_t bytesRead;
    uint32_t secCount;
    HARDWARE_TICK time;
    uint8_t fBuf1[FILEBUF_SIZE];
    uint8_t fBuf2[FILEBUF_SIZE];

    static const uint8_t BIT_HEADER[] = {
        0x0f, 0xf0,
        0x0f, 0xf0,
        0x0f, 0xf0,
        0x0f, 0xf0,
        0x00
    };

    static const uint8_t BIT_INIT           = 0x00;
    static const uint8_t BIT_NEXT           = 0x01;
    static const uint8_t BIT_NCD_NAME       = 0x61;
    static const uint8_t BIT_PART_NAME      = 0x62;
    static const uint8_t BIT_CREATE_DATE    = 0x63;
    static const uint8_t BIT_CREATE_TIME    = 0x64;
    static const uint8_t BIT_STREAM_LENGTH  = 0x65;
    static const uint32_t FileLength        = BITSTREAM_LENGTH;

    DEBUG(2, "FPGA:Starting Configuration.");

    // if file is larger than a raw .bin file let's see if it has a .bit header
    if (FF_Size(pFile)/*->Filesize*/ > FileLength) {
        char bitinfo[20] = {0}; // "YYYY/MM/DD HH:MM:SS",0

        uint8_t* data = fBuf1;
        const uint32_t maxSize = sizeof(fBuf1);

        uint8_t bitTag = 0x00;
        uint16_t length = 0;

        while (!FF_isEOF(pFile)) {
            uint16_t seekLength = 0;

            FF_Read(pFile, 1, sizeof(length), (uint8_t*)&length);
            length = (length << 8) | (length >> 8);

            if (length > maxSize) {
                seekLength = length - maxSize;
                length = maxSize;
            }

            if (BIT_STREAM_LENGTH == bitTag) {
                length = sizeof(uint32_t);
                FF_Seek(pFile, (FF_T_SINT32) - sizeof(length), FF_SEEK_CUR);
            }

            FF_Read(pFile, 1, length, data);

            if (BIT_INIT == bitTag) {
                if (length != sizeof(BIT_HEADER) ||
                        memcmp(data, BIT_HEADER, length)) {
                    WARNING("FPGA:Unknown binary format");
                    return 1;
                }

                ++bitTag;
                continue;

            } else if (BIT_NEXT == bitTag) {
                bitTag = data[0];
                continue;

            } else if (BIT_NCD_NAME == bitTag) {
                data[length] = '\0';
                DEBUG(0, "NCD: %s", data);

            } else if (BIT_PART_NAME == bitTag) {
                data[length] = '\0';
                DEBUG(0, "PART:%s", data);

            } else if (BIT_CREATE_DATE == bitTag) {
                data[length] = '\0';
                DEBUG(0, "DATE:%s", data);

                if (*bitinfo) {
                    strcat(bitinfo, " ");
                }

                strncat(bitinfo, (char*)data, 10);

            } else if (BIT_CREATE_TIME == bitTag) {
                data[length] = '\0';
                DEBUG(0, "TIME:%s", data);

                if (*bitinfo) {
                    strcat(bitinfo, " ");
                }

                strncat(bitinfo, (char*)data, 8);

            } else if (BIT_STREAM_LENGTH == bitTag) {
                uint32_t dataLength = (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3];

                if (FileLength != dataLength) {
                    WARNING("FPGA:Stream length mismatch");
                }

                break;
            }

            FF_Seek(pFile, seekLength, FF_SEEK_CUR);
            FF_Read(pFile, 1, sizeof(bitTag), &bitTag);
        }

        if (*bitinfo) {
            INFO("CORE: %s", bitinfo);
        }
    }

    if ((FF_Size(pFile) /*->Filesize*/ - FF_Tell(pFile) /*->FilePointer*/) < FileLength) {
        WARNING("FPGA:Bitstream too short!");
        return 1;
    }

    time = Timer_Get(0);

    // set PROG low to reset FPGA (open drain)
    IO_DriveLow_OD(PIN_FPGA_PROG_L); //AT91C_BASE_PIOA->PIO_OER = PIN_FPGA_PROG_L;

    SSC_Configure_Boot(); // TEMP MIKEJ SHOULD NOT BE NECESSARY

    SSC_EnableTxRx(); // start to drive config outputs
    Timer_Wait(1);
    IO_DriveHigh_OD(PIN_FPGA_PROG_L);  //AT91C_BASE_PIOA->PIO_ODR = PIN_FPGA_PROG_L;
    Timer_Wait(2);

    // check INIT is high
    if (IO_Input_L(PIN_FPGA_INIT_L)) {
        WARNING("FPGA:INIT is not high after PROG reset.");
        return 1;
    }

    // check DONE is low
    if (IO_Input_H(PIN_FPGA_DONE)) {
        WARNING("FPGA:DONE is high before configuration.");
        return 1;
    }

    // send FPGA data with SSC DMA in parallel to reading the file
    secCount = 0;

    do {
        uint8_t* pBufR;
        uint8_t* pBufW;

        // showing some progress...
        if (!((secCount++ >> 4) & 3)) {
            ACTLED_ON;

        } else {
            ACTLED_OFF;
        }

        // switch between 2 buffers to read-in
        if (secCount & 1) {
            pBufR = &(fBuf2[0]);

        } else {
            pBufR = &(fBuf1[0]);
        }

        bytesRead = FF_Read(pFile, FILEBUF_SIZE, 1, pBufR);
        // take the just read buffer for writing
        pBufW = pBufR;
        SSC_WaitDMA();
        SSC_WriteBufferSingle(pBufW, bytesRead, 0);
    } while (bytesRead > 0);

    SSC_WaitDMA();

    // some extra clocks
    SSC_Write(0x00);
    //
    SSC_DisableTxRx();
    ACTLED_OFF;
    Timer_Wait(1);

    // check DONE is high
    if (!IO_Input_H(PIN_FPGA_DONE) ) {
        WARNING("FPGA:Failed to config FPGA.");
        return 1;

    } else {
        time = Timer_Get(0) - time;

        DEBUG(0, "FPGA configured in %d ms.", Timer_Convert(time));
    }

    return 0;
}

//
// Memory Test
//

uint8_t FPGA_DramTrain(void)
{
    // actually just dram test for now
    uint8_t mBuf[512];
    uint32_t i;
    uint32_t addr;
    uint8_t error = 0;
    DEBUG(0, "FPGA:DRAM enabled, running test.");

    const uint32_t TESTsize = 7; // 128 bytes = sizeof(kMemtest)

#if defined(AT91SAM7S256) // Replay
    // 25..0  64MByte
    // 25 23        15        7
    // 00 0000 0000 0000 0000 0000 0000
    const uint32_t DRAMsize = 26; // 64MB
#elif defined(ARDUINO_SAMD_MKRVIDOR4000)
    const uint32_t DRAMsize = 23; // 8MB
#else
    const uint32_t DRAMsize = TESTsize;
#endif

    const uint32_t num_loops = DRAMsize - TESTsize;

    const uint32_t BUFFERsize = sizeof(kMemtest);
    Assert((1 << TESTsize) <= BUFFERsize);

    //uint16_t key;
    //do {
    memset(mBuf, 0x00, sizeof(mBuf));

    for (i = 0; i < BUFFERsize; i++) {
        mBuf[i] = kMemtest[i];
    }

    addr = 0;

    for (i = 0; i < num_loops; i++) {
        mBuf[BUFFERsize - 1] = (uint8_t) i;
        FileIO_MCh_BufToMem(mBuf, addr, BUFFERsize);
        addr = (0x100 << i);
    }

    addr = 0;

    for (i = 0; i < num_loops; i++) {

        int retry = 0;
        const int num_retries = 5;

        for (; retry < num_retries; ++retry) {
            memset(mBuf, 0xAA, 512);
            FileIO_MCh_MemToBuf(mBuf, addr, BUFFERsize);

            if (memcmp(mBuf, &kMemtest[0], BUFFERsize - 1) || (mBuf[BUFFERsize - 1] != (uint8_t) i) ) {
                continue;   // retry
            }

            break;
        }

        if (retry) {
            WARNING("!!Match fail Addr:%8X (%d)", addr, retry);
            DumpBuffer(mBuf, BUFFERsize);
            DEBUG(0, "Should be:");
            DumpBuffer(&kMemtest[0], BUFFERsize - 1);
            error = 1;
        }

        addr = (0x100 << i);
    }

    DEBUG(0, "FPGA:DRAM TEST %s.", error ? "failed" : "passed");

    //  key = OSD_GetKeyCode(NULL);
    //} while (key != KEY_MENU);

    return 0;
}

uint8_t FPGA_DramEye(uint8_t mode)
{
    uint32_t stat;
    uint32_t ram_phase;
    uint32_t ram_ctrl;
    uint16_t key;

    DEBUG(0, "FPGA:DRAM BIST stress.... this takes a while");

    ram_phase = kDRAM_PHASE;
    ram_ctrl  = kDRAM_SEL;
    OSD_ConfigSendCtrl((ram_ctrl << 8) | ram_phase);
    OSD_ConfigSendUserD(0x00000000); // ensure disabled
    OSD_ConfigSendUserD(0x01000000); // enable BIST
    //OSD_ConfigSendUserD(0x03000000); // enable BIST mode 1 (bounce/burst test)

    do {
        Timer_Wait(200);
        stat = OSD_ConfigReadStatus();

        if (mode != 0) {
            DEBUG(0, "BIST cycles :%01X err: %01X", stat & 0x38, stat & 0x04);
        }

        if (mode == 0) {
            if (((stat & 0x38) >> 3) == 2) { // two passes

                if (stat & 0x04) {
                    DEBUG(0, "FPGA:DRAM BIST **** FAILED !! ****");

                } else {
                    DEBUG(0, "FPGA:DRAM BIST passed.");
                }

                break;
            }
        }

        key = OSD_GetKeyCode(NULL);

        if (key == KEY_LEFT) {
            ram_phase -= 8;
            OSD_ConfigSendCtrl((ram_ctrl << 8) | ram_phase);
            OSD_ConfigSendUserD(0x00000000); // disable BIST
            OSD_ConfigSendUserD(0x01000000); // enable BIST

        }

        if (key == KEY_RIGHT) {
            ram_phase += 8;
            OSD_ConfigSendCtrl((ram_ctrl << 8) | ram_phase);
            OSD_ConfigSendUserD(0x00000000); // disable BIST
            OSD_ConfigSendUserD(0x01000000); // enable BIST
        }

    } while (key != KEY_MENU);

    OSD_ConfigSendUserD(0x00000000); // disable BIST

    return 0;
}


uint8_t FPGA_ProdTest(void)
{
    uint32_t ram_phase;
    uint32_t ram_ctrl;
    uint16_t key;

    DEBUG(0, "PRODTEST: phase 0");
    ram_phase = kDRAM_PHASE;
    ram_ctrl  = kDRAM_SEL;
    OSD_ConfigSendCtrl((ram_ctrl << 8) | ram_phase);
    FPGA_DramTrain();

    FPGA_DramEye(0); // /=0 for interactive

    // return to nominal
    ram_phase = kDRAM_PHASE;
    ram_ctrl  = kDRAM_SEL;
    OSD_ConfigSendCtrl((ram_ctrl << 8) | ram_phase);

    /*void OSD_ConfigSendUser(uint32_t configD, uint32_t configS)*/
    uint32_t testpat  = 0;
    uint32_t testnum  = 0;
    uint32_t maskpat  = 0;
    uint32_t vidpat   = 0;
    uint32_t vidstd   = 0;

    uint32_t stat     = 0;
    uint32_t config_d = 0;
    uint32_t config_s = 0;
    uint32_t failed   = 0;

    OSD_ConfigSendUserS(0); // default, no reset required

    do {
        config_d = ((maskpat & 0xF) << 18) | ((vidpat & 0x3) << 16) | ((testpat & 0xFF) << 8) | (testnum & 0xFF);

        OSD_ConfigSendUserD(config_d);
        Timer_Wait(1);
        OSD_ConfigSendUserD(config_d | 0x80000000);
        Timer_Wait(1);
        OSD_ConfigSendUserD(config_d | 0x00000000);
        Timer_Wait(1);

        stat = OSD_ConfigReadStatus();

        if ((stat & 0x03) != 0x01) {
            WARNING("Test %02X Num %02X, ** FAIL **", testpat, testnum);
            failed ++;
        }

        testnum++;

        switch (testpat) {
            case 0x00 :
                if (testnum == 0x32) {
                    testnum = 0;
                    testpat ++;
                }

                break;

            case 0x01 :
                if (testnum == 0x06) {
                    testnum = 0;
                    testpat ++;
                }

                break;

            case 0x02 :
                if (testnum == 0x18) {
                    testnum = 0;
                    testpat ++;
                }

                break;

            case 0x03 :
                if (testnum == 0x18) {
                    testnum = 0;
                    testpat ++;
                }

                break;

            case 0x04 :
                if (testnum == 0x18) {
                    testnum = 0;
                    testpat ++;
                }

                break;

            case 0x05 :
                if (testnum == 0x16) {
                    testnum = 0;
                    testpat ++;
                }

                break;

            case 0x06 :
                if (testnum == 0x26) {
                    testnum = 0;
                    testpat ++;
                }

                break;

            case 0x07 :
                if (testnum == 0x0A) {
                    testnum = 0;
                    testpat ++;
                }

                break;
        }

        if (testpat == 8) {
            if (failed == 0) {
                DEBUG(0, "IO TEST PASS");
            }

            testpat = 0;
            failed = 0;
        }

        key = OSD_GetKeyCode(NULL);

        if (key == KEY_F8) {
            vidpat = (vidpat - 1) & 0x3;
        }

        if (key == KEY_F9) {
            vidpat = (vidpat + 1) & 0x3;
        }

        if (key == KEY_F6) {
            if    (maskpat == 8) {
                maskpat = 0;

            } else if (maskpat != 0) {
                maskpat--;
            }

            DEBUG(0, "Mask %01X", maskpat);
        }

        if (key == KEY_F7) {
            if    (maskpat == 0) {
                maskpat = 8;

            } else if (maskpat != 15) {
                maskpat++;
            }

            DEBUG(0, "Mask %01X", maskpat);
        }

        if ((key == KEY_F4) || (key == KEY_F5)) {

            if (key == KEY_F4) {
                vidstd = (vidstd - 1) & 0x7;
            }

            if (key == KEY_F5) {
                vidstd = (vidstd + 1) & 0x7;
            }

            // update, hard reset of FPGA

            IO_DriveLow_OD(PIN_FPGA_RST_L); // make sure FPGA is held in reset

            // set up coder/clock
            switch (vidstd) {
                case 0 :
                    CFG_vid_timing_SD(F60HZ);
                    CFG_set_coder(CODER_NTSC);
                    break;

                case 1 :
                    CFG_vid_timing_SD(F50HZ);
                    CFG_set_coder(CODER_PAL);
                    break;

                case 2 :
                    CFG_vid_timing_SD(F60HZ);
                    CFG_set_coder(CODER_NTSC_NOTRAP);
                    break;

                case 3 :
                    CFG_vid_timing_SD(F50HZ);
                    CFG_set_coder(CODER_PAL_NOTRAP);
                    break;

                case 4 :
                    CFG_vid_timing_HD27(F60HZ);
                    CFG_set_coder(CODER_DISABLE);
                    break;

                case 5 :
                    CFG_vid_timing_HD27(F50HZ);
                    CFG_set_coder(CODER_DISABLE);
                    break;

                case 6 :
                    CFG_vid_timing_HD74(F60HZ);
                    CFG_set_coder(CODER_DISABLE);
                    break;

                case 7 :
                    CFG_vid_timing_HD74(F50HZ);
                    CFG_set_coder(CODER_DISABLE);
                    break;
            }

            DEBUG(0, "VidStd %01X", vidstd);
            Timer_Wait(200);

            IO_DriveHigh_OD(PIN_FPGA_RST_L); // release reset
            Timer_Wait(200);

            if ((vidstd == 6) || (vidstd == 7)) {
                CFG_set_CH7301_HD();

            } else {
                CFG_set_CH7301_SD();
            }

            // resend config
            config_s = (vidstd & 0x7);
            OSD_ConfigSendUserS(config_s);
            OSD_ConfigSendUserD(config_d);
            Timer_Wait(10);

            // apply new static config
            OSD_Reset(OSDCMD_CTRL_RES);

            Timer_Wait(10);
        }

        Timer_Wait(5);

    } while (key != KEY_MENU);

    return 0;
}

void FPGA_ClockMon(status_t* currentStatus)
{
    static uint8_t old_stat = 0xFF; // so we always update first time in
    uint8_t stat = (OSD_ConfigReadStatus() >> 12) & 0xF;

    if (stat != old_stat) {
        old_stat = stat;
        DEBUG(0, "Clock change : %02X", stat);
        DEBUG(0, " status %04X", (uint16_t)OSD_ConfigReadStatus());

        if (stat == 0 || stat == 8) { // default clock
            // (RTG enabled with sys clock is 8, but we could change the filters etc)

            // restore defaults
            if (currentStatus->coder_fitted) {
                CFG_set_coder(currentStatus->coder_cfg);
            };

            Configure_VidBuf(1, currentStatus->filter_cfg.stc, currentStatus->filter_cfg.lpf, currentStatus->filter_cfg.mode);

            Configure_VidBuf(2, currentStatus->filter_cfg.stc, currentStatus->filter_cfg.lpf, currentStatus->filter_cfg.mode);

            Configure_VidBuf(3, currentStatus->filter_cfg.stc, currentStatus->filter_cfg.lpf, currentStatus->filter_cfg.mode);

            Configure_ClockGen(&currentStatus->clock_cfg);

        } else {

            // coder off
            CFG_set_coder(CODER_DISABLE);

            // filter off
            Configure_VidBuf(1, 0, 3, 3);
            Configure_VidBuf(2, 0, 3, 3);
            Configure_VidBuf(3, 0, 3, 3);

            // jump to light speed
            clockconfig_t clkcfg = currentStatus->clock_cfg; // copy

            switch (stat & 0x07) { // ignore top bit
                //27 *  280/ 33  N/M
                case 6 : // 114.50 MHz
                    clkcfg.pll3_m = 33;
                    clkcfg.pll3_n = 280;
                    clkcfg.p_sel[4] = 4; // pll3
                    clkcfg.p_div[4] = 2;
                    clkcfg.y_sel[4] = 1; // on
                    break;

                case 5 : // 108.00 MHz
                    clkcfg.pll3_m = 27;
                    clkcfg.pll3_n = 216;
                    clkcfg.p_sel[4] = 4; // pll3
                    clkcfg.p_div[4] = 2;
                    clkcfg.y_sel[4] = 1; // on
                    break;

                case 4 : // 82.00 MHz
                    clkcfg.pll3_m = 27;
                    clkcfg.pll3_n = 164;
                    clkcfg.p_sel[4] = 4; // pll3
                    clkcfg.p_div[4] = 2;
                    clkcfg.y_sel[4] = 1; // on
                    break;

                case 3 : // 75.25 MHz
                    clkcfg.pll3_m = 2;
                    clkcfg.pll3_n = 11;
                    clkcfg.p_sel[4] = 4; // pll3
                    clkcfg.p_div[4] = 2;
                    clkcfg.y_sel[4] = 1; // on
                    break;

                case 2 : // 50.00 MHz
                    clkcfg.pll3_m = 27;
                    clkcfg.pll3_n = 150;
                    clkcfg.p_sel[4] = 4; // pll3
                    clkcfg.p_div[4] = 3;
                    clkcfg.y_sel[4] = 1; // on
                    break;

                case 1 : // 40.00 MHz
                    clkcfg.pll3_m = 27;
                    clkcfg.pll3_n = 160;
                    clkcfg.p_sel[4] = 4; // pll3
                    clkcfg.p_div[4] = 4;
                    clkcfg.y_sel[4] = 1; // on
                    break;

            }

            Configure_ClockGen(&clkcfg);
        }

        // let clock settle
        Timer_Wait(100);

        // set CH7301 PLL filter
        switch (stat & 0x07) { // ignore top bit
            case 6 :
            case 5 :
            case 4 :
            case 3 :
                // >= 65MHz
                DEBUG(0, "DPLL high rate");
                Write_CH7301(0x33, 0x06);
                Write_CH7301(0x34, 0x26);
                Write_CH7301(0x36, 0xA0);
                break;

            default :
                //<= 65MHz
                DEBUG(0, "DPLL low rate");
                Write_CH7301(0x33, 0x08);
                Write_CH7301(0x34, 0x16);
                Write_CH7301(0x36, 0x60);
        }

        // do video reset
        OSD_Reset(OSDCMD_CTRL_RES_VID);

    }
}

//
// Replay Application Call (rApps):
//
// ----------------------------------------------------------------
//  we need this local/inline to avoid function calls in this stage
// ----------------------------------------------------------------



static inline void _FPGA_WaitStat(uint8_t mask, uint8_t wanted)
{
    do {
        _SPI_EnableFileIO();
        _rSPI(0x87); // do Read

        if ((_rSPI(0) & mask) == wanted) {
            break;
        }

        _SPI_DisableFileIO();
    } while (1);

    _SPI_DisableFileIO();
}


void FPGA_ExecMem(uint32_t base, uint16_t len, uint32_t checksum)
{
    uint32_t i, j, l = 0, sum = 0;
    volatile uint32_t* dest = (volatile uint32_t*)0x00200000L;
    uint8_t value;

    DEBUG(0, "FPGA:copy %d bytes from 0x%lx and execute if the checksum is 0x%lx", len, base, checksum);
    DEBUG(0, "FPGA:we have about %ld bytes free for the code", ((uint32_t)(intptr_t)&value) - 0x00200000L);

    if ((((uint32_t)(intptr_t)&value) - 0x00200000L) < len) {
        WARNING("FPGA: Not enough memory, processor may crash!");
    }

    _SPI_EnableFileIO();
    _rSPI(0x80); // set address
    _rSPI((uint8_t)(base));
    _rSPI((uint8_t)(base >> 8));
    _rSPI((uint8_t)(base >> 16));
    _rSPI((uint8_t)(base >> 24));
    _SPI_DisableFileIO();

    _SPI_EnableFileIO();
    _rSPI(0x81); // set direction
    _rSPI(0x80); // read
    _SPI_DisableFileIO();

    // LOOP FOR BLOCKS TO READ TO SRAM
    for (i = 0; i < (len / 512) + 1; ++i) {
        uint32_t buf[128];
        uint32_t* ptr = &(buf[0]);
        _SPI_EnableFileIO();
        _rSPI(0x84); // read first buffer, FPGA stalls if we don't read this size
        _rSPI((uint8_t)( 512 - 1));
        _rSPI((uint8_t)((512 - 1) >> 8));
        _SPI_DisableFileIO();
        _FPGA_WaitStat(0x04, 0);
        _SPI_EnableFileIO();
        _rSPI(0xA0); // should check status
        _SPI_ReadBufferSingle(buf, 512);
        _SPI_DisableFileIO();

        for (j = 0; j < 128; ++j) {
            // avoid summing up undefined data in the last block
            if (l < len) {
                sum += *ptr++;

            } else {
                break;
            }

            l += 4;
        }
    }

    // STOP HERE
    if (sum != checksum) {
        ERROR("FPGA:CHK exp: 0x%lx got: 0x%lx", checksum, sum);
        dest = (volatile uint32_t*)0x00200000L;
        DEBUG(0, "FPGA:<-- 0x%08lx", *(dest));
        DEBUG(0, "FPGA:<-- 0x%08lx", *(dest + 1));
        DEBUG(0, "FPGA:<-- 0x%08lx", *(dest + 2));
        DEBUG(0, "FPGA:<-- 0x%08lx", *(dest + 3));
        return;
    }

    // NOW COPY IT TO RAM!
    // no variables in mem from here...

    sum = 0;
    dest = (volatile uint32_t*)0x00200000L;
    DEBUG(0, "FPGA:SRAM start: 0x%lx (%d blocks)", (uint32_t)(intptr_t)dest, 1 + len / 512);
    Timer_Wait(500); // take care we can send this message before we go on!

    _SPI_EnableFileIO();
    _rSPI(0x80); // set address
    _rSPI((uint8_t)(base));
    _rSPI((uint8_t)(base >> 8));
    _rSPI((uint8_t)(base >> 16));
    _rSPI((uint8_t)(base >> 24));
    _SPI_DisableFileIO();

    _SPI_EnableFileIO();
    _rSPI(0x81); // set direction
    _rSPI(0x80); // read
    _SPI_DisableFileIO();

    // LOOP FOR BLOCKS TO READ TO SRAM
    for (i = 0; i < (len / 512) + 1; ++i) {
        _SPI_EnableFileIO();
        _rSPI(0x84); // read first buffer, FPGA stalls if we don't read this size
        _rSPI((uint8_t)( 512 - 1));
        _rSPI((uint8_t)((512 - 1) >> 8));
        _SPI_DisableFileIO();
        _FPGA_WaitStat(0x04, 0);
        _SPI_EnableFileIO();
        _rSPI(0xA0); // should check status
        _SPI_ReadBufferSingle((void*)dest, 512);
        _SPI_DisableFileIO();

        for (j = 0; j < 128; ++j) {
            *dest++;
        }
    }

    // execute from SRAM the code we just pushed in
#if defined(__arm__)
    asm("ldr r3, = 0x00200000\n");
    asm("bx  r3\n");
#endif
}

#ifdef TINFL_HEADER_INCLUDED

//
// ZLIB inflate (decompress) a stream through read/write callbacks
//
size_t zlib_inflate(inflate_read_func_ptr read_func, void* const read_context, const size_t read_buffer_size, inflate_write_func_ptr write_func, void* const write_context, int flags)
{
    const size_t write_buffer_size = 32 * 1024; // TINFL_LZ_DICT_SIZE

    mz_uint8 read_buffer[read_buffer_size];
    mz_uint8 write_buffer[write_buffer_size];

    size_t read_buffer_avail = 0;
    size_t total_bytes_written = 0;

    size_t src_buf_offset = 0;
    size_t dst_buf_offset = 0;

    tinfl_decompressor decomp;      // 10992 bytes
    tinfl_init(&decomp);

    // update flag bits (chunked input & output)
    flags |= TINFL_FLAG_HAS_MORE_INPUT;
    flags &= ~TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF;

    while (1) {

        size_t src_buf_size = read_buffer_avail - src_buf_offset;
        size_t dst_buf_size = write_buffer_size - dst_buf_offset;
        tinfl_status status = tinfl_decompress(&decomp, read_buffer + src_buf_offset, &src_buf_size, write_buffer, write_buffer + dst_buf_offset, &dst_buf_size, flags);
        src_buf_offset += src_buf_size;

        if (status == TINFL_STATUS_NEEDS_MORE_INPUT) {
            read_buffer_avail = read_func(read_buffer, read_buffer_size, read_context);
            src_buf_offset = 0;
        }

        if ((dst_buf_size) && (!write_func(write_buffer + dst_buf_offset, dst_buf_size, write_context))) {
            break;
        }

        total_bytes_written += dst_buf_size;

        if (status <= TINFL_STATUS_DONE) {
            break;
        }

        dst_buf_offset = (dst_buf_offset + dst_buf_size) & (write_buffer_size - 1);
    }

    return total_bytes_written;
}


//
// GZIP decompressor
//
static int read_gzip_header(inflate_read_func_ptr read_func, void* const read_context)
{
    typedef struct {
        uint8_t sig[2];
        uint8_t cm;
        uint8_t flg;
        uint8_t mtime[4];
        uint8_t extra;
        uint8_t os;
    } gzip_header;
    enum gzip_flags {
        ftext = 1 << 0,
        fhcrc = 1 << 1,
        fextra = 1 << 2,
        fname = 1 << 3,
        fcomment = 1 << 4
    };

    uint8_t c;
    uint16_t crc;
    gzip_header hdr;
    size_t size = read_func(&hdr, sizeof(gzip_header), read_context);

    if (size != sizeof(gzip_header)) {
        WARNING("Unable to read GZIP header");
        return -1;
    }

    if (hdr.sig[0] != 0x1f || hdr.sig[1] != 0x8b) {
        WARNING("GZIP signature mismatch");
        return -1;
    }

    if (hdr.cm != 8) {
        WARNING("GZIP compression method is not DEFLATE");
        return -1;
    }

    if (hdr.flg & fextra) {
        uint16_t extra[2];
        read_func(&extra, sizeof(extra), read_context); // subfield id
        read_func(&extra, sizeof(extra), read_context); // length
        size_t skip = extra[0] | (extra[1] << 8);

        while (skip--) {
            read_func(&c, 1, read_context);
        }
    }

    if (hdr.flg & fname) {
        do {
            read_func(&c, sizeof(c), read_context);
        } while (c);
    }

    if (hdr.flg & fcomment) {
        do {
            read_func(&c, sizeof(c), read_context);
        } while (c);
    }

    if (hdr.flg & fhcrc) {
        read_func(&crc, sizeof(crc), read_context);
    }

    return 0;
}

size_t gunzip(inflate_read_func_ptr read_func, void* const read_context, inflate_write_func_ptr write_func, void* const write_context)
{
    if (read_gzip_header(read_func, read_context)) {
        return 0;
    }

    // NB - at this point we need ~45KB stack space (!!)
    return zlib_inflate(read_func, read_context, 1024, write_func, write_context, 0);
}

#endif

typedef struct _DecompressBufferContext {
    char*       buffer;
    uint32_t    remaining;
} tDecompressBufferContext;

#ifndef FPGA_DISABLE_EMBEDDED_CORE

static size_t read_embedded_buffer(void* buffer, size_t len, void* context)
{
    tDecompressBufferContext* read_context = (tDecompressBufferContext*)context;

    if (len > read_context->remaining) {
        len = read_context->remaining;
    }

    memcpy(buffer, read_context->buffer, len);
    read_context->buffer += len;
    read_context->remaining -= len;

    return len;
}
static size_t write_to_dram(const void* buffer, size_t len, void* context)
{
    uint32_t* write_offset = (uint32_t*)context;
    size_t written = 0;

    while (len != 0) {
        uint32_t size = len > 512 ? 512 : len;

        if (FileIO_MCh_BufToMem((void*)buffer, *write_offset, size) != 0) {
            WARNING("DRAM write failed!");
            return written;
        }

        written += size;
        *write_offset += size;
        len -= size;
        buffer = &((uint8_t*)buffer)[size];
    }

    return written;
}
#endif // FPGA_DISABLE_EMBEDDED_CORE

void FPGA_DecompressToDRAM(char* buffer, uint32_t size, uint32_t base)
{
#ifdef FPGA_DISABLE_EMBEDDED_CORE
    ERROR("Embedded core not available!");
#else
    tDecompressBufferContext read_context;
    read_context.buffer = buffer;
    read_context.remaining = size;
    uint32_t write_offset = base;
    size_t bytes = gunzip(read_embedded_buffer, &read_context, write_to_dram, &write_offset);
    DEBUG(1, "Decompressed %d bytes to %08x", bytes, base);
#endif
}

#ifndef FPGA_DISABLE_EMBEDDED_CORE
static size_t write_to_file(const void* buffer, size_t len, void* context)
{
    FF_FILE* file = (FF_FILE*)context;
    size_t written = 0;

    while (len != 0) {
        uint32_t size = FF_Write(file, 1, len, (FF_T_UINT8*)buffer);

        written += size;
        len -= size;
        buffer = &((uint8_t*)buffer)[size];
    }

    return written;
}
#endif

void FPGA_WriteEmbeddedToFile(FF_FILE* file)
{
#ifdef FPGA_DISABLE_EMBEDDED_CORE
    ERROR("Embedded core not available!");
#else
    uint32_t read_offset = 0;
    size_t size = gunzip(read_embedded_core, &read_offset, write_to_file, file);
    (void)size; // unused-variable warning/error
    Assert(size == BITSTREAM_LENGTH);
#endif
}
