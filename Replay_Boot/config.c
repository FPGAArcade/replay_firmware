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
/** @file config.c */

#include "config.h"

#include "fileio.h"
#include "fpga.h"
#include "osd.h"
#include "card.h"
#include "hardware/io.h"
#include "hardware/timer.h"
#include "twi.h"
#include "fileio.h"
#include "menu.h"

#include "messaging.h"

#include <sys/unistd.h> // sbrk()
#include <malloc.h> // mallinfo()

extern char __FIRST_IN_RAM[];
extern char end[];
extern char __TOP_STACK[];

/** @brief extern link to sdcard i/o manager

  Having this here is "ugly", but ff_ioman declares this "prototype"
  in the header which is actually code and should be in the .c file.
  Won't fix for now to avoid issues with the fullfat stuff....

  Usually it should be part of the module header where it belongs to,
  not at all in this .h/.c file...
*/
extern FF_IOMAN* pIoman;

extern const char* version;  // temp

// y0 - FPGA DRAM
// y1 - Coder
// y2 - FPGA aux/sys clk
// y3 - Expansion Main
// y4 - FPGA video
// y5 - Expansion Small

//  input clock = 27MHz.
//  output = (27 * n / m) / p. example m=375 n=2048 p=12 output = 12.288MHz. VCO = 147.456 MHz
//
const vidbuf_t init_vidbuf = {0, 0, 3};

const clockconfig_t init_clock_config_pal = {
    481, 4044,  // pll1 m,n = 227MHz
    46,  423,   // pll2 m,n = 248MHz
    225, 2048,  // pll3 m,n = 245.76 // for 49.152 audio
    { 1,  2,  4,  0,  0,  0, }, // p0,1,2,3,4,5
    { 2,  14, 5,  1,  1,  1, }, // d0,1,2,3,4,5
    { 1,  1,  1,  0,  1,  0  }  // y0,1,2,3,4,5
    // 113  17  49   0   27  0
};

const clockconfig_t init_clock_config_ntsc = {
    33,  280,   // pll1 m,n = 229MHz
    27,  133,   // pll2 m,n = 133MHz
    225, 2048,  // pll3 m,n = 245.76 // for 49.152 audio
    { 1,  1,  4,  0,  0,  0, }, // p0,1,2,3,4,5
    { 2,  16, 5,  1,  1,  1, }, // d0,1,2,3,4,5
    { 1,  1,  1,  0,  1,  0  }  // y0,1,2,3,4,5
};
// 114  14  49   0   27  0

const clockconfig_t init_clock_config_hd74 = {
    33,  280,   // pll1 m,n = 229MHz
    2,   11,   // pll2 m,n = 148.5MHz
    225, 2048,  // pll3 m,n = 245.76 // for 49.152 audio
    { 1,  0,  4,  0,  2,  0, }, // p0,1,2,3,4,5
    { 2,  0,  5,  1,  2,  1, }, // d0,1,2,3,4,5
    { 1,  0,  1,  0,  1,  0  }  // y0,1,2,3,4,5
};
// 57  14  49   0   74.25  0

// VIDEO ENCODER CONFIGURATION
// ===========================
//
// Has to be set via configured FPGA
//

// fstep 50-80ps  65+/-15ps
// 27MHz    37.0ns 1/4 cycle ~ 9200 ps = 141 steps
// 74.25MHz 13.5ns 1/4 cycle ~ 3300 ps =  51 steps

const vidconfig_t   default_vid_config_sd = {
    0x00, // reg 0x1C CM   Clock Mode Register
    0x48, // reg 0x1D IC   Input Clock Register
    0xC0, // reg 0x1E GPIO
    0x80, // reg 0x1F IDF  Input Data Format
    0x00, // reg 0x20 CD   Connection Detect Register
    0x01, // reg 0x21 DC   DAC control
    0x00, // reg 0x23 HPF  Hot plug detection
    0x80, // reg 0x31 TCTL DVI Control Input Register
    0x08, // reg 0x33 TPCP DVI PLL Charge Pump
    0x16, // reg 0x34 TPD  DVI PLL Divider
    0x30, // reg 0x35 TPVT DVI PLL Supply Control
    0x60, // reg 0x36 TPF  DVI PLL Filter
    0x00, // reg 0x37 TCT  DVI Clock Test
    0x18, // reg 0x48 TSTP Test Pattern
    0xC0, // reg 0x49 PM   Power Management  /00 turn dvi off
    0x00  // reg 0x56 DSP  Sync Polarity
};

const vidconfig_t   default_vid_config_hd = { // >65MHz
    0x00, // reg 0x1C CM   Clock Mode Register
    0x48, // reg 0x1D IC   Input Clock Register
    0xC0, // reg 0x1E GPIO
    0x80, // reg 0x1F IDF  Input Data Format
    0x00, // reg 0x20 CD   Connection Detect Register
    0x01, // reg 0x21 DC   DAC control
    0x00, // reg 0x23 HPF  Hot plug detection
    0x80, // reg 0x31 TCTL DVI Control Input Register
    0x06, // reg 0x33 TPCP DVI PLL Charge Pump
    0x26, // reg 0x34 TPD  DVI PLL Divider
    0x30, // reg 0x35 TPVT DVI PLL Supply Control
    0xA0, // reg 0x36 TPF  DVI PLL Filter
    0x00, // reg 0x37 TCT  DVI Clock Test
    0x18, // reg 0x48 TSTP Test Pattern
    0xC0, // reg 0x49 PM   Power Management  /00 turn dvi off
    0x00  // reg 0x56 DSP  Sync Polarity
};


// we copy the actual bootloader from flash to the RAM and call it
// (we assume there is no collision with the actual stack)
void CFG_call_bootloader(void)
{
    int i;

    volatile uint32_t* src = (volatile uint32_t*)0x200;
    volatile uint32_t* dest = (volatile uint32_t*)0x00200000;
    volatile uint16_t* patch = (volatile uint16_t*)0x2001d2;
    volatile uint16_t* delay = (volatile uint16_t*)0x20027c;

    // set PROG low to reset FPGA (open drain)
    IO_DriveLow_OD(PIN_FPGA_PROG_L);
    Timer_Wait(1);
    IO_DriveHigh_OD(PIN_FPGA_PROG_L);
    Timer_Wait(2);

    // copy bootloader to SRAM
    for (i = 0; i < 1024; i++) {
        *dest++ = *src++;
    }

    // remove the GPIO button check for faster flashing iterations
    if (*patch == 0xd156) { // bne.n  200282 <Bootrom+0x116> ("goto run_flash")
        *patch = 0x46c0;    // nop
    }

    // set an extended timeout - when debugging firmware flashing over USB
    if (*delay == 0x2b0a && 0) {
        *delay = 0x2b1a;
    }

    // launch bootloader in SRAM
    asm("ldr r3, = 0x00200000\n");
    asm("bx  r3\n");
}

void CFG_update_status(status_t* current_status)
{
    current_status->coder_fitted       = IO_Input_L(PIN_CODER_FITTED_L);
    current_status->card_detected      = IO_Input_L(PIN_CARD_DETECT);
    current_status->card_write_protect = IO_Input_H(PIN_WRITE_PROTECT);
}

void CFG_print_status(status_t* current_status)
{

    if (current_status->coder_fitted) {
        DEBUG(1, "CODER: Fitted.");

    } else {
        DEBUG(1, "CODER: Not fitted.");
    }

    if (current_status->card_detected) {
        DEBUG(1, "Memory card detected.");

        if (current_status->card_write_protect) {
            DEBUG(1, "Memory card is write protected.");

        } else {
            DEBUG(1, "Memory card is not write protected.");
        }

    } else {
        DEBUG(1, "No memory card detected.");
    }

}

void CFG_card_start(status_t* current_status)
{
    if (current_status->card_detected) {
        if ( (current_status->card_init_ok && current_status->fs_mounted_ok) || current_status->usb_mounted)
            // assume all good
        {
            return;
        }

        if (Card_Init()) { // true is non zero
            DEBUG(1, "Card Init OK.");
            current_status->card_init_ok = TRUE;

        } else {
            ERROR("SDCARD:Card Init FAIL.");
            current_status->card_init_ok = FALSE;
            current_status->fs_mounted_ok = FALSE;
            return;
        }

        current_status->fs_mounted_ok = FALSE;
        for(uint8_t mtr=0; mtr < 3 && !current_status->fs_mounted_ok; mtr++) {
          if(!FF_MountPartition(pIoman, 0)) {
            current_status->fs_mounted_ok = TRUE;
          } else {
            Timer_Wait(20);
          }
        }
        if(current_status->fs_mounted_ok) {
            DEBUG(1, "Partition mounted ok.");
        } else {
            ERROR("SDCARD:Could not mount partition.");
            return;
        }          
        switch (pIoman->pPartition->Type) {
            case FF_T_FAT32:
                MSG_info("SDCARD: FAT32 formatted");
                break;

            //DEBUG(1,"FAT32 Formatted Drive"); break;
            case FF_T_FAT16:
                MSG_info("SDCARD: FAT16 formatted");
                break;

            //DEBUG(1,"FAT16 Formatted Drive"); break;
            case FF_T_FAT12:
                MSG_info("SDCARD: FAT12 formatted");
                break;
                //DEBUG(1,"FAT12 Formatted Drive"); break;
        }

        //DEBUG(1,"");
        //DEBUG(1,"Block Size: %d",     pIoman->pPartition->BlkSize);
        //DEBUG(1,"Cluster Size: %d kB", (pIoman->pPartition->BlkSize *
        //        pIoman->pPartition->SectorsPerCluster) / 1024);
        //DEBUG(1,"Volume Size: %lu MB", FF_GetVolumeSize(pIoman));
        //DEBUG(1,"");

        MSG_info("SDCARD: %dB/%dkB/%luMB",
                 pIoman->pPartition->BlkSize,
                 (pIoman->pPartition->BlkSize *
                  pIoman->pPartition->SectorsPerCluster) >> 10,
                 FF_GetVolumeSize(pIoman)
                );

    }  else { // no card, maybe removed
        current_status->card_init_ok = FALSE;
        current_status->fs_mounted_ok = FALSE;
        // set SPI clock to 24MHz, perhaps the debugger is being used
        AT91C_BASE_SPI->SPI_CSR[0] = AT91C_SPI_CPOL | (2 << 8);
    }
}

