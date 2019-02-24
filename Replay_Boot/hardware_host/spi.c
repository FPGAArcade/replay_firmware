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
#include "board.h"
#include "messaging.h"
#include "hardware/spi.h"
#include "hardware/timer.h"
#include "../osd.h"
#include <string.h>
#include "../fileio.h"

#include <ncurses.h>

// hdiutil create sdcard.dmg -volname "REPLAY" -fs FAT32 -size 256m -format UDRW -srcfolder ../../../hw/replay/cores/amiga_68060/sdcard/
// mv sdcard.dmg sdcard.bin

enum {
    SPI_SDCARD = 1 << 0,
    SPI_FILEIO = 1 << 1,
    SPI_OSD    = 1 << 2,
    SPI_DIRECT = 1 << 3
};
static uint8_t spi_enable = 0;
static uint32_t spi_osd_offset = 0;
static uint8_t spi_osd_buffer[4096];

static int spi_osd_keycode = ERR;
static uint8_t spi_osd_keybuf[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t spi_osd_keypos = 0;
static uint8_t spi_osd_keylen = 0;
static uint8_t spi_osd_ps2table[128] = {0};
static void ConvertToPS2(int ch);

static uint32_t spi_fio_offset = 0;
static uint8_t spi_fio_buffer[4096];
static uint32_t fio_address = 0;
static uint8_t fio_direction = 0;
static uint16_t fio_read_size = 0;

static uint8_t dram[64 * 1024 * 1024];

#define     CMD0        0x40        /*Resets the multimedia card*/
#define     CMD1        0x41        /*Activates the card's initialization process*/
#define     CMD2        0x42        /*--*/
#define     CMD3        0x43        /*--*/
#define     CMD4        0x44        /*--*/
#define     CMD5        0x45        /*reseved*/
#define     CMD6        0x46        /*reserved*/
#define     CMD7        0x47        /*--*/
#define     CMD8        0x48        /*reserved*/
#define     CMD9        0x49        /*CSD : Ask the selected card to send its card specific data*/
#define     CMD10       0x4a        /*CID : Ask the selected card to send its card identification*/
#define     CMD11       0x4b        /*--*/
#define     CMD12       0x4c        /*--*/
#define     CMD13       0x4d        /*Ask the selected card to send its status register*/
#define     CMD14       0x4e        /*--*/
#define     CMD15       0x4f        /*--*/
#define     CMD16       0x50        /*Select a block length (in bytes) for all following block commands (Read:between 1-512 and Write:only 512)*/
#define     CMD17       0x51        /*Reads a block of the size selected by the SET_BLOCKLEN command, the start address and block length must be set so that the data transferred will not cross a physical block boundry*/
#define     CMD18       0x52        /*--*/
#define     CMD19       0x53        /*reserved*/
#define     CMD20       0x54        /*--*/
#define     CMD21       0x55        /*reserved*/
#define     CMD22       0x56        /*reserved*/
#define     CMD23       0x57        /*reserved*/
#define     CMD24       0x58        /*Writes a block of the size selected by CMD16, the start address must be alligned on a sector boundry, the block length is always 512 bytes*/
#define     CMD25       0x59        /*--*/
#define     CMD26       0x5a        /*--*/
#define     CMD27       0x5b        /*Programming of the programmable bits of the CSD*/
#define     CMD28       0x5c        /*If the card has write protection features, this command sets the write protection bit of the addressed group. The porperties of the write protection are coded in the card specific data (WP_GRP_SIZE)*/
#define     CMD29       0x5d        /*If the card has write protection features, this command clears the write protection bit of the addressed group*/
#define     CMD30       0x5e        /*If the card has write protection features, this command asks the card to send the status of the write protection bits. 32 write protection bits (representing 32 write protect groups starting at the specific address) followed by 16 CRD bits are transferred in a payload format via the data line*/
#define     CMD31       0x5f        /*reserved*/
#define     CMD32       0x60        /*sets the address of the first sector of the erase group*/
#define     CMD33       0x61        /*Sets the address of the last sector in a cont. range within the selected erase group, or the address of a single sector to be selected for erase*/
#define     CMD34       0x62        /*Removes on previously selected sector from the erase selection*/
#define     CMD35       0x63        /*Sets the address of the first erase group within a range to be selected for erase*/
#define     CMD36       0x64        /*Sets the address of the last erase group within a continuos range to be selected for erase*/
#define     CMD37       0x65        /*Removes one previously selected erase group from the erase selection*/
#define     CMD38       0x66        /*Erases all previously selected sectors*/
#define     CMD39       0x67        /*--*/
#define     CMD40       0x68        /*--*/
#define     CMD41       0x69        /*reserved*/
#define     CMD42       0x6a        /*reserved*/
#define     CMD43       0x6b        /*reserved*/
#define     CMD44       0x6c        /*reserved*/
#define     CMD45       0x6d        /*reserved*/
#define     CMD46       0x6e        /*reserved*/
#define     CMD47       0x6f        /*reserved*/
#define     CMD48       0x70        /*reserved*/
#define     CMD49       0x71        /*reserved*/
#define     CMD50       0x72        /*reserved*/
#define     CMD51       0x73        /*reserved*/
#define     CMD52       0x74        /*reserved*/
#define     CMD53       0x75        /*reserved*/
#define     CMD54       0x76        /*reserved*/
#define     CMD55       0x77        /*reserved*/
#define     CMD56       0x78        /*reserved*/
#define     CMD57       0x79        /*reserved*/
#define     CMD58       0x7a        /*reserved*/
#define     CMD59       0x7b        /*Turns the CRC option ON or OFF. A '1' in the CRC option bit will turn the option ON, a '0' will turn it OFF*/
#define     CMD60       0x7c        /*--*/
#define     CMD61       0x7d        /*--*/
#define     CMD62       0x7e        /*--*/
#define     CMD63       0x7f        /*--*/

WINDOW* win;
//
// SPI
//
void SPI_Init(void)
{
    printf("%s\n", __FUNCTION__);

    initscr();
    raw();
    noecho();
    cbreak();

    int x, y;
    int w = OSDLINELEN + 2;
    int h = OSDNLINE + 2;
    getmaxyx(stdscr, y, x);
    int x0 = (x - w) / 2;
    int y0 = (y - h) / 2;
    start_color();
    short basic_colors[] = { COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN, COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE };

    for (int b = 0; b < 8; ++b) {
        for (int f = 0; f < 8; ++f) {
            init_pair(b * 8 + f + 1, basic_colors[f], basic_colors[b]);
        }
    }

    win = newwin(h, w, y0, x0);
    box(win, 0, 0);
    curs_set(0);
    keypad(win, TRUE);
    nodelay(win, TRUE);
    wrefresh(win);

    for (int i = 0; i < 128; ++i) {
        spi_osd_ps2table[OSD_ConvASCII(i)] = i;
    }

    spi_osd_ps2table[0x0a] = 0x5a;
    spi_osd_ps2table[0x20] = 0x29;
    spi_osd_ps2table[0x7f] = 0x66;
}

struct _sdc_cmd {
    union {
        struct {
            uint8_t command;
            uint8_t arg0;
            uint8_t arg1;
            uint8_t arg2;
            uint8_t arg3;
            uint8_t crc;
        };

        uint8_t buffer[6];
    };
} sdc_cmd;
uint8_t sdc_result_length = 0;
uint8_t sdc_result[5] = {0};
uint32_t sdc_read_sector = 0;
uint32_t sdc_data_length = 0;
uint8_t sdc_data[1 + 512 + 2] = {0};
uint8_t* sdc_data_ptr = 0;

uint8_t last_command = 0;

enum {
    SPI_IDLE        = 1 << 0,
    SPI_ERASE_RST   = 1 << 1,
    SPI_ILLEGAL     = 1 << 2,
    SPI_CRC         = 1 << 3,
    SPI_ERASE_SEQ   = 1 << 4,
    SPI_ADDRESS     = 1 << 5,
    SPI_PARAM       = 1 << 6,
};

uint8_t osd_command = 0xff;
uint8_t osd_param = 0xff;

unsigned char rSPI(unsigned char outByte)
{
    uint8_t v = 0;

    if (spi_enable & SPI_SDCARD) {
        sdc_cmd.buffer[0] = sdc_cmd.buffer[1];
        sdc_cmd.buffer[1] = sdc_cmd.buffer[2];
        sdc_cmd.buffer[2] = sdc_cmd.buffer[3];
        sdc_cmd.buffer[3] = sdc_cmd.buffer[4];
        sdc_cmd.buffer[4] = sdc_cmd.buffer[5];
        sdc_cmd.buffer[5] = outByte;

        switch (sdc_cmd.command) {
            case CMD0:
                printf("CMD0 [%02x,%02x,%02x,%02x] CRC = %02x\n", sdc_cmd.arg0, sdc_cmd.arg1, sdc_cmd.arg2, sdc_cmd.arg3, sdc_cmd.crc);
                last_command = sdc_cmd.command;
                sdc_result_length = 1;
                sdc_result[0] = SPI_IDLE;
                memset(sdc_cmd.buffer, 0x00, sizeof(sdc_cmd.buffer));
                break;

            case CMD12:
                printf("CMD12 [%02x,%02x,%02x,%02x] CRC = %02x\n", sdc_cmd.arg0, sdc_cmd.arg1, sdc_cmd.arg2, sdc_cmd.arg3, sdc_cmd.crc);
                last_command = sdc_cmd.command;
                sdc_result_length = 1;
                sdc_result[0] = 0x00;
                memset(sdc_cmd.buffer, 0x00, sizeof(sdc_cmd.buffer));
                break;

            case CMD8:
                printf("CMD8 [%02x,%02x,%02x,%02x] CRC = %02x\n", sdc_cmd.arg0, sdc_cmd.arg1, sdc_cmd.arg2, sdc_cmd.arg3, sdc_cmd.crc);
                last_command = sdc_cmd.command;
                sdc_result_length = 5;
                sdc_result[0] = 0xaa;
                sdc_result[1] = 0x01;
                sdc_result[2] = 0x00;
                sdc_result[3] = 0x00;
                sdc_result[4] = SPI_IDLE;
                memset(sdc_cmd.buffer, 0x00, sizeof(sdc_cmd.buffer));
                break;

            case CMD17: {
                printf("CMD17 [%02x,%02x,%02x,%02x] CRC = %02x\n", sdc_cmd.arg0, sdc_cmd.arg1, sdc_cmd.arg2, sdc_cmd.arg3, sdc_cmd.crc);
                last_command = sdc_cmd.command;
                sdc_read_sector = 0;
                sdc_read_sector |= sdc_cmd.arg0;
                sdc_read_sector <<= 8;
                sdc_read_sector |= sdc_cmd.arg1;
                sdc_read_sector <<= 8;
                sdc_read_sector |= sdc_cmd.arg2;
                sdc_read_sector <<= 8;
                sdc_read_sector |= sdc_cmd.arg3;
                printf("read_sector = $%x\n", sdc_read_sector);
                sdc_result_length = 1;
                sdc_result[0] = 0x00;
                memset(sdc_cmd.buffer, 0x00, sizeof(sdc_cmd.buffer));

                sdc_data_length = 512 + 1 + 2;
                sdc_data_ptr = sdc_data;
                sdc_data[0] = 0xfe;

                FILE* f = fopen("sdcard.bin", "rb");

                if (f) {
                    fseek(f, sdc_read_sector * 512, SEEK_SET);
                    fread(&sdc_data[1], 1, 512, f);
                    fclose(f);
                }

                break;
            }

            case CMD18: {
                printf("CMD18 [%02x,%02x,%02x,%02x] CRC = %02x\n", sdc_cmd.arg0, sdc_cmd.arg1, sdc_cmd.arg2, sdc_cmd.arg3, sdc_cmd.crc);
                last_command = sdc_cmd.command;
                sdc_read_sector = 0;
                sdc_read_sector |= sdc_cmd.arg0;
                sdc_read_sector <<= 8;
                sdc_read_sector |= sdc_cmd.arg1;
                sdc_read_sector <<= 8;
                sdc_read_sector |= sdc_cmd.arg2;
                sdc_read_sector <<= 8;
                sdc_read_sector |= sdc_cmd.arg3;
                printf("read_sector = $%x\n", sdc_read_sector);
                sdc_result_length = 1;
                sdc_result[0] = 0x00;
                memset(sdc_cmd.buffer, 0x00, sizeof(sdc_cmd.buffer));

                sdc_data_length = 512 + 1 + 2;
                sdc_data_ptr = sdc_data;
                sdc_data[0] = 0xfe;

                FILE* f1 = fopen("sdcard.bin", "rb");

                if (f1) {
                    fseek(f1, sdc_read_sector * 512, SEEK_SET);
                    fread(&sdc_data[1], 1, 512, f1);
                    fclose(f1);
                }

                break;
            }

            case CMD41:
                printf("CMD41 [%02x,%02x,%02x,%02x] CRC = %02x\n", sdc_cmd.arg0, sdc_cmd.arg1, sdc_cmd.arg2, sdc_cmd.arg3, sdc_cmd.crc);
                last_command = sdc_cmd.command;
                sdc_result_length = 1;
                sdc_result[0] = 0;
                memset(sdc_cmd.buffer, 0x00, sizeof(sdc_cmd.buffer));
                break;

            case CMD55:
                printf("CMD55 [%02x,%02x,%02x,%02x] CRC = %02x\n", sdc_cmd.arg0, sdc_cmd.arg1, sdc_cmd.arg2, sdc_cmd.arg3, sdc_cmd.crc);
                last_command = sdc_cmd.command;
                sdc_result_length = 1;
                sdc_result[0] = SPI_IDLE;
                memset(sdc_cmd.buffer, 0x00, sizeof(sdc_cmd.buffer));
                break;

            case CMD58:
                printf("CMD58 [%02x,%02x,%02x,%02x] CRC = %02x\n", sdc_cmd.arg0, sdc_cmd.arg1, sdc_cmd.arg2, sdc_cmd.arg3, sdc_cmd.crc);
                last_command = sdc_cmd.command;
                sdc_result_length = 5;
                sdc_result[0] = 0x00;
                sdc_result[1] = 0x00;
                sdc_result[2] = 0x00;
                sdc_result[3] = 0x40;
                sdc_result[4] = 0x00;
                memset(sdc_cmd.buffer, 0x00, sizeof(sdc_cmd.buffer));
                break;

            default:
                printf("UNKNOWN SPI CMD %02x [%02x,%02x,%02x,%02x] CRC = %02x\n", sdc_cmd.command, sdc_cmd.arg0, sdc_cmd.arg1, sdc_cmd.arg2, sdc_cmd.arg3, sdc_cmd.crc);
                last_command = sdc_cmd.command;
                sdc_result_length = 1;
                sdc_result[0] = SPI_PARAM;
                memset(sdc_cmd.buffer, 0x00, sizeof(sdc_cmd.buffer));
                break;

            case 0x00:
            case 0xff:
                if (sdc_data_length && !sdc_result_length) {
                    v = *sdc_data_ptr++;
                    --sdc_data_length;

                    if (sdc_data_length == 0 && last_command == CMD18) {
                        printf("ANOTHER SECTOR\n");
                        sdc_read_sector++;
                        sdc_data_length = 512 + 1 + 2;
                        sdc_data_ptr = sdc_data;
                        sdc_data[0] = 0xfe;

                        FILE* f = fopen("sdcard.bin", "rb");

                        if (f) {
                            fseek(f, sdc_read_sector * 512, SEEK_SET);
                            fread(&sdc_data[1], 1, 512, f);
                            fclose(f);
                        }

                    }

                    break;

                }

                v = (sdc_result_length) ? sdc_result[--sdc_result_length] : 0xff;
                break;

        }

    } else if (spi_enable & SPI_OSD) {
        if (spi_osd_offset < sizeof(spi_osd_buffer)) {
            spi_osd_buffer[spi_osd_offset++] = outByte;

        } else {
            fprintf(stderr, "OUT OF OSD SPACE!\n");
        }

        uint8_t cmd = spi_osd_buffer[0] & 0xf0;
        uint32_t size = spi_osd_offset;

        uint8_t param = spi_osd_buffer[0] & 0x0f;

        switch (cmd) {
            case OSDCMD_READSTAT:
                if (size < (1 + sizeof(uint8_t))) {
                    break;
                }

                printf("OSDCMD_READSTAT %01x\n", param);

                switch (param) {
                    case 1: // ReadSysconVer
                        v = 0xa5;
                        break;

                    case 2: // ReadVer (low)
                        v = 0x12;
                        break;

                    case 3: // ReadVer (high)
                        v = 0x34;
                        break;

                    case 4: // ReadFileIO (enabled)
                        v = 0x3f;
                        break;

                    case 5: // ReadFileIO (driver)
                        v = 0x81;
                        break;

                    case 6: // ReadStatus (low)
                        v = 0x12;
                        break;

                    case 7: // ReadStatus (high)
                        v = 0x34;
                        break;

                    case 0:
                        v = spi_osd_keypos < spi_osd_keylen ? STF_NEWKEY : 0;

                        if (!v) {
                            int ch = wgetch(win);

                            if (spi_osd_keycode != ch) {
                                ConvertToPS2(ch);
                                spi_osd_keycode = ch;
                            }
                        }

                        break;

                    case 8: // ReadKeyboard
                        v = spi_osd_keypos < spi_osd_keylen ? spi_osd_keybuf[spi_osd_keypos++] : 0;
                        break;
                }

                spi_osd_offset = 0;
                break;

            case OSDCMD_CTRL:
                printf("OSDCMD_CTRL %01x (reset)\n", param);
                spi_osd_offset = 0;
                break;

            case OSDCMD_CONFIG:
                switch (param) {
                    case 0: // static
                    case 1: // dynamic
                        if (size < (1 + sizeof(uint32_t))) {
                            break;
                        }

                        printf("OSDCMD_CONFIG static/dynamic %01x\n", param);
                        spi_osd_offset = 0;
                        break;

                    case 3: // ctrl
                        if (size < (1 + sizeof(uint16_t))) {
                            break;
                        }

                        printf("OSDCMD_CONFIG ctrl %01x\n", param);
                        spi_osd_offset = 0;
                        break;

                    case 8: // fileio (cha)
                    case 9: // fileio (chb)
                        if (size < (1 + sizeof(uint8_t))) {
                            break;
                        }

                        printf("OSDCMD_CONFIG fileio %01x\n", param);
                        spi_osd_offset = 0;
                        break;
                }

                break;

            case OSDCMD_SENDPS2:
                if (size < (1 + sizeof(uint8_t))) {
                    break;
                }

                printf("OSDCMD_SENDPS2 %01x\n", param);
                spi_osd_offset = 0;
                break;

            case OSDCMD_DISABLE:
                printf("OSDCMD_ENABLE/DISABLE %01x\n", param);
                spi_osd_offset = 0;
                break;

            case OSDCMD_SETHOFF:
                if (param == 0) { // OSDCMD_SETHOFF
                    if (size < (1 + sizeof(uint16_t))) {
                        break;
                    }

                    printf("OSDCMD_SETHOFF %01x\n", param);
                    osd_command = OSDCMD_SETHOFF;
                    break;

                } else if (param == 1) { // OSDCMD_SETVOFF
                    if (size < (1 + sizeof(uint8_t))) {
                        break;
                    }

                    printf("OSDCMD_SETVOFF %01x\n", param);
                    osd_command = OSDCMD_SETVOFF;
                    break;

                }

            case OSDCMD_WRITE:
                if (size > 1) {
                    break;
                }

                printf("OSDCMD_WRITE %01x\n", param);
                osd_command = OSDCMD_WRITE;
                osd_param = param;
                break;

            default:
                fprintf(stderr, "UNKNOWN OSD SPI COMMAND! %01x\n", cmd);
                {
                    int* p = 0;
                    *p = 3;
                }
                break;

        }

    } else if (spi_enable & SPI_FILEIO) {
        if (spi_fio_offset < sizeof(spi_fio_buffer)) {
            spi_fio_buffer[spi_fio_offset++] = outByte;

        } else {
            fprintf(stderr, "OUT OF FIO SPACE (%02x)!\n", spi_fio_buffer[0]);
        }

        uint8_t cmd = spi_fio_buffer[0] & 0xf0;
        uint32_t size = spi_fio_offset;

        uint8_t param = spi_fio_buffer[0] & 0x0f;
        printf("FILEIO %02x / %02x\n", cmd, param);

        switch (cmd) {
            case 0x80:  // set address / direction
                if (param == 0) {   // set address
                    if (size < 1 + sizeof(uint32_t)) {
                        break;
                    }

                    fio_address = 0;
                    fio_address |= spi_fio_buffer[4];
                    fio_address <<= 8;
                    fio_address |= spi_fio_buffer[3];
                    fio_address <<= 8;
                    fio_address |= spi_fio_buffer[2];
                    fio_address <<= 8;
                    fio_address |= spi_fio_buffer[1];
                    printf("FILEIO_ADDRESS %08x\n", fio_address);
                    spi_fio_offset = 0;

                } else if (param == 1) { // set direction
                    if (size < 1 + sizeof(uint8_t)) {
                        break;
                    }

                    fio_direction = spi_fio_buffer[1];
                    printf("FILEIO_DIRECTION %s\n", fio_direction ? "READ" : "WRITE");
                    spi_fio_offset = 0;

                } else if (param == 4) { // set read size
                    if (size < 1 + sizeof(uint16_t)) {
                        break;
                    }

                    fio_read_size = 0;
                    fio_read_size |= spi_fio_buffer[2];
                    fio_read_size <<= 8;
                    fio_read_size |= spi_fio_buffer[1];
                    printf("FILEIO_READ_SIZE %04x\n", fio_read_size);
                    spi_fio_offset = 0;

                } else if (param == 7) { // get status
                    if (size < 1 + sizeof(uint8_t)) {
                        break;
                    }

                    v = 0;
                    printf("FILEIO_GET_STATUS\n");
                    spi_fio_offset = 0;
                }

                break;

            case 0xa0:  // data
            case 0xb0:  // data
                printf("FILEIO_READ/WRITE\n");
                spi_fio_offset = 0;
                break;

            // channel A
            case FILEIO_FCH_CMD_STAT_R:
            case FILEIO_FCH_CMD_CMD_W:
            case FILEIO_FCH_CMD_FIFO_R:
            case FILEIO_FCH_CMD_FIFO_W:

            // channel B
            case 0x40+FILEIO_FCH_CMD_STAT_R:
            case 0x40+FILEIO_FCH_CMD_CMD_W:
            case 0x40+FILEIO_FCH_CMD_FIFO_R:
            case 0x40+FILEIO_FCH_CMD_FIFO_W:
                printf("FILEIO_FCH_CMD unhandled\n");
                spi_fio_offset = 0;
                break;

            default:
                fprintf(stderr, "UNKNOWN FILEIO SPI COMMAND! %01x\n", cmd);
                {
                    int* p = 0;
                    *p = 3;
                }
                break;

        }


    } else {
        v = (outByte == 0xff) ? 0xff : 0x00;
    }

    //    printf("%s %02x => %02x\n", __FUNCTION__, outByte, v);
    return v;
}

void SPI_WriteBufferSingle(void* pBuffer, uint32_t length)
{
    printf("%s %p %08x -> %08x\n", __FUNCTION__, pBuffer, length, fio_address);
    memcpy(&dram[fio_address], pBuffer, length);
    fio_address += length;
}

void SPI_ReadBufferSingle(void* pBuffer, uint32_t length)
{
    printf("%s %p %08x <- %08x\n", __FUNCTION__, pBuffer, length, fio_address);
    memcpy(pBuffer, &dram[fio_address], length);
    fio_address += length;
}

void SPI_Wait4XferEnd(void)
{
    printf("%s\n", __FUNCTION__);
}

void SPI_EnableCard(void)
{
    printf("%s\n", __FUNCTION__);
    spi_enable |= SPI_SDCARD;
}

void SPI_DisableCard(void)
{
    printf("%s\n", __FUNCTION__);
    spi_enable &= ~SPI_SDCARD;
}

void SPI_EnableFileIO(void)
{
    printf("%s\n", __FUNCTION__);
    spi_enable |= SPI_FILEIO;
}

void SPI_DisableFileIO(void)
{
    printf("%s\n", __FUNCTION__);
    spi_enable &= ~SPI_FILEIO;
}

void SPI_EnableOsd(void)
{
    printf("%s\n", __FUNCTION__);
    spi_enable |= SPI_OSD;
    spi_osd_offset = 0;
}

void SPI_DisableOsd(void)
{
    printf("%s\n", __FUNCTION__);
    spi_enable &= ~SPI_OSD;

    if (spi_osd_offset == 0) {
        return;
    }

    if (osd_command == 0xff) {
        printf("ERROR no osd command for dangling data\n");
        return;
    }

    if (osd_command == OSDCMD_WRITE && spi_osd_offset > 1) {
        uint8_t row = osd_param;
        uint8_t col = spi_osd_buffer[1] >= OSDLINELEN ? spi_osd_buffer[1] - OSDLINELEN : spi_osd_buffer[1];
        //        uint8_t page = spi_osd_buffer[1] >= OSDLINELEN ? 1 : 0;
        char buffer[OSDLINELEN * 2];
        uint8_t attrib[OSDLINELEN * 2];

        if ((spi_osd_offset & 0x1) != 0) {
            printf("ERROR missing attrib %i\n", spi_osd_offset);
            return;
        }

        int num_chars = (spi_osd_offset - 2) / 2;

        if (col + num_chars > OSDLINELEN) {
            num_chars = OSDLINELEN - col;
        }

        wmove(win, row + 1, col + 1);

        for (int i = 0; i < num_chars; ++i) {
            uint8_t c = spi_osd_buffer[i * 2 + 2];
            uint8_t a = spi_osd_buffer[i * 2 + 3];
            buffer[i] = c;
            attrib[i] = a;
            short f = a & 0xf;
            short b = (a & 0xf0) >> 4;
            uint8_t bold = (f & 0x8);
            uint8_t dim = (b & 0x8);
            uint8_t invert = (b == 4);
            f &= 7;
            b &= 7;

            if (invert) {
                b = 0, f = 4;
            }

            int extras = (bold ? A_BOLD : 0) | (dim ? A_DIM : 0) | (invert ? A_REVERSE : 0);
            (void)extras;
            waddch(win, c | COLOR_PAIR(b * 8 + f + 1) | extras);
        }

        buffer[(spi_osd_offset - 2) / 2] = 0;
        printf("OSDTXT: %i/%i : (%i) %s\n", row, col, num_chars, buffer);
        wrefresh(win);

    }


    osd_command = osd_param = 0xff;
}

void SPI_EnableDirect(void)
{
    printf("%s\n", __FUNCTION__);
    spi_enable |= SPI_DIRECT;
}

void SPI_DisableDirect(void)
{
    printf("%s\n", __FUNCTION__);
    spi_enable &= ~SPI_DIRECT;
}

unsigned char SPI_IsActive(void)
{
    printf("%s\n", __FUNCTION__);
    return 0;
}

static const uint8_t Pos_KEY[] = {0x72, 0x75, 0x6B, 0x74, 0x7D};
static const uint8_t Fx_KEY[] = {0x05, 0x06, 0x04, 0x0C, 0x03, 0x0B, 0x83, 0x0A, 0x01, 0x09, 0x78, 0x07};

void ConvertToPS2(int ch)
{
    uint8_t make = ch != ERR;

    if (!make) {
        ch = spi_osd_keycode;
    }

    spi_osd_keypos = spi_osd_keylen = 0;

    if ('a' <= ch && ch <= 'z') {
        ch &= ~0x20;
    }

    if (ch < 128) {
        unsigned char c = spi_osd_ps2table[ch];

        if (c == 0) {
            fprintf (stderr, "no code for %02x => %02x\n", ch, c);

            return;
        }

        if (!make) {
            spi_osd_keybuf[spi_osd_keylen++] = 0xf0;    // break
        }

        spi_osd_keybuf[spi_osd_keylen++] = c;
        return;
    }

    if (KEY_F(1) <= ch && ch <= KEY_F(12)) {
        if (!make) {
            spi_osd_keybuf[spi_osd_keylen++] = 0xf0;    // break
        }

        spi_osd_keybuf[spi_osd_keylen++] = Fx_KEY[ch - KEY_F(1)];
        return;
    }

    if (KEY_DOWN <= ch && ch <= KEY_HOME) {
        spi_osd_keybuf[spi_osd_keylen++] = 0xe0;        // extended

        if (!make) {
            spi_osd_keybuf[spi_osd_keylen++] = 0xf0;    // break
        }

        spi_osd_keybuf[spi_osd_keylen++] = Pos_KEY[ch - KEY_DOWN];
    }
}