void CFG_set_coder(coder_t standard)
{
    // Y1 -> 10 - LT_PN  - 18pf
    // Y2 -> 9 - LT_N - 9pf
    // high adds cap
    switch (standard) {
        case CODER_DISABLE :
            DEBUG(2, "CODER: Disabled.");
            AT91C_BASE_PIOA->PIO_CODR  = PIN_CODER_NTSC_H | PIN_CODER_CE;
            AT91C_BASE_PIOA->PIO_CODR  = PIN_DTXD_Y1      | PIN_DRXD_Y2;
            break;

        case CODER_PAL:
            DEBUG(2, "CODER: PAL Y trap.");
            AT91C_BASE_PIOA->PIO_SODR  = PIN_CODER_CE;
            AT91C_BASE_PIOA->PIO_CODR  = PIN_CODER_NTSC_H;
            AT91C_BASE_PIOA->PIO_SODR  = PIN_DTXD_Y1;
            AT91C_BASE_PIOA->PIO_CODR  = PIN_DRXD_Y2;
            break;

        case CODER_NTSC:
            DEBUG(2, "CODER: NTSC Y trap.");
            AT91C_BASE_PIOA->PIO_SODR  = PIN_CODER_CE;
            AT91C_BASE_PIOA->PIO_SODR  = PIN_CODER_NTSC_H;
            AT91C_BASE_PIOA->PIO_SODR  = PIN_DTXD_Y1;
            AT91C_BASE_PIOA->PIO_SODR  = PIN_DRXD_Y2;
            break;

        case CODER_PAL_NOTRAP:
            DEBUG(2, "CODER: PAL no trap.");
            AT91C_BASE_PIOA->PIO_SODR  = PIN_CODER_CE;
            AT91C_BASE_PIOA->PIO_CODR  = PIN_CODER_NTSC_H;
            AT91C_BASE_PIOA->PIO_CODR  = PIN_DTXD_Y1 | PIN_DRXD_Y2;
            break;

        case CODER_NTSC_NOTRAP:
            DEBUG(2, "CODER: NTSC no trap.");
            AT91C_BASE_PIOA->PIO_SODR  = PIN_CODER_CE;
            AT91C_BASE_PIOA->PIO_SODR  = PIN_CODER_NTSC_H;
            AT91C_BASE_PIOA->PIO_CODR  = PIN_DTXD_Y1 | PIN_DRXD_Y2;
            break;

        default :
            ERROR("CODER: Unknown standard.");
    }
}

uint8_t CFG_upload_rom(char* filename, uint32_t base, uint32_t size,
                       uint8_t verify, uint8_t format, uint32_t* sconf, uint32_t* dconf)
{
    FF_FILE* fSource = NULL;
    fSource = FF_Open(pIoman, filename, FF_MODE_READ, NULL);
    uint8_t rc = 1;
    uint32_t offset = 0;
    uint16_t filebase = 0;
    uint32_t bytes_read;

    if (fSource) {
        FF_Seek(fSource, 0, FF_SEEK_SET);

        if (format == 2) {
            // CRT format, contains a header and embedded ROM images (for C64 core only!)
            char varstr[33];
            uint8_t var8;
            uint16_t var16;
            uint32_t var32;
            varstr[32] = 0;
            bytes_read = FF_Read(fSource, 16, 1, (uint8_t*)&varstr);

            if ((bytes_read != 16) || (strncmp(varstr, "C64 CARTRIDGE   ", 16))) {
                FF_Close(fSource);
                ERROR("Bad header text");
                return 1;
            }

            bytes_read = FF_Read(fSource, 4, 1, (uint8_t*)&var32); // read will have wrong endian...

            if ((bytes_read != 4) || ((var32 != 0x40000000) && (var32 != 0x20000000)) ) { // we allow a "bad" header size
                FF_Close(fSource);
                ERROR("Bad header size %04lx", var32);
                return 1;
            }

            bytes_read = FF_Read(fSource, 2, 1, (uint8_t*)&var16);

            if ((bytes_read != 2) || (var16 != 0x0001)) {
                FF_Close(fSource);
                ERROR("Bad version %04x", var16);
                return 1;
            }

            bytes_read = FF_Read(fSource, 2, 1, (uint8_t*)&var16);

            if ((bytes_read != 2) || (var16 != 0x0)) {
                FF_Close(fSource);
                ERROR("Only standard ROM CRT supported (%d)", var16);
                return 1;
            }

            bytes_read = FF_Read(fSource, 1, 1, (uint8_t*)&var8); // EXROM state

            if (bytes_read != 1) {
                FF_Close(fSource);
                ERROR("Unexpected EOF in header");
                return 1;
            }

            (*sconf) &= 0xFFFFFFFE;

            if (var8) {
                (*sconf) |= 1;
            }

            bytes_read = FF_Read(fSource, 1, 1, (uint8_t*)&var8); // GAME state

            if (bytes_read != 1) {
                FF_Close(fSource);
                ERROR("Unexpected EOF in header");
                return 1;
            }

            (*sconf) &= 0xFFFFFFFD;

            if (var8) {
                (*sconf) |= 2;
            }

            bytes_read = FF_Read(fSource, 6, 1, (uint8_t*)&varstr);

            if (bytes_read != 6) {
                FF_Close(fSource);
                ERROR("Unexpected EOF in header");
                return 1;
            }

            bytes_read = FF_Read(fSource, 32, 1, (uint8_t*)&varstr);

            if (bytes_read != 32) {
                FF_Close(fSource);
                ERROR("Bad CRT name");
                return 1;
            }

            INFO("CRT: %s", varstr);
            // now we load CHIP blocks
            offset = 0x40;

            while (1) {
                FF_Seek(fSource, offset, FF_SEEK_SET);  // just in case, do a re-positioning

                if (FF_BytesLeft(fSource) < 0x10) {
                    break;    // finished loading
                }

                bytes_read = FF_Read(fSource, 4, 1, (uint8_t*)&varstr);

                if ((bytes_read != 4) || (strncmp(varstr, "CHIP", 4))) {
                    FF_Close(fSource);
                    ERROR("Bad block text");
                    return 1;
                }

                bytes_read = FF_Read(fSource, 4, 1, (uint8_t*)&var32);  // block size

                if ((bytes_read != 4) || ((var32 != 0x10400000) && (var32 != 0x10200000)) ) {
                    FF_Close(fSource);
                    ERROR("Bad block size %04lx", var32);
                    return 1;
                }

                bytes_read = FF_Read(fSource, 2, 1, (uint8_t*)&var16);

                if ((bytes_read != 2) || (var16 != 0x0)) {
                    FF_Close(fSource);
                    ERROR("Only ROM blocks supported (%d)", var16);
                    return 1;
                }

                bytes_read = FF_Read(fSource, 2, 1, (uint8_t*)&var16);   // bank

                if (bytes_read != 2) {
                    FF_Close(fSource);
                    ERROR("Unexpected EOF in block");
                    return 1;
                }

                INFO("  Bank: %d", var16);
                bytes_read = FF_Read(fSource, 2, 1, (uint8_t*)&var16); // filebase to load

                if (bytes_read != 2) {
                    FF_Close(fSource);
                    ERROR("Unexpected EOF in block");
                    return 1;
                }

                filebase = ((var16 >> 8) & 0xff) + ((var16 & 0xff) << 8); // endian-correct
                INFO("  Start: $%04X", filebase);
                bytes_read = FF_Read(fSource, 2, 1, (uint8_t*)&var16);   // size

                if (bytes_read != 2) {
                    FF_Close(fSource);
                    ERROR("Unexpected EOF in block");
                    return 1;
                }

                size = ((var16 >> 8) & 0xff) + ((var16 & 0xff) << 8); // endian-correct
                INFO("  Size:  $%04X", size);
                offset += 0x10;
                DEBUG(1, "%s @0x%X (0x%X),S:%d", filename, base + filebase, offset, size);
                FileIO_MCh_FileToMem(fSource, base + filebase, size, offset);
                offset += ((var32 >> 8) & 0xFF00); // add endian-corrected block size to offset before starting over
            }

        } else {
            // binary or PRG format
            if (format == 1) {
                // PRG format, start address in first two bytes
                bytes_read = FF_Read(fSource, 2, 1, (uint8_t*)&filebase);

                if (bytes_read == 0) {
                    FF_Close(fSource);
                    ERROR("Unexpected EOF");
                    return 1;
                }

                offset = 2;
            }

            if (!size) { // auto-size
                size = FF_BytesLeft(fSource);
            }

            DEBUG(1, "%s @0x%X (0x%X),S:%d", filename, base + filebase, offset, size);
            FileIO_MCh_FileToMem(fSource, base + filebase, size, offset);

            if (verify) {
                rc = FileIO_MCh_FileToMemVerify(fSource, base + filebase, size, offset);

            } else {
                rc = 0;
            }

            FF_Close(fSource);
        }

    } else {
        ERROR("Could not open %s", filename);
        return 1;
    }

    return rc;
}

uint8_t CFG_download_rom(char* filename, uint32_t base, uint32_t size)
{
    FF_FILE* fSource = NULL;
    uint8_t rc = 1;
    uint32_t offset = 0;
    uint16_t filebase = 0;

    //DEBUG(1,"CFG_download_rom(\"%s\",%x,%x)",filename,base,size);

    fSource = FF_Open(pIoman, filename,
                      FF_MODE_WRITE | FF_MODE_CREATE | FF_MODE_TRUNCATE, NULL);

    //DEBUG(1,"FF_Open(%x,\"%s\",%x,%x) --> %x",pIoman, filename,
    //                  FF_MODE_WRITE|FF_MODE_CREATE|FF_MODE_TRUNCATE, NULL, fSource);

    if (fSource) {
        if (!size) { // auto-size does not work here!
            return 1;
        }

        DEBUG(1, "%s @0x%X (0x%X),S:%d", filename, base + filebase, offset, size);
        rc = FileIO_MCh_MemToFile(fSource, base + filebase, size, offset);
        FF_Close(fSource);

    } else {
        WARNING("Could not open %s", filename);
        return 1;
    }

    return rc;
}

uint32_t CFG_get_free_mem(void)
{
    uint8_t  stack;
    uint32_t total;
    void*    heap;
    /* Allocating less than 1kB memory (e.g. only 4 byte)
       could lead to wrong results !!!  */
    heap = malloc(1024);
    total =  (uint32_t)&stack - (uint32_t)heap;
    free(heap);

    DEBUG(3, "===========================");
    DEBUG(3, " Free memory %ld bytes", total);
    DEBUG(3, "===========================");
    return (total);
}

void CFG_dump_mem_stats(void)
{
    uint32_t ram_bytes, static_bytes, heap_bytes, avail_bytes, unused_bytes;
    uint32_t current_stack, peak_stack;

    uint8_t* p;
    uint8_t* const RAMbeg  = (uint8_t*)__FIRST_IN_RAM;      // top of RAM
    uint8_t* const heap    = (uint8_t*)end;                 // end of data/bss, and start of the heap
    uint8_t* const unused  = (uint8_t*)sbrk(0);             // address of next heap block
    uint8_t* const stack   = (uint8_t*)__builtin_frame_address(0); // current stack frame
    uint8_t* const RAMend  = (uint8_t*)__TOP_STACK;         // end of RAM & first stack frame
    const uint32_t sentinel = 0xFA57F00D;                   // See Cstartup.S
    const struct mallinfo mi = mallinfo();                  // See malloc.h

    ram_bytes = RAMend - RAMbeg;      // total amount of RAM
    static_bytes = heap - RAMbeg;     // size of data/bss sections
    heap_bytes = unused - heap;       // amount of memory mapped to malloc

    // count number of sentinel words still left in RAM
    p = unused;

    while (*(uint32_t*)(void*)p == sentinel && p < stack) {
        p += sizeof(uint32_t);
    }

    if (p > stack) {
        ERROR("HEAP/STACK CORRUPTION! %08x > %08x", p, stack);
        return;
    }

    unused_bytes = p - unused;        // number of 'unmapped' bytes (i.e. not used by the heap nor touched by the stack)
    avail_bytes = unused_bytes + mi.fordblks;  // unmapped + unused bytes

    current_stack = RAMend - stack;   // amount of stack space current in-use
    peak_stack = RAMend - p;          // peak stack usage

    DEBUG(1, "     ___________________________________________________");
    DEBUG(1, "MEM | Total : %5d = %5d static  + %5d heap + %d unused + %d stack", ram_bytes, static_bytes, heap_bytes, unused_bytes, peak_stack);
    DEBUG(1, "MEM | Heap  : %5d = %5d in-use  + %5d free", mi.arena, mi.uordblks, mi.fordblks);
    DEBUG(1, "MEM | Stack : %5d = %5d current + %5d touched", peak_stack, current_stack, peak_stack - current_stack);
    DEBUG(1, "MEM | Avail : %5d = %5d free    + %5d unused", avail_bytes, mi.fordblks, unused_bytes);
}

uint8_t CFG_configure_fpga(char* filename)
{
    FF_FILE* fSource = NULL;

    fSource = FF_Open(pIoman, filename, FF_MODE_READ, NULL);

    if (fSource) {
        uint8_t retryCounter = 0;
        uint8_t configStatus = 0;

        DEBUG(1, "FPGA configuration: %s found", filename);

        do {
            configStatus = FPGA_Config(fSource);
            FF_Seek(fSource, 0, FF_SEEK_SET);
            retryCounter ++;
        } while (configStatus && retryCounter < 3);

        if (configStatus) {
            ERROR("!! Failed to configure FPGA.");
            return 4;
        }

        FF_Close(fSource);

    } else {
        ERROR("!! %s not found.", filename);
        return 3;
    }

    return 0;
}

void CFG_vid_timing_SD(framerate_t standard)
{
    if (standard == F50HZ) {
        Configure_ClockGen(&init_clock_config_pal);

    } else {
        Configure_ClockGen(&init_clock_config_ntsc);
    }

    // we should turn y1 clock off if coder disabled

    Configure_VidBuf(1, 0, 0, 3); // 9MHZ
    Configure_VidBuf(2, 0, 0, 3);
    Configure_VidBuf(3, 0, 0, 3);
}

void CFG_vid_timing_HD27(framerate_t standard) // for 480P/576P
{
    // PAL/NTSC base clock, but scan doubled. Coder disabled

    if (standard == F50HZ) {
        Configure_ClockGen(&init_clock_config_pal);

    } else {
        Configure_ClockGen(&init_clock_config_ntsc);
    }

    Configure_VidBuf(1, 0, 1, 3); // 16MHz
    Configure_VidBuf(2, 0, 1, 3);
    Configure_VidBuf(3, 0, 1, 3);

}

void CFG_vid_timing_HD74(framerate_t standard) // for 720P etc
{
    // just using 74.24 not /1.001

    Configure_ClockGen(&init_clock_config_hd74); // 60 HZ setup on base clock for now
    Configure_VidBuf(1, 0, 3, 3);
    Configure_VidBuf(2, 0, 3, 3);
    Configure_VidBuf(3, 0, 3, 3);
}

void CFG_set_CH7301_SD(void)
{
    Configure_CH7301(&default_vid_config_sd);
}

void CFG_set_CH7301_HD(void)
{
    Configure_CH7301(&default_vid_config_hd);
}


// ===========================================================================
// INI HANDLING: INITIAL FPGA SETUP
// ===========================================================================
/*
Example INI file (pre-setup part):
------------------------------------------------------------------------------
[SETUP]
bin = replay.bin

# sets initial clocking (PLL)
#       PAL / NTSC
CLOCK = PAL
# or direct setup (here: NTSC)
#          M1 N1 M2 N2 M3 N3      ps0 ps1 ps2 ps3 ps4 ps5 pd0 pd1 pd2 pd3 pd4 pd5 ys0 ys1 ys2 ys3 ys4 ys5
#CLOCK =   33,280,27,133,225,2048,1,1,4,0,0,0,2,16,5,1,1,1,1,1,1,0,1,0

# sets composite/svhs coder, if fitted
coder =  PAL_NOTRAP

# sets video filter for all 3 channels
# dc=250mv, 9MHZ
VFILTER = 0,0,3

# configures what will be available on FPGA after bin is loaded,
# if a line is ommited, the interface will be not used at all (= all zero)
#
# twi passed for further video configuration
#
#         vid
en_twi =   1

# spi for:
#     cfg to send/receive config words
#     osd for OSD video/key support
#
#         cfg osd
en_spi =   1,  1

# replay button configuration
#
#        off / menu / reset
button = menu

# OSD hotkey combo configuration
#
#        f12 / ...
hotkey = f12

[UPLOAD]

# enables verification feature, checks back any ROM/DATA upload
VERIFY = 1

------------------------------------------------------------------------------
*/

void CFG_set_status_defaults(status_t* currentStatus, uint8_t init)
{

    // for init, assume struct has been zero'd

    if (init) {
        strcpy(currentStatus->ini_dir, "\\");
        strcpy(currentStatus->act_dir, "\\");
        strcpy(currentStatus->ini_file, "replay.ini");
        strcpy(currentStatus->bin_file, "replay.bin");
    }

    // these will all be reset during the ini preread
    currentStatus->twi_enabled      = 0;
    currentStatus->spi_osd_enabled  = 0;
    currentStatus->spi_fpga_enabled = 0;
    currentStatus->verify_dl        = 0;
    currentStatus->last_rom_adr     = 0;
    currentStatus->dram_phase       = 0;
    currentStatus->clockmon         = 0;
    currentStatus->fileio_cha_ena   = 0;
    currentStatus->fileio_chb_ena   = 0;

    currentStatus->button   = BUTTON_MENU;
    currentStatus->hotkey   = KEY_MENU;
    _strlcpy(currentStatus->hotkey_string,
             OSD_GetStringFromKeyCode(currentStatus->hotkey),
             sizeof(currentStatus->hotkey_string));
    currentStatus->clockmon = FALSE;
    currentStatus->osd_init = OSD_INIT_ON;
    currentStatus->osd_timeout = 30;

    currentStatus->clock_cfg  = init_clock_config_ntsc;
    currentStatus->filter_cfg = init_vidbuf;
    currentStatus->coder_cfg  = CODER_DISABLE;


    // DAC setup?
}


// === Configuration file option callbacks
// =======================================

static uint8_t _CFG_handle_SETUP_BIN(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    strncpy(pStatus->bin_file, value, sizeof(pStatus->bin_file));
    DEBUG(2, "Will use %s for the FPGA.", pStatus->bin_file);
    return 0;
}

static uint8_t _CFG_handle_SETUP_CLOCK(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    /*strcpy(pStatus->clock_bak,value);*/
    if (MATCH(value, "PAL")) {
        pStatus->clock_cfg = init_clock_config_pal;
        /*Configure_ClockGen(&init_clock_config_pal);*/

    } else if (MATCH(value, "NTSC")) {
        pStatus->clock_cfg = init_clock_config_ntsc;
        /*Configure_ClockGen(&init_clock_config_ntsc);*/

    } else if (MATCH(value, "HD74")) {
        /*Configure_ClockGen(&init_clock_config_hd74);*/
        pStatus->clock_cfg = init_clock_config_hd74;

    } else {
        ini_list_t valueList[32];
        uint16_t entries = ParseList(value, valueList, 32);

        if (entries == 24) {
            clockconfig_t clkcfg = {
                valueList[0].intval, valueList[1].intval,       // pll1 m,n
                valueList[2].intval, valueList[3].intval,       // pll2 m,n
                valueList[4].intval, valueList[5].intval,       // pll3 m,n
                {
                    valueList[6].intval, valueList[7].intval,
                    valueList[8].intval, valueList[9].intval,
                    valueList[10].intval, valueList[11].intval,
                },//p0,1,2,3,4,5
                {
                    valueList[12].intval, valueList[13].intval,
                    valueList[14].intval, valueList[15].intval,
                    valueList[16].intval, valueList[17].intval,
                },//d0,1,2,3,4,5
                {
                    valueList[18].intval, valueList[19].intval,
                    valueList[20].intval, valueList[21].intval,
                    valueList[22].intval, valueList[23].intval
                } //y0,1,2,3,4,5
            };
            // sanity check??? nope
            pStatus->clock_cfg = clkcfg;
            /*Configure_ClockGen(&clkcfg);*/

        } else {
            WARNING("clock config invalid: %d entries", entries);
            return 1;
        }
    }

    return 0;
}



static uint8_t _CFG_handle_SETUP_CODER(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    coder_t codcfg;

    /*strcpy(pStatus->coder_bak,value);*/
    if (MATCH(value, "DISABLE")) {
        codcfg = CODER_DISABLE;

    } else if (MATCH(value, "PAL")) {
        codcfg = CODER_PAL;

    } else if (MATCH(value, "NTSC")) {
        codcfg = CODER_NTSC;

    } else if (MATCH(value, "PAL_NOTRAP")) {
        codcfg = CODER_PAL_NOTRAP;

    } else if (MATCH(value, "NTSC_NOTRAP")) {
        codcfg = CODER_NTSC_NOTRAP;

    } else {
        WARNING("coder config invalid!");
        return 1;
    }

    if (pStatus->coder_fitted) {
        /*CFG_set_coder(codcfg);*/
        pStatus->coder_cfg = codcfg;

        DEBUG(1, "Coder configured");

    } else {
        DEBUG(0, "No coder fitted!");
    }

    return 0;
}



static uint8_t _CFG_handle_SETUP_VFILTER(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    ini_list_t valueList[8];
    /*strcpy(pStatus->vfilter_bak,value);*/
    uint16_t entries = ParseList(value, valueList, 8);

    if (entries == 3) {
        /*Configure_VidBuf(1, valueList[0].intval, valueList[1].intval, valueList[2].intval);*/
        /*Configure_VidBuf(2, valueList[0].intval, valueList[1].intval, valueList[2].intval);*/
        /*Configure_VidBuf(3, valueList[0].intval, valueList[1].intval, valueList[2].intval);*/

        pStatus->filter_cfg.stc  = valueList[0].intval;
        pStatus->filter_cfg.lpf  = valueList[1].intval;
        pStatus->filter_cfg.mode = valueList[2].intval;

        DEBUG(1, "VFILTER mode: %d", pStatus->filter_cfg.mode);

    } else {
        WARNING("VFILTER config invalid");
    }

    return entries == 3 ? 0 : 1;
}


static uint8_t _CFG_handle_SETUP_PHASE(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    ini_list_t valueList[8];
    uint16_t entries = ParseList(value, valueList, 8);

    if (entries == 1) {
        pStatus->dram_phase = valueList[0].intval;
        DEBUG(1, "DRAM phase: %d", pStatus->dram_phase);
        return 0;
    }

    // NOTE: No `strval` used in valueList, so no FreeList required

    WARNING("Invalid DRAM phase config");
    return 1;
}

static uint8_t _CFG_handle_SETUP_EN_TWI(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    ini_list_t valueList[8];
    uint16_t entries = ParseList(value, valueList, 8);

    if (entries == 1) {
        pStatus->twi_enabled = valueList[0].intval;
        DEBUG(1, "TWI passthrough: %s", pStatus->twi_enabled ? "ENABLED" : "-");
        return 0;
    }

    // NOTE: No `strval` used in valueList, so no FreeList required
    return 1;
}

static uint8_t _CFG_handle_SETUP_EN_SPI(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    ini_list_t valueList[8];
    uint16_t entries = ParseList(value, valueList, 8);

    if (entries == 2) {
        pStatus->spi_fpga_enabled = valueList[0].intval;
        pStatus->spi_osd_enabled = valueList[1].intval;
        DEBUG(1, "SPI control: %s %s ", pStatus->spi_fpga_enabled ? "FPGA" : "-"
              , pStatus->spi_osd_enabled ? "OSD" : "-");
        return 0;
    }

    // NOTE: No `strval` used in valueList, so no FreeList required
    return 1;
}

static uint8_t _CFG_handle_SETUP_SPI_CLK(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    ini_list_t valueList[8];
    uint16_t entries = ParseList(value, valueList, 8);

    if (entries == 1) {
        // we allow only reducing clock to avoid issues with sdcard settings
        if (valueList[0].intval >
                ((AT91C_BASE_SPI->SPI_CSR[0] & AT91C_SPI_SCBR) >> 8)) {
            /*pStatus->spiclk_old = ((AT91C_BASE_SPI->SPI_CSR[0] &*/
            /*AT91C_SPI_SCBR) >> 8);*/
            AT91C_BASE_SPI->SPI_CSR[0] = AT91C_SPI_CPOL |
                                         (valueList[0].intval << 8);
            uint32_t spiFreq = BOARD_MCK /
                               ((AT91C_BASE_SPI->SPI_CSR[0] & AT91C_SPI_SCBR) >> 8) /
                               1000000;
            DEBUG(1, "New SPI clock: %d MHz", spiFreq);
            /*pStatus->spiclk_bak = valueList[0].intval;*/
        }

        return 0;
    }

    return 1;
}

static uint8_t _CFG_handle_SETUP_BUTTON(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    if (MATCH(value, "OFF")) {
        pStatus->button = BUTTON_OFF;

    } else if (MATCH(value, "RESET")) {
        pStatus->button = BUTTON_RESET;

    } else if (MATCH(value, "MENU")) {
        pStatus->button = BUTTON_MENU;

    } else {
        return 1;
    }

    DEBUG(1, "Button control: %s", value);
    return 0;
}

static uint8_t _CFG_handle_SETUP_HOTKEY(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    uint16_t keycode = OSD_GetKeyCodeFromString(value);

    if (keycode) {
        pStatus->hotkey = keycode;
        _strlcpy(pStatus->hotkey_string, OSD_GetStringFromKeyCode(keycode), sizeof(pStatus->hotkey_string));
        DEBUG(1, "Hotkey control: %s -> %04X / %s", value, pStatus->hotkey, pStatus->hotkey_string);
        return 0;
    }

    return 1;
}

/*static uint8_t _CFG_handle_SETUP_INI_KEYB_MODE(status_t *pStatus, const ini_symbols_t name, const char *value)
{
  WARNING("KEYBOARD keyword is obsolete");
  return 0;
}*/

static uint8_t _CFG_handle_SETUP_CLOCKMON(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    if (MATCH(value, "ENA")) {
        pStatus->clockmon = TRUE;

    } else if (MATCH(value, "ENABLE")) {
        pStatus->clockmon = TRUE;

    } else if (MATCH(value, "DIS")) {
        pStatus->clockmon = FALSE;

    } else if (MATCH(value, "DISABLE")) {
        pStatus->clockmon = FALSE;

    } else {
        return 1;
    }

    if (pStatus->clockmon) {
        MSG_info("ClockMonitor Enabled");
    }

    return 0;
}

static uint8_t _CFG_handle_SETUP_OSD_INIT(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    if (MATCH(value, "OFF")) {
        pStatus->osd_init = OSD_INIT_OFF;

    } else if (MATCH(value, "ON")) {
        pStatus->osd_init = OSD_INIT_ON;

    } else {
        WARNING("CLOCKMON config invalid");
        return 1;
    }

    DEBUG(1, "OSD init: %s", value);
    return 0;
}

static uint8_t _CFG_handle_SETUP_OSD_TIMEOUT(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    ini_list_t valueList[1];
    uint16_t entries = ParseList(value, valueList, 1);

    if (entries == 1) {
        if (valueList[0].intval > 255 || valueList[0].intval < 0) {
            WARNING("osd_timeout value of of range");

        } else {
            pStatus->osd_timeout = valueList[ 0].intval;
        }

    } else {
        WARNING("invalid osd_timeout config");
    }

    DEBUG(1, "OSD timeout: %d", pStatus->osd_timeout);
    return 0;
}


static uint8_t _CFG_handle_SETUP_VIDEO(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    /*strcpy(pStatus->video_bak,value);*/
    if (pStatus->twi_enabled) {
        ini_list_t valueList[32];
        uint16_t entries = ParseList(value, valueList, 32);

        if (entries == 16) {
            const vidconfig_t vid_config = {
                valueList[ 0].intval, valueList[ 1].intval,
                valueList[ 2].intval, valueList[ 3].intval,
                valueList[ 4].intval, valueList[ 5].intval,
                valueList[ 6].intval, valueList[ 7].intval,
                valueList[ 8].intval, valueList[ 9].intval,
                valueList[10].intval, valueList[11].intval,
                valueList[12].intval, valueList[13].intval,
                valueList[14].intval, valueList[15].intval,
            };
            Configure_CH7301(&vid_config);

        } else {
            WARNING("VIDEO config invalid");
            return 1;
        }

        DEBUG(2, "VIDEO: %s", value);
        return 0;
    }

    WARNING("VIDEO config but TWI disabled");
    return 1;
}


static uint8_t _CFG_handle_SETUP_CONFIG(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    if (pStatus->spi_fpga_enabled) {
        ini_list_t valueList[8];
        uint16_t entries = ParseList(value, valueList, 8);

        if (entries == 2) {
            pStatus->config_s = valueList[0].intval;
            pStatus->config_d = valueList[1].intval;
            DEBUG(2, "CONFIG: static=0x%08X, dynamic=0x%08X",
                  pStatus->config_s, pStatus->config_d);
            return 0;
        }

        WARNING("FPGA config: invalid");

    } else {
        WARNING("FPGA config: SPI disabled");
    }

    return 1;
}


static uint8_t _CFG_handle_SETUP_INFO(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    ini_list_t valueList[8];
    uint16_t entries = ParseList(value, valueList, 8);

    if (entries == 1) {
        MSG_info(valueList[0].strval);
    }

    FreeList(valueList, entries);
    return 0; // always successful
}




static uint8_t _CFG_handle_TARGETS_TARGET(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    ini_list_t valueList[3];
    uint16_t entries = ParseList(value, valueList, 3);

    if (entries == 2 || entries == 3) { // name,file or name,dir,file
        tIniTarget* t = malloc(sizeof(tIniTarget));

        // NOTE: We do NOT free the valuelist, since we hijack the str ptrs to avoid a dup/copy
        t->name = valueList[0].strval;
        t->dir = entries == 3 ? valueList[1].strval : NULL;
        t->file = entries == 2 ? valueList[1].strval : valueList[2].strval;
        t->next = pStatus->ini_targets;
        pStatus->ini_targets = t;
        DEBUG(3, "Added target: %s: %s\\%s", t->name, t->dir, t->file);
        return 0;
    }

    WARNING("Invalid target config");
    return 1;
}

static uint8_t _CFG_handle_MENU_TITLE(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    ini_list_t valueList[8];
    uint16_t entries = ParseList(value, valueList, 8);

    if (entries == 1) {
        DEBUG(2, "TITLE: %s ", value);
        DEBUG(3, "T1: %lx %lx %lx ", pStatus->menu_act,
              pStatus->menu_act->next, pStatus->menu_act->item_list);

        // we filled this menu branch already with items
        if (pStatus->menu_top) {   // add further entry
            // prepare next level and set pointers correctly
            pStatus->menu_act->next = malloc(sizeof(menu_t));
            // link back
            pStatus->menu_act->next->last = pStatus->menu_act;
            // step in linked list
            pStatus->menu_act = pStatus->menu_act->next;

        } else {                   // first top entry
            // prepare top level
            pStatus->menu_act = malloc(sizeof(menu_t));
            pStatus->menu_act->last = NULL;
            // set top level
            pStatus->menu_top = pStatus->menu_act;
        }

        pStatus->menu_act->next = NULL;
        pStatus->menu_act->item_list = malloc(sizeof(menuitem_t));
        pStatus->menu_item_act = pStatus->menu_act->item_list;
        pStatus->menu_item_act->item_name[0] = 0;
        pStatus->menu_item_act->next = NULL;
        pStatus->menu_item_act->last = NULL;
        pStatus->menu_item_act->option_list = NULL;
        pStatus->menu_item_act->selected_option = NULL;
        pStatus->menu_item_act->action_name[0] = 0;
        // store title to actual branch
        strncpy(pStatus->menu_act->menu_title,
                valueList[0].strval, MAX_MENU_STRING);

        DEBUG(3, "T2: %lx %lx %lx ", pStatus->menu_act,
              pStatus->menu_act->next, pStatus->menu_act->item_list);
    }

    FreeList(valueList, entries);

    return entries == 1 ? 0 : 1;
}

static uint8_t _CFG_handle_MENU_ITEM(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    ini_list_t valueList[8];
    uint16_t entries = ParseList(value, valueList, 8);

    if ((entries >= 2) && (entries <= 4)) {
        DEBUG(2, "ITEM: %s ", value);
        DEBUG(3, "I1: %lx %lx %lx ", pStatus->menu_item_act,
              pStatus->menu_item_act->next,
              pStatus->menu_item_act->option_list);

        // item string is set, so this is a new item
        // (happens with all but the seed item entry of a menu)
        if (pStatus->menu_item_act->item_name[0]) {
            pStatus->menu_item_act->next = malloc(sizeof(menuitem_t));
            // link back
            pStatus->menu_item_act->next->last = pStatus->menu_item_act;
            // step in linked list
            pStatus->menu_item_act = pStatus->menu_item_act->next;
        }

        // basic settings
        pStatus->menu_item_act->next = NULL;
        pStatus->menu_item_act->option_list = NULL;
        pStatus->menu_item_act->selected_option = NULL;
        pStatus->menu_item_act->action_name[0] = 0;
        strncpy(pStatus->menu_item_act->item_name,
                valueList[0].strval, MAX_MENU_STRING);

        // special item with own handler, options handled there or ignored
        if (entries == 2) {
            strncpy(pStatus->menu_item_act->action_name, valueList[1].strval,
                    MAX_ITEM_STRING);
            pStatus->menu_item_act->action_value = 0;
        }

        // absolute mask item or action handler with value
        if (entries == 3) {
            pStatus->menu_item_act->conf_mask = valueList[1].intval;

            if (MATCH(valueList[2].strval, "DYNAMIC") ||
                    MATCH(valueList[2].strval, "DYN") ||
                    MATCH(valueList[2].strval, "D")) {
                pStatus->menu_item_act->conf_dynamic = 1;

            } else if (MATCH(valueList[2].strval, "STATIC") ||
                       MATCH(valueList[2].strval, "STAT") ||
                       MATCH(valueList[2].strval, "S")) {
                pStatus->menu_item_act->conf_dynamic = 0;

            } else {
                strncpy(pStatus->menu_item_act->action_name,
                        valueList[1].strval, MAX_ITEM_STRING);
                pStatus->menu_item_act->action_value = valueList[2].intval;
            }
        }

        // value/shift mask item
        if (entries == 4) {
            pStatus->menu_item_act->conf_mask = valueList[1].intval <<
                                                valueList[2].intval;

            if (MATCH(valueList[3].strval, "DYNAMIC") ||
                    MATCH(valueList[3].strval, "DYN") ||
                    MATCH(valueList[3].strval, "D")) {
                pStatus->menu_item_act->conf_dynamic = 1;

            } else if (MATCH(valueList[3].strval, "STATIC") ||
                       MATCH(valueList[3].strval, "STAT") ||
                       MATCH(valueList[3].strval, "S")) {
                pStatus->menu_item_act->conf_dynamic = 0;

            } else {
                pStatus->menu_item_act->conf_dynamic = 0;
                WARNING("Menu items must be static or dynamic");
                FreeList(valueList, entries);
                return 1;
            }
        }

        pStatus->menu_item_act->option_list = malloc(sizeof(itemoption_t));
        pStatus->item_opt_act = pStatus->menu_item_act->option_list;
        pStatus->item_opt_act->next = NULL;
        pStatus->item_opt_act->last = NULL;
        pStatus->item_opt_act->option_name[0] = 0;
        DEBUG(3, "I2: %lx %lx %lx ", pStatus->menu_item_act,
              pStatus->menu_item_act->next,
              pStatus->menu_item_act->option_list);

        FreeList(valueList, entries);
        return 0;
    }

    FreeList(valueList, entries);
    return 1;
}

static uint8_t _CFG_handle_MENU_OPTION(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    ini_list_t valueList[8];
    uint8_t nocheck = 0;
    uint16_t entries = ParseList(value, valueList, 8);

    if ((entries == 2) || (entries == 3)) {
        DEBUG(2, "OPTION: %s ", value);
        DEBUG(3, "O1: %lx %lx", pStatus->item_opt_act,
              pStatus->item_opt_act->next);

        // option string is set, so we add a new one
        // (happens with all but the seed option entry of a menu item)
        if (pStatus->item_opt_act->option_name[0]) {
            pStatus->item_opt_act->next = malloc(sizeof(menuitem_t));
            // link back
            pStatus->item_opt_act->next->last = pStatus->item_opt_act;
            // step in linked list
            pStatus->item_opt_act = pStatus->item_opt_act->next;
            pStatus->item_opt_act->next = NULL;
        }

        pStatus->item_opt_act->conf_value = valueList[1].intval;

        if (entries == 3) {
            if (MATCH(valueList[2].strval, "FLAGS") ||
                    MATCH(valueList[2].strval, "F")) {
                nocheck = 1;

            } else {
                if (MATCH(valueList[2].strval, "DEFAULT") ||
                        MATCH(valueList[2].strval, "DEF") ||
                        MATCH(valueList[2].strval, "D")) {
                    pStatus->menu_item_act->selected_option = pStatus->item_opt_act;

                } else {
                    WARNING("bad option type - use 'default' or none");
                    FreeList(valueList, entries);
                    return 1;
                }
            }
        }

        if ((pStatus->menu_item_act->conf_mask &
                pStatus->item_opt_act->conf_value) !=
                pStatus->item_opt_act->conf_value) {
            if (!nocheck) {
                WARNING("item mask does not fit to value");
                FreeList(valueList, entries);
                return 1;
            }
        }

        // protect from no default by setting selected to first item if not set
        if (!pStatus->menu_item_act->selected_option) {
            pStatus->menu_item_act->selected_option = pStatus->item_opt_act;
        }

        strncpy(pStatus->item_opt_act->option_name,
                valueList[0].strval, MAX_OPTION_STRING);
        DEBUG(3, "O2: %lx %lx", pStatus->item_opt_act,
              pStatus->item_opt_act->next);
        FreeList(valueList, entries);
        return 0;
    }

    FreeList(valueList, entries);
    return 1;
}


static uint8_t _CFG_handle_UPLOAD_VERIFY(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    ini_list_t valueList[8];
    uint16_t entries = ParseList(value, valueList, 8);

    if (entries == 1) {
        pStatus->verify_dl = valueList[0].intval;
    }

    DEBUG(1, "UPLOAD verification: %s", pStatus->verify_dl ? "ENABLED" : "DISABLED");
    return 0;
}


static uint8_t _CFG_handle_UPLOAD_DATA(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    //      (name is relative to INI path)
    /*if (pStatus->data_bak) {*/
    /*pStatus->data_bak_last->next=malloc(sizeof(data_list_t));*/
    /*pStatus->data_bak_last=pStatus->data_bak_last->next;*/
    /*} else {*/
    /*pStatus->data_bak=malloc(sizeof(data_list_t));*/
    /*pStatus->data_bak_last=pStatus->data_bak;*/
    /*}*/
    /*pStatus->data_bak_last->next=NULL;*/
    /*strcpy(pStatus->data_bak_last->data_bak,value);*/
    if (!pStatus->spi_fpga_enabled) {
        WARNING("UPLOAD DATA: SPI disabled");
        return 1;
    }

    ini_list_t valueList[32];
    uint16_t entries = ParseList(value, valueList, 32);

    // check for maximum of 16 entries
    // (plus address and dataset length), check both lengths
    if ((entries <= 18) && ((entries - 2) == valueList[entries - 1].intval)) {
        uint8_t buf[16];

        for (int i = 0; i < (entries - 2); buf[i] = valueList[i].intval, i++);

        DEBUG(2, "Data upload @ 0x%08lX (%ld byte)",
              valueList[entries - 2].intval, entries - 2);

        if (FileIO_MCh_BufToMem(buf, valueList[entries - 2].intval,
                                valueList[entries - 1].intval)) {
            WARNING("DATA upload to FPGA failed");
            return 1;
        }

        if (pStatus->verify_dl) {
            // required to readback data and check it again...
            uint8_t tmpbuf[16];
            FileIO_MCh_MemToBuf(tmpbuf, valueList[entries - 2].intval,
                                valueList[entries - 1].intval);

            for (int i = 0; i < (entries - 2); i++) {
                if (tmpbuf[i] != buf[i]) {
                    WARNING("DATA verification failed");
                    return 1;
                }
            }

            return 0;

        } else {
            return FileIO_MCh_BufToMem(buf, valueList[entries - 2].intval,
                                       valueList[entries - 1].intval);
        }
    }

    return 1;
}


// ===> Get data from FPGA and execute
static uint8_t _CFG_handle_UPLOAD_LAUNCH(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    ini_list_t valueList[8];
    uint16_t entries = ParseList(value, valueList, 8);

    if (entries == 3) {
        uint32_t start   = valueList[0].intval;
        uint32_t length  = valueList[1].intval;
        uint32_t checksum = valueList[2].intval;
        DEBUG(2, "LAUNCH FROM MEMORY @ 0x%lx, l:%d (s:0x%x),", start, length, checksum);
        FPGA_ExecMem(start, (uint16_t)length, checksum);
        return 0;

    } else {
        return 1;
    }
}


static uint8_t _CFG_handle_UPLOAD_ROM(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    // (name is relative to INI path)
    if (!pStatus->spi_fpga_enabled) {
        WARNING("ROM upload, but SPI disabled");
        return 1;
    }

    ini_list_t valueList[8];
    uint16_t entries = ParseList(value, valueList, 8);

    if ((entries == 4) && (strlen(valueList[0].strval))) {
        char fullname[FF_MAX_PATH];
        // set address properly
        pStatus->last_rom_adr = valueList[2].intval + valueList[1].intval;
        // prepare filename
        sprintf(fullname, "%s%s", pStatus->ini_dir, valueList[0].strval);

        uint32_t staticbits = pStatus->config_s;
        uint32_t dynamicbits = pStatus->config_d;
        DEBUG(1, "OLD config - S:%08lx D:%08lx", staticbits, dynamicbits);
        DEBUG(2, "ROM upload @ 0x%08lX (%ld byte)",
              valueList[2].intval, valueList[1].intval);

        if (CFG_upload_rom(fullname, valueList[2].intval, valueList[1].intval,
                           pStatus->verify_dl, valueList[3].intval, &staticbits, &dynamicbits)) {
            WARNING("ROM upload to FPGA failed");
            FreeList(valueList, entries);
            return 1;

        } else {
            DEBUG(1, "NEW config - S:%08lx D:%08lx", staticbits, dynamicbits);
            pStatus->config_s = staticbits;
            //not used yet: pStatus->config_d = dynamicbits;
            // send bits to FPGA
            OSD_ConfigSendUserS(staticbits);
            //not used yet: OSD_ConfigSendUserD(dynamicbits);
            FreeList(valueList, entries);
            return 0;
        }

    } else if ((entries == 3) && (strlen(valueList[0].strval))) {
        char fullname[FF_MAX_PATH];
        // set address properly
        pStatus->last_rom_adr = valueList[2].intval + valueList[1].intval;
        // prepare filename
        sprintf(fullname, "%s%s", pStatus->ini_dir, valueList[0].strval);

        uint32_t staticbits = pStatus->config_s;
        uint32_t dynamicbits = pStatus->config_d;
        DEBUG(1, "OLD config - S:%08lx D:%08lx", staticbits, dynamicbits);
        DEBUG(2, "ROM upload @ 0x%08lX (%ld byte)",
              valueList[2].intval, valueList[1].intval);

        if (CFG_upload_rom(fullname, valueList[2].intval, valueList[1].intval,
                           pStatus->verify_dl, 0, &staticbits, &dynamicbits)) {
            WARNING("ROM upload to FPGA failed");
            FreeList(valueList, entries);
            return 1;

        } else {
            DEBUG(1, "NEW config - S:%08lx D:%08lx", staticbits, dynamicbits);
            pStatus->config_s = staticbits;
            //not used yet: pStatus->config_d = dynamicbits;
            // send bits to FPGA
            OSD_ConfigSendUserS(staticbits);
            //not used yet: OSD_ConfigSendUserD(dynamicbits);
            FreeList(valueList, entries);
            return 0;
        }

    } else if ((entries == 2) && (strlen(valueList[0].strval))) {
        char fullname[FF_MAX_PATH];
        uint32_t adr = pStatus->last_rom_adr;
        // increment address properly
        pStatus->last_rom_adr = adr + valueList[1].intval;
        // prepare filename
        sprintf(fullname, "%s%s", pStatus->ini_dir, valueList[0].strval);

        uint32_t staticbits = pStatus->config_s;
        uint32_t dynamicbits = pStatus->config_d;
        DEBUG(1, "OLD config - S:%08lx D:%08lx", staticbits, dynamicbits);
        DEBUG(2, "ROM upload @ 0x%08lX (%ld byte)",
              adr, valueList[1].intval);

        if (CFG_upload_rom(fullname, adr, valueList[1].intval,
                           pStatus->verify_dl, 0, &staticbits, &dynamicbits)) {
            WARNING("ROM upload to FPGA failed");
            FreeList(valueList, entries);
            return 1;

        } else {
            DEBUG(1, "NEW config - S:%08lx D:%08lx", staticbits, dynamicbits);
            pStatus->config_s = staticbits;
            //not used yet: pStatus->config_d = dynamicbits;
            // send bits to FPGA
            OSD_ConfigSendUserS(staticbits);
            //not used yet: OSD_ConfigSendUserD(dynamicbits);
            FreeList(valueList, entries);
            return 0;
        }

    } else {
        WARNING("invalid ROM config: entries=%d str0: %s", entries, valueList[0].strval);
    }

    FreeList(valueList, entries);
    return 1;
}



static uint8_t _CFG_handle_FILES_CHA_MOUNT(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    ini_list_t valueList[2];
    uint16_t entries = ParseList(value, valueList, 2);
    uint8_t  unit = 0;

    if ((entries == 1) || (entries == 2)) {
        if (entries == 2) {
            unit = valueList[1].intval;
        }

        if (unit < FCH_MAX_NUM) {
            if (strlen(valueList[0].strval)) {
                char fullname[FF_MAX_PATH];
                // prepare filename
                sprintf(fullname, "%s%s", pStatus->ini_dir, valueList[0].strval);
                FileIO_FCh_Insert(0, unit, fullname);
            };

        } else {
            DEBUG(1, "Illegal FileIO ChA number");
        };

    }

    FreeList(valueList, entries);

    return 0;
}

static uint8_t _CFG_handle_FILES_CHA_CFG(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    ini_list_t valueList[5];
    uint16_t entries = ParseList(value, valueList, 5);

    if (entries > 0) {
        if (MATCH(valueList[0].strval, "REMOVABLE")) {
            pStatus->fileio_cha_mode = (fileio_mode_t) REMOVABLE;
            DEBUG(1, "ChA Removable");

        } else if (MATCH(valueList[0].strval, "FIXED")) {
            pStatus->fileio_cha_mode = (fileio_mode_t) FIXED;
            DEBUG(1, "ChA Fixed");

        } else {
            WARNING("ChA bad drive type - use 'removable' or fixed");
            FreeList(valueList, entries);
            return 1;
        }
    }

    if (entries > 1) {
        DEBUG(1, "ChA %d extension filters: ", entries - 1);
        pStatus->fileio_cha_ext = malloc(sizeof(file_ext_t) * entries);

        for (uint16_t i = 0; i < entries - 1; ++i) {
            _strlcpy(pStatus->fileio_cha_ext[i].ext, valueList[i + 1].strval, sizeof(file_ext_t));
            DEBUG(1, "ChA extension %d : %s", i, pStatus->fileio_cha_ext[i].ext);
        }

        memset(pStatus->fileio_cha_ext[entries - 1].ext, 0x00, sizeof(file_ext_t));

    } else {
        pStatus->fileio_cha_ext = NULL; // == "*"
    }

    FreeList(valueList, entries);
    return 0;
}

static uint8_t _CFG_handle_FILES_CHB_MOUNT(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    ini_list_t valueList[2];
    uint16_t entries = ParseList(value, valueList, 2);
    uint8_t  unit = 0;

    if ((entries == 1) || (entries == 2)) {
        if (entries == 2) {
            unit = valueList[1].intval;
        }

        if (unit < FCH_MAX_NUM) {
            if (strlen(valueList[0].strval)) {
                char fullname[FF_MAX_PATH];
                // prepare filename
                sprintf(fullname, "%s%s", pStatus->ini_dir, valueList[0].strval);
                FileIO_FCh_Insert(1, unit, fullname);
            };

        } else {
            DEBUG(1, "Illegal FileIO ChB number");
        };

    }

    FreeList(valueList, entries);

    return 0;
}


static uint8_t _CFG_handle_FILES_CHB_CFG(status_t* pStatus, const ini_symbols_t name, const char* value)
{
    ini_list_t valueList[5];
    uint16_t entries = ParseList(value, valueList, 5);

    if (entries > 0) {
        if (MATCH(valueList[0].strval, "REMOVABLE")) {
            pStatus->fileio_chb_mode = (fileio_mode_t) REMOVABLE;
            DEBUG(1, "ChB Removable");

        } else if (MATCH(valueList[0].strval, "FIXED")) {
            pStatus->fileio_chb_mode = (fileio_mode_t) FIXED;
            DEBUG(1, "ChB Fixed");

        } else {
            WARNING("ChB bad drive type - use 'removable' or fixed");
            FreeList(valueList, entries);
            return 1;
        }
    }

    if (entries > 1) {
        DEBUG(1, "ChB %d extension filters: ", entries - 1);
        pStatus->fileio_chb_ext = malloc(sizeof(file_ext_t) * entries);

        for (uint16_t i = 0; i < entries - 1; ++i) {
            _strlcpy(pStatus->fileio_chb_ext[i].ext, valueList[i + 1].strval, sizeof(file_ext_t));
            DEBUG(1, "ChB extension %d : %s", i, pStatus->fileio_chb_ext[i].ext);
        }

        memset(pStatus->fileio_chb_ext[entries - 1].ext, 0x00, sizeof(file_ext_t));

    } else {
        pStatus->fileio_chb_ext = NULL; // == "*"
    }

    FreeList(valueList, entries);
    return 0;
}



uint8_t _CFG_pre_parse_handler(void* status, const ini_symbols_t section,
                               const ini_symbols_t name, const char* value)
{
    if (section == INI_SETUP) {
        switch (name) {
            case INI_BIN:
                return _CFG_handle_SETUP_BIN((status_t*)status, name, value);

            case INI_CLOCK:
                return _CFG_handle_SETUP_CLOCK((status_t*)status, name, value);

            case INI_CODER:
                return _CFG_handle_SETUP_CODER((status_t*)status, name, value);

            case INI_PHASE:
                return _CFG_handle_SETUP_PHASE((status_t*)status, name, value);

            case INI_VFILTER:
                return _CFG_handle_SETUP_VFILTER((status_t*)status, name, value);

            case INI_EN_TWI:
                return _CFG_handle_SETUP_EN_TWI((status_t*)status, name, value);

            case INI_EN_SPI:
                return _CFG_handle_SETUP_EN_SPI((status_t*)status, name, value);

            case INI_SPI_CLK:
                return _CFG_handle_SETUP_SPI_CLK((status_t*)status, name, value);

            case INI_BUTTON:
                return _CFG_handle_SETUP_BUTTON((status_t*)status, name, value);

            case INI_HOTKEY:
                return _CFG_handle_SETUP_HOTKEY((status_t*)status, name, value);

            case INI_CLOCKMON:
                return _CFG_handle_SETUP_CLOCKMON((status_t*)status, name, value);

            case INI_OSD_INIT:
                return _CFG_handle_SETUP_OSD_INIT((status_t*)status, name, value);

            case INI_OSD_TIMEOUT:
                return _CFG_handle_SETUP_OSD_TIMEOUT((status_t*)status, name, value);

            default:
                break;
        }

    } else if (section == INI_UPLOAD) {
        if (name == INI_VERIFY) {
            return _CFG_handle_UPLOAD_VERIFY((status_t*)status, name, value);
        }
    }


    return 0;
}


uint8_t CFG_pre_init(status_t* currentStatus, const char* iniFile)
{
    FF_FILE* fIni = NULL;

    DEBUG(2, "--- PRE-SETUP INI RUN ---");

    if (currentStatus->fs_mounted_ok) {

        DEBUG(1, "Looking for %s (pre-init)", iniFile);

        fIni = FF_Open(pIoman, iniFile, FF_MODE_READ, NULL);

        if (fIni) {

            uint8_t status = ParseIni(fIni, _CFG_pre_parse_handler, currentStatus);
            FF_Close(fIni);

            if (status != 0 ) {
                ERROR("Execution stopped at INI line %d", status);
                return 1;
            }

        } else {
            ERROR("INI file not found");
            return 1;

        }

    }

    // config hardware
    Configure_ClockGen(&currentStatus->clock_cfg);

    if (currentStatus->coder_fitted) {
        CFG_set_coder(currentStatus->coder_cfg);
    };

    Configure_VidBuf(1, currentStatus->filter_cfg.stc, currentStatus->filter_cfg.lpf, currentStatus->filter_cfg.mode);

    Configure_VidBuf(2, currentStatus->filter_cfg.stc, currentStatus->filter_cfg.lpf, currentStatus->filter_cfg.mode);

    Configure_VidBuf(3, currentStatus->filter_cfg.stc, currentStatus->filter_cfg.lpf, currentStatus->filter_cfg.mode);

    DEBUG(2, "PRE-INI processing done");

    return 0;
}

// ===========================================================================
// INI HANDLING: POST FPGA SETUP
// ===========================================================================
/*
Example INI file (post-setup part):
------------------------------------------------------------------------------
[SETUP]

# VGA/DVI settings, "EN_TWI" must be set to one !
#           0x1c 0x1d 0x1e 0x1f 0x20 0x21 0x23 0x31 0x33 0x34 0x35 0x36 0x37 0x48 0x49 0x56
video =     0x00,0x48,0xC0,0x80,0x00,0x01,0x00,0x80,0x08,0x16,0x30,0x60,0x00,0x18,0xC0,0x00

[UPLOAD]

# first to load (files relative to this ini file)
#
# name = file1.rom
# size = 0x4000
# addr = 0x12345678
ROM = file1.rom,0x4000,0x12345678

# start after previous ROM (or zero, if first entry), length given
#
# name = file2.rom
# size = 0x2000
# addr = 0x12345678+0x4000
ROM = file2.rom,0x2000

# arbitrary data upload (up to 16 byte), always to a given address
# (does not change ROM loader address and is thus idependent to use)
#
# list of data (bytes), address, number of bytes (= brief check of validity)
DATA = 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x98765432,8

# 2x32bit static+dynamic configuration bits, "CFG" in EN_SPI must be set to one!
#        31                             0,31                             0
config = 10101000111001101010100011100110,10101000111001101010100011100110

[MENU]

# menu entry
title = "Video Settings"

#         text            mask          static/dynamic (or just d/s)
item = "Filter",     0x00003000, dynamic

#              text       value           none/default
option = "H only", 0x00001000, default
option = "V only", 0x00002000
option = "H+V",    0x00003000

#         text      mask  shift    static/dynamic
item = "Filter",     3,   12,     dynamic

#              text       value   none/default
option = "H only",   1,        default
option = "V only",   2
option = "H+V",      3

------------------------------------------------------------------------------
*/

uint8_t _CFG_parse_handler(void* status, const ini_symbols_t section,
                           const ini_symbols_t name, const char* value)
{
    status_t* pStatus = (status_t*)status;

    if (section == INI_SETUP) {
        switch (name) {
            case INI_CONFIG:
                return _CFG_handle_SETUP_CONFIG(pStatus, name, value);

            case INI_INFO:
                return _CFG_handle_SETUP_INFO(pStatus, name, value);

            case INI_VIDEO:
                return _CFG_handle_SETUP_VIDEO(pStatus, name, value);

            default:
                break;
        }

    } else if (section == INI_TARGETS) {
        if (name == INI_TARGET) {
            return _CFG_handle_TARGETS_TARGET(pStatus, name, value);
        }

    } else if (section == INI_MENU) {
        switch (name) {
            case INI_TITLE:
                return _CFG_handle_MENU_TITLE(pStatus, name, value);

            case INI_ITEM:
                return _CFG_handle_MENU_ITEM(pStatus, name, value);

            case INI_OPTION:
                return _CFG_handle_MENU_OPTION(pStatus, name, value);

            default:
                break;
        }

    } else if (section == INI_UPLOAD) {
        switch (name) {
            case INI_DATA:
                return _CFG_handle_UPLOAD_DATA(pStatus, name, value);

            case INI_LAUNCH:
                return _CFG_handle_UPLOAD_LAUNCH(pStatus, name, value);

            case INI_ROM:
                return _CFG_handle_UPLOAD_ROM(pStatus, name, value);

            default:
                break;
        }

    } else if (section == INI_FILES) {
        switch (name) {
            case INI_CHA_MOUNT:
                return _CFG_handle_FILES_CHA_MOUNT(pStatus, name, value);

            case INI_CHB_MOUNT:
                return _CFG_handle_FILES_CHB_MOUNT(pStatus, name, value);

            case INI_CHA_CFG:
                return _CFG_handle_FILES_CHA_CFG(pStatus, name, value);

            case INI_CHB_CFG:
                return _CFG_handle_FILES_CHB_CFG(pStatus, name, value);

            default:
                break;
        }
    }

    return 0;
}

uint8_t CFG_init(status_t* currentStatus, const char* iniFile)
{
    //  uint32_t i = 0;
    //  uint16_t key = 0;
    //  char s[OSDMAXLEN]; // watch size

    // POST FPGA BOOT CONFIG
    DEBUG(2, "--- POST-SETUP INI RUN ---");

    // reset ROM address, in case it is initially not stated in INI
    currentStatus->last_rom_adr = 0;

    if (OSD_ConfigReadSysconVer() != 0xA5) {
        DEBUG(1, "!! Comms failure. FPGA Syscon not detected !!");
        return 1; // can do no more here
    }

    uint32_t config_ver        = OSD_ConfigReadVer();
    uint32_t config_stat       = OSD_ConfigReadStatus();

    uint32_t init_mem = CFG_get_free_mem();
    DEBUG(1, "Initial free MEM: %ld bytes", init_mem);

    // Set memory phase first
    if (!currentStatus->dram_phase) {
        OSD_ConfigSendCtrl((kDRAM_SEL << 8) | kDRAM_PHASE); // default phase

    } else {
        if (abs(currentStatus->dram_phase) < 21) {
            INFO("DRAM phase fix: %d -> %d", kDRAM_PHASE, kDRAM_PHASE + currentStatus->dram_phase);
            OSD_ConfigSendCtrl((kDRAM_SEL << 8) | (kDRAM_PHASE + currentStatus->dram_phase)); // phase from INI

        } else {
            WARNING("DRAM phase value bad, ignored!");
        }
    }

    // reset core and halt it for now
    OSD_Reset(OSDCMD_CTRL_RES | OSDCMD_CTRL_HALT);
    Timer_Wait(100);

    // initialize and check DDR RAM
    if (config_ver & 0x8000) {
        FPGA_DramTrain();
    }


    // PollL FPGA and see how many drives are supported
    uint32_t config_fileio_ena = OSD_ConfigReadFileIO_Ena();
    uint32_t config_fileio_drv = OSD_ConfigReadFileIO_Drv();

    currentStatus->fileio_cha_ena  =  config_fileio_ena       & 0x0F;
    currentStatus->fileio_cha_drv  =  config_fileio_drv       & 0x0F;
    currentStatus->fileio_cha_mode = (fileio_mode_t) REMOVABLE;

    currentStatus->fileio_chb_ena = (config_fileio_ena >> 4) & 0x0F;
    currentStatus->fileio_chb_drv = (config_fileio_drv >> 4) & 0x0F;
    currentStatus->fileio_chb_mode = (fileio_mode_t) REMOVABLE;

    DEBUG(1, "FileIO ChA supported : "BYTETOBINARYPATTERN4", Driver : %01X",
          BYTETOBINARY4(currentStatus->fileio_cha_ena), currentStatus->fileio_cha_drv);

    DEBUG(1, "FileIO ChB supported : "BYTETOBINARYPATTERN4", Driver : %01X",
          BYTETOBINARY4(currentStatus->fileio_chb_ena), currentStatus->fileio_chb_drv);

    currentStatus->fileio_cha_ext = NULL;
    currentStatus->fileio_chb_ext = NULL;

    // update status (all unmounted)
    FileIO_FCh_UpdateDriveStatus(0);
    FileIO_FCh_SetDriver(0, currentStatus->fileio_cha_drv); // temp

    FileIO_FCh_UpdateDriveStatus(1);
    FileIO_FCh_SetDriver(1, currentStatus->fileio_chb_drv); // temp

    // PARSE INI FILE
    if (currentStatus->fs_mounted_ok) {
        FF_FILE* fIni = NULL;
        DEBUG(1, "Looking for %s (post-init)", iniFile);

        fIni = FF_Open(pIoman, iniFile, FF_MODE_READ, NULL);

        if (fIni) {
            int32_t status = ParseIni(fIni, _CFG_parse_handler, currentStatus);
            FF_Close(fIni);

            if (status != 0 ) {
                ERROR("Execution stopped at INI line %d", status);
                return status;
            }

        } else {
            ERROR("INI file not found");
            return 1;
        }
    }

    init_mem -= CFG_get_free_mem();
    DEBUG(1, "Final free MEM:   %ld bytes", CFG_get_free_mem());

    if (init_mem) {
        DEBUG(0, "Menu requires %ld bytes", init_mem);
    }

    // for OSD
    sprintf(currentStatus->status[0], "ARM |FW:%s", version);
    sprintf(currentStatus->status[1], "FPGA|FW:%04X STAT:%04x IO:%04X",
            config_ver, config_stat, ((config_fileio_drv << 8) | config_fileio_ena));

    // TODO: ini path may exceed length and crash system here!
    sprintf(currentStatus->status[2], "INI |%s", iniFile);


    // check all config bits
    _MENU_update_bits(currentStatus);

    // reset core and remove halt
    OSD_Reset(OSDCMD_CTRL_RES);
    Timer_Wait(100);

    DEBUG(1, "POST-INI processing done, core started");

    // temp
    if (config_ver == 0x80FF ) {
        DEBUG(2, "Executing production test.");
        FPGA_ProdTest();
    }

    return 0;
}

void CFG_add_default(status_t* currentStatus)
{
    status_t* pStatus = (status_t*) currentStatus;

    // add default menu entry
    if (pStatus->menu_top) {   // add further entry
        // prepare next level and set pointers correctly
        pStatus->menu_act->next = malloc(sizeof(menu_t));
        // link back
        pStatus->menu_act->next->last = pStatus->menu_act;
        // step in linked list
        pStatus->menu_act = pStatus->menu_act->next;

    } else {                   // first top entry
        // prepare top level
        pStatus->menu_act = malloc(sizeof(menu_t));
        pStatus->menu_act->last = NULL;
        // set top level
        pStatus->menu_top = pStatus->menu_act;
    }

    pStatus->menu_act->next = NULL;
    strcpy(pStatus->menu_act->menu_title, "Replay Menu");

    pStatus->menu_act->item_list = malloc(sizeof(menuitem_t));
    pStatus->menu_item_act = pStatus->menu_act->item_list;
    pStatus->menu_item_act->next = NULL;
    pStatus->menu_item_act->last = NULL;

    strcpy(pStatus->menu_item_act->item_name, "Load Target");
    pStatus->menu_item_act->option_list = NULL;
    pStatus->menu_item_act->selected_option = NULL;
    pStatus->menu_item_act->conf_dynamic = 0;
    pStatus->menu_item_act->conf_mask = 0;
    strcpy(pStatus->menu_item_act->action_name, "iniselect");
    pStatus->menu_item_act->option_list = malloc(sizeof(itemoption_t));
    pStatus->item_opt_act = pStatus->menu_item_act->option_list;
    pStatus->item_opt_act->next = NULL;
    pStatus->item_opt_act->last = NULL;
    pStatus->item_opt_act->option_name[0] = 0;

    pStatus->menu_item_act->next = malloc(sizeof(menuitem_t));
    pStatus->menu_item_act->next->last = pStatus->menu_item_act;
    pStatus->menu_item_act = pStatus->menu_item_act->next;
    pStatus->menu_item_act->next = NULL;

    strcpy(pStatus->menu_item_act->item_name, "Reset Target");
    pStatus->menu_item_act->option_list = NULL;
    pStatus->menu_item_act->selected_option = NULL;
    pStatus->menu_item_act->conf_dynamic = 0;
    pStatus->menu_item_act->conf_mask = 0;
    strcpy(pStatus->menu_item_act->action_name, "reset");
    pStatus->menu_item_act->option_list = malloc(sizeof(itemoption_t));
    pStatus->item_opt_act = pStatus->menu_item_act->option_list;
    pStatus->item_opt_act->next = NULL;
    pStatus->item_opt_act->last = NULL;
    pStatus->item_opt_act->option_name[0] = 0;

    /*pStatus->menu_item_act = pStatus->menu_item_act->next;*/
    /*strcpy(pStatus->menu_item_act->item_name,"Save Config");*/
    /*pStatus->menu_item_act->next = malloc(sizeof(menuitem_t));*/
    /*pStatus->menu_item_act->next->last=pStatus->menu_item_act;*/
    /*pStatus->menu_item_act->option_list=NULL;*/
    /*pStatus->menu_item_act->selected_option=NULL;*/
    /*pStatus->menu_item_act->conf_dynamic=0;*/
    /*pStatus->menu_item_act->conf_mask=0;*/
    /*strcpy(pStatus->menu_item_act->action_name,"backup");*/
    /*pStatus->menu_item_act->option_list=malloc(sizeof(itemoption_t));*/
    /*pStatus->item_opt_act = pStatus->menu_item_act->option_list;*/
    /*pStatus->item_opt_act->next=NULL;*/
    /*pStatus->item_opt_act->last=NULL;*/
    /*pStatus->item_opt_act->option_name[0]=0;*/

    pStatus->menu_item_act->next = malloc(sizeof(menuitem_t));
    pStatus->menu_item_act->next->last = pStatus->menu_item_act;
    pStatus->menu_item_act = pStatus->menu_item_act->next;
    pStatus->menu_item_act->next = NULL;

    strcpy(pStatus->menu_item_act->item_name, "Reboot Board");
    pStatus->menu_item_act->option_list = NULL;
    pStatus->menu_item_act->selected_option = NULL;
    pStatus->menu_item_act->conf_dynamic = 0;
    pStatus->menu_item_act->conf_mask = 0;
    strcpy(pStatus->menu_item_act->action_name, "reboot");
    pStatus->menu_item_act->option_list = malloc(sizeof(itemoption_t));
    pStatus->item_opt_act = pStatus->menu_item_act->option_list;
    pStatus->item_opt_act->next = NULL;
    pStatus->item_opt_act->last = NULL;
    pStatus->item_opt_act->option_name[0] = 0;

    pStatus->menu_item_act->next = malloc(sizeof(menuitem_t));
    pStatus->menu_item_act->next->last = pStatus->menu_item_act;
    pStatus->menu_item_act = pStatus->menu_item_act->next;
    pStatus->menu_item_act->next = NULL;

    strcpy(pStatus->menu_item_act->item_name, "Mount SDCard");
    pStatus->menu_item_act->option_list = NULL;
    pStatus->menu_item_act->selected_option = NULL;
    pStatus->menu_item_act->conf_dynamic = 0;
    pStatus->menu_item_act->conf_mask = 0;
    strcpy(pStatus->menu_item_act->action_name, "mountmsc");
    pStatus->menu_item_act->option_list = malloc(sizeof(itemoption_t));
    pStatus->item_opt_act = pStatus->menu_item_act->option_list;
    pStatus->item_opt_act->next = NULL;
    pStatus->item_opt_act->last = NULL;
    pStatus->item_opt_act->option_name[0] = 0;

    // Add ini_targets - TODO : this should probably go on a separate menu screen..
    for (tIniTarget* it = pStatus->ini_targets; it != NULL; it = it->next) {
        DEBUG(3, "_CFG_add_default: adding ini_target %s", it->name);

        pStatus->menu_item_act->next = malloc(sizeof(menuitem_t));
        pStatus->menu_item_act->next->last = pStatus->menu_item_act;
        pStatus->menu_item_act = pStatus->menu_item_act->next;
        pStatus->menu_item_act->next = NULL;

        strcpy(pStatus->menu_item_act->item_name, it->name);

        pStatus->menu_item_act->option_list = NULL;
        pStatus->menu_item_act->selected_option = NULL;
        pStatus->menu_item_act->conf_dynamic = 0;
        pStatus->menu_item_act->conf_mask = 0;
        strcpy(pStatus->menu_item_act->action_name, "iniload");

        pStatus->menu_item_act->option_list = malloc(sizeof(itemoption_t));

        pStatus->item_opt_act = pStatus->menu_item_act->option_list;
        pStatus->item_opt_act->next = it->dir ? malloc(sizeof(itemoption_t)) : NULL;
        pStatus->item_opt_act->last = NULL;
        strcpy(pStatus->item_opt_act->option_name, it->file);

        if (it->dir != NULL && pStatus->item_opt_act->next != NULL) {
            pStatus->item_opt_act = pStatus->item_opt_act->next;
            pStatus->item_opt_act->next = NULL;
            pStatus->item_opt_act->last = NULL;
            strcpy(pStatus->item_opt_act->option_name, it->dir);
        }
    }
}

void CFG_free_menu(status_t* currentStatus)
{
    DEBUG(2, "--- FREE MENU (and backup) SPACE ---");

    currentStatus->menu_act = currentStatus->menu_top;

    while (currentStatus->menu_act) {
        void* p;
        DEBUG(3, "T:%08lx >%08lx <%08lx ><%08lx", currentStatus->menu_act,
              currentStatus->menu_act->next, currentStatus->menu_act->last,
              currentStatus->menu_act->next->last);
        currentStatus->menu_item_act = currentStatus->menu_act->item_list;

        while (currentStatus->menu_item_act) {
            DEBUG(3, "I:%08lx >%08lx <%08lx ><%08lx", currentStatus->menu_item_act,
                  currentStatus->menu_item_act->next,
                  currentStatus->menu_item_act->last,
                  currentStatus->menu_item_act->next->last);
            currentStatus->item_opt_act = currentStatus->menu_item_act->option_list;

            while (currentStatus->item_opt_act) {
                DEBUG(3, "O:%08lx >%08lx <%08lx ><%08lx",
                      currentStatus->item_opt_act, currentStatus->item_opt_act->next,
                      currentStatus->item_opt_act->last,
                      currentStatus->item_opt_act->next->last);
                p = currentStatus->item_opt_act;
                currentStatus->item_opt_act = currentStatus->item_opt_act->next;
                free(p);
            }

            p = currentStatus->menu_item_act;
            currentStatus->menu_item_act = currentStatus->menu_item_act->next;
            free(p);
        }

        p = currentStatus->menu_act;
        currentStatus->menu_act = currentStatus->menu_act->next;
        free(p);
    }

    if (currentStatus->fileio_cha_ext) {
        free(currentStatus->fileio_cha_ext);
    }

    if (currentStatus->fileio_chb_ext) {
        free(currentStatus->fileio_chb_ext);
    }

    currentStatus->fileio_cha_ext = NULL;
    currentStatus->fileio_chb_ext = NULL;

    currentStatus->menu_top = NULL;
    currentStatus->menu_act = NULL;
    currentStatus->menu_item_act = NULL;
    currentStatus->item_opt_act = NULL;
}

/*
void CFG_free_bak(status_t *currentStatus)
{
  currentStatus->rom_bak_last=currentStatus->rom_bak;
  while (currentStatus->rom_bak_last) {
    rom_list_t *p = currentStatus->rom_bak_last->next;
    free(currentStatus->rom_bak_last);
    currentStatus->rom_bak_last=p;
  }
  currentStatus->rom_bak=NULL;
  currentStatus->rom_bak_last=NULL;

  currentStatus->data_bak_last=currentStatus->data_bak;
  while (currentStatus->data_bak_last) {
    data_list_t *p = currentStatus->data_bak_last->next;
    free(currentStatus->data_bak_last);
    currentStatus->data_bak_last=p;
  }
  currentStatus->data_bak=NULL;
  currentStatus->data_bak_last=NULL;

  currentStatus->info_bak_last=currentStatus->info_bak;
  while (currentStatus->info_bak_last) {
    info_list_t *p = currentStatus->info_bak_last->next;
    free(currentStatus->info_bak_last);
    currentStatus->info_bak_last=p;
  }
  currentStatus->info_bak=NULL;
  currentStatus->info_bak_last=NULL;

  currentStatus->clock_bak[0]=0;
  currentStatus->coder_bak[0]=0;
  currentStatus->vfilter_bak[0]=0;
  currentStatus->video_bak[0]=0;

  currentStatus->spiclk_bak=0;
  if (currentStatus->spiclk_old) {
    AT91C_BASE_SPI->SPI_CSR[0] = AT91C_SPI_CPOL |
                                  (currentStatus->spiclk_old<<8);
  }
  currentStatus->spiclk_old=0;
}
*/

void _CFG_write_str(FF_FILE* fIni, char* str)
{
    uint32_t len = strlen(str);
    FF_Write (fIni, len, 1, (uint8_t*)str);
}

void _CFG_write_strln(FF_FILE* fIni, char* str)
{
    _CFG_write_str(fIni, str);
    _CFG_write_str(fIni, "\r\n");
}

void CFG_save_all(status_t* currentStatus, const char* iniDir,
                  const char* iniFile)
{
    /*
    FF_FILE *fIni = NULL;
    char full_filename[FF_MAX_PATH];
    sprintf(full_filename,"%s%s",iniDir,iniFile);

    if (currentStatus->fs_mounted_ok) {
      DEBUG(1,"Writing %s",full_filename);

      fIni = FF_Open(pIoman, full_filename,
                     FF_MODE_WRITE|FF_MODE_CREATE|FF_MODE_TRUNCATE, NULL);

      if(fIni) {
        char s[128];

        // SETUP section
        _CFG_write_strln(fIni,"[SETUP]");
        sprintf(s,"bin = %s",currentStatus->bin_file);
        _CFG_write_strln(fIni,s);

        sprintf(s,"clock = %s",currentStatus->clock_bak);
        _CFG_write_strln(fIni,s);

        sprintf(s,"coder = %s",currentStatus->coder_bak);
        _CFG_write_strln(fIni,s);

        sprintf(s,"vfilter = %s",currentStatus->vfilter_bak);
        _CFG_write_strln(fIni,s);

        sprintf(s,"en_twi = %d",currentStatus->twi_enabled);
        _CFG_write_strln(fIni,s);

        sprintf(s,"en_spi = %d,%d",currentStatus->spi_fpga_enabled,
                                   currentStatus->spi_osd_enabled);
        _CFG_write_strln(fIni,s);

        if (currentStatus->spiclk_bak) {
          sprintf(s,"spi_clk = %ld",currentStatus->spiclk_bak);
          _CFG_write_strln(fIni,s);
        }

        sprintf(s,"button = %s",currentStatus->button==BUTTON_RESET?"reset": (
                                currentStatus->button==BUTTON_OFF?"off":"menu"));
        _CFG_write_strln(fIni,s);

        sprintf(s,"video = %s",currentStatus->video_bak);
        _CFG_write_strln(fIni,s);

        sprintf(s,"config = 0x%08lx,0x%08lx",currentStatus->config_s
                                        ,currentStatus->config_d);
        _CFG_write_strln(fIni,s);

        currentStatus->info_bak_last=currentStatus->info_bak;
        while (currentStatus->info_bak_last) {
          sprintf(s,"info = %s",currentStatus->info_bak_last->info_bak);
          _CFG_write_strln(fIni,s);
          currentStatus->info_bak_last=currentStatus->info_bak_last->next;
        }

        // UPLOAD section
        _CFG_write_strln(fIni,"");
        _CFG_write_strln(fIni,"[UPLOAD]");

        sprintf(s,"verify = 0");
        _CFG_write_strln(fIni,s);

        currentStatus->rom_bak_last=currentStatus->rom_bak;
        while (currentStatus->rom_bak_last) {
          sprintf(s,"rom = %s",currentStatus->rom_bak_last->rom_bak);
          _CFG_write_strln(fIni,s);
          currentStatus->rom_bak_last=currentStatus->rom_bak_last->next;
        }

        currentStatus->data_bak_last=currentStatus->data_bak;
        while (currentStatus->data_bak_last) {
          sprintf(s,"data = %s",currentStatus->data_bak_last->data_bak);
          _CFG_write_strln(fIni,s);
          currentStatus->data_bak_last=currentStatus->data_bak_last->next;
        }

        // MENU section
        _CFG_write_strln(fIni,"");
        if (currentStatus->menu_top) {
          menu_t *menu_act = currentStatus->menu_top;
          menuitem_t *item_act;
          itemoption_t *option_act;

          _CFG_write_strln(fIni,"[MENU]");
          while (menu_act && menu_act->item_list) {
            if (menu_act && (!MATCH(menu_act->menu_title,"Replay Menu"))) {
              sprintf(s,"title = \"%s\"",menu_act->menu_title);
              _CFG_write_strln(fIni,s);

              item_act = menu_act->item_list;
              while (item_act && item_act->option_list) {
                sprintf(s,"item = \"%s\",0x%08lx,%s",item_act->item_name,
                          item_act->conf_mask,
                          item_act->conf_dynamic?"dynamic":"static");
                _CFG_write_strln(fIni,s);

                option_act = item_act->option_list;
                while (option_act && option_act->option_name[0]) {
                  sprintf(s,"option = \"%s\",0x%08lx%s",option_act->option_name,
                            option_act->conf_value,
                            option_act==item_act->selected_option?",default":"");
                  _CFG_write_strln(fIni,s);

                  option_act = option_act->next;
                }
                item_act = item_act->next;
              }
            }
            menu_act = menu_act->next;
          }
        }

        FF_Close(fIni);
      }
    }
    */
}
