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

#include "fileio_drv.h"
#include "hardware/spi.h"
#include "messaging.h"

const uint8_t DRV02_DEBUG = 1;

#define DRV02_STAT_REQ_ACK             0x01
#define DRV02_STAT_TRANS_ACK_OK        0x02
#define DRV02_STAT_TRANS_ACK_SEEK_ERR  0x12
#define DRV02_STAT_TRANS_ACK_TRUNC_ERR 0x22
#define DRV02_STAT_TRANS_ACK_ABORT_ERR 0x42

#define DRV02_BUF_SIZE 512

#define UEF_ChunkHeaderSize (sizeof(uint16_t) + sizeof(uint32_t))
#define UEF_infoID      0x0000
#define UEF_tapeID      0x0100
#define UEF_highToneID  0x0110
#define UEF_highDummyID 0x0111
#define UEF_gapID       0x0112
#define UEF_freqChgID   0x0113
#define UEF_securityID  0x0114
#define UEF_floatGapID  0x0116
#define UEF_startBit    0
#define UEF_stopBit     1
#define UEF_Baud        (1000000.0/(16.0*52.0))

typedef enum {
    XXX, // unsupported
    RAW,
    UEF
} drv02_format_t;

typedef struct {
    drv02_format_t  format;
    uint32_t        file_size;
} drv02_desc_t;

typedef struct {
    // UEF header
    uint16_t    id;
    uint32_t    length;
    // chunk data
    uint32_t    file_offset;
    uint32_t    bit_offset_start;
    uint32_t    bit_offset_end;
    uint32_t    pre_carrier;
} __attribute__((packed)) ChunkInfo;

// TODO - allocate this as part of the desc structure
static ChunkInfo s_ChunkData = { 0 };

void FileIO_Drv02_RAW_Process(uint8_t ch, fch_t* pDrive, uint8_t drive_number, uint8_t dir, uint8_t* fbuf, uint32_t addr, uint16_t size)
{
    uint16_t cur_size = 0;
    uint32_t act_size = 0;

    if (FF_Seek(pDrive->fSource, addr, FF_SEEK_SET)) {
        WARNING("Drv02:Seek error");
        FileIO_FCh_WriteStat(ch, DRV02_STAT_TRANS_ACK_SEEK_ERR);
        return;
    }

    while (size) {
        cur_size = size;

        if (cur_size > DRV02_BUF_SIZE) {
            cur_size = DRV02_BUF_SIZE;
        }

        if (DRV02_DEBUG) {
            DEBUG(1, "Drv02:Process Ch%d Drive:%02X Addr:%08X Size:%04X", ch, drive_number, addr, cur_size);
        }

        // align to 512 byte boundaries if possible
        uint32_t addr_offset = addr & 0x1FF;

        if (addr_offset != 0) {
            if (DRV02_DEBUG) {
                DEBUG(1, "Drv02:non-aligned:%08X", addr);
            }

            addr_offset = 0x200 - addr_offset;

            if (cur_size > addr_offset) {
                cur_size = addr_offset;

                if (DRV02_DEBUG) {
                    DEBUG(1, "Drv02:new size:%04X", cur_size);
                }
            }
        }

        if (dir) { // write
            // request should not be asserted if data is not ready
            // write will fail if read only
            if (pDrive->status & FILEIO_STAT_READONLY_OR_PROTECTED) {
                WARNING("Drv02:W Read only disk!");
                FileIO_FCh_WriteStat(ch, DRV02_STAT_TRANS_ACK_TRUNC_ERR); // truncated
                return;
            }

            SPI_EnableFileIO();
            rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_FIFO_R));
            SPI_ReadBufferSingle(fbuf, cur_size);
            SPI_DisableFileIO();
            /*DumpBuffer(fbuf,cur_size);*/

            act_size = FF_Write(pDrive->fSource, cur_size, 1, fbuf);

            if (act_size != cur_size) {
                WARNING("Drv02:!! Write Fail!!");
                FileIO_FCh_WriteStat(ch, DRV02_STAT_TRANS_ACK_TRUNC_ERR); // truncated
                return;
            }

        } else {
            // enough faffing, do the read
            act_size = FF_Read(pDrive->fSource, cur_size, 1, fbuf);

            if (DRV02_DEBUG) {
                DEBUG(1, "Drv02:bytes read:%04X", act_size);
            }

            /*DumpBuffer(fbuf, cur_size);*/

            SPI_EnableFileIO();
            rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_FIFO_W));
            SPI_WriteBufferSingle(fbuf, act_size);
            SPI_DisableFileIO();

            if (act_size != cur_size) {
                FileIO_FCh_WriteStat(ch, DRV02_STAT_TRANS_ACK_TRUNC_ERR); // truncated
                return;
            }
        }

        addr += cur_size;
        size -= cur_size;

        // check to see if we can send/rx more
        // NOTE, this stalls the GUI so not recommeded to use the flow control here
        if (size) { // going again
            if (dir) { // write to arm
                if (FileIO_FCh_WaitStat(ch, FILEIO_REQ_OK_TO_ARM, FILEIO_REQ_OK_TO_ARM)) {
                    FileIO_FCh_WriteStat(ch, DRV02_STAT_TRANS_ACK_ABORT_ERR); // err
                    return;
                }

            } else {
                if (FileIO_FCh_WaitStat(ch, FILEIO_REQ_OK_FM_ARM, FILEIO_REQ_OK_FM_ARM)) {
                    FileIO_FCh_WriteStat(ch, DRV02_STAT_TRANS_ACK_ABORT_ERR); // err
                    return;
                }
            }
        } // end of wait

    }  // end of transfer loop

    // signal transfer done
    FileIO_FCh_WriteStat(ch, DRV02_STAT_TRANS_ACK_OK); // ok
}

// *** UEF -.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.

// wrapper helpers - TODO remove..
static size_t fread ( void* ptr, size_t size, size_t count, FF_FILE* stream )
{
    return FF_Read(stream, size, count, ptr);
}
static int fseek ( FF_FILE* stream, long int offset, int origin )
{
    return FF_Seek(stream, offset, origin);
}

static uint16_t ReadChunkHeader(FF_FILE* f, ChunkInfo* chunk)
{
    chunk->id = -1;
    chunk->length = 0;

    if (fread(chunk, 1, UEF_ChunkHeaderSize, f) != UEF_ChunkHeaderSize) {
        return (-1);
    }

    return chunk->id;
}

static ChunkInfo* GetChunkAtPosFile(FF_FILE* f, uint32_t* p_bit_pos)
{
    uint32_t bit_pos = *p_bit_pos;
    uint8_t length_check = (bit_pos == 0xffffffff);
    ChunkInfo* chunk = &s_ChunkData;

    /*DEBUG(1, "Find pos : %d", bit_pos);*/

    if (chunk->bit_offset_start <= bit_pos && bit_pos < chunk->bit_offset_end) {
        bit_pos -= chunk->bit_offset_start;
        *p_bit_pos = bit_pos;
        return chunk;
    }

    uint32_t chunk_start = 0;
    uint32_t chunk_bitlen;

    if (chunk->bit_offset_end != 0 && bit_pos >= chunk->bit_offset_end) {
        fseek(f, chunk->file_offset + chunk->length, FF_SEEK_SET);
        chunk_start = chunk->bit_offset_end;
        bit_pos -= chunk_start;

    } else {
        fseek(f, 12, FF_SEEK_SET);     // sizeof(UEF_header)
    }

    chunk->bit_offset_end = 0;

    while (!FF_isEOF(f)) {
        uint16_t id = ReadChunkHeader(f, chunk);

        if (id == (uint16_t) - 1) {
            break;
        }

        /*DEBUG(1, "Parse ChunkID : %04x - Length : %4d bytes (%04x) - Offset = %d", chunk->id, chunk->length, chunk->length, (uint32_t)FF_Tell(f));*/
        chunk->file_offset = FF_Tell(f);

        if (UEF_tapeID == id || UEF_gapID == id || UEF_highToneID == id || UEF_highDummyID == id) {

            if (id == UEF_tapeID) {
                chunk_bitlen = chunk->length * 10;

            } else if (id == UEF_gapID || id == UEF_highToneID) {
                uint16_t ms;

                if (fread(&ms, 1, sizeof(ms), f) != sizeof(ms)) {
                    break;
                }

                chunk_bitlen = ms * (UEF_Baud / 1000.0);
                fseek(f, -sizeof(ms), FF_SEEK_CUR);

            } else if (id == UEF_highDummyID) {
                uint16_t ms;

                if (fread(&ms, 1, sizeof(ms), f) != sizeof(ms)) {
                    break;
                }

                chunk->pre_carrier = ms * (UEF_Baud / 1000.0);

                if (fread(&ms, 1, sizeof(ms), f) != sizeof(ms)) {
                    break;
                }

                uint32_t post_carrier = ms * (UEF_Baud / 1000.0);
                chunk_bitlen = chunk->pre_carrier + 20 + post_carrier;
                fseek(f, -sizeof(ms) * 2, FF_SEEK_CUR);
            }

            if (bit_pos < chunk_bitlen) {
                chunk->bit_offset_start = chunk_start;
                chunk->bit_offset_end = chunk_start + chunk_bitlen;
                break;
            }

            bit_pos -= chunk_bitlen;
            chunk_start += chunk_bitlen;

        } else if (length_check) {

            if (UEF_infoID == id) {
                char buffer[64];
                uint32_t length = chunk->length;

                while (length > 0) {
                    uint32_t read_len = length;

                    if (read_len > sizeof(buffer) - 1) {
                        read_len = sizeof(buffer) - 1;
                    }

                    if (fread(buffer, 1, read_len, f) != read_len) {
                        break;
                    }

                    buffer[read_len] = '\0';
                    DEBUG(0, "Drv02:UEF Info : '%s'", buffer);

                    length -= read_len;
                }

            } else if (UEF_freqChgID == id) {
                float freq;

                if (fread(&freq, 1, sizeof(freq), f) != sizeof(freq)) {
                    break;
                }

                DEBUG(0, "Drv02:Ignoring base frequency change : %d", (int)freq);

            } else if (UEF_floatGapID == id) {
                float gap;

                if (fread(&gap, 1, sizeof(gap), f) != sizeof(gap)) {
                    break;
                }

                DEBUG(0, "Drv02:Ignoring floating point gap : %d ms", (int)(gap * 1000.f));

            } else if (UEF_securityID == id) {

                DEBUG(0, "Drv02:UEF security block ignored");

            } else {
                DEBUG(0, "Drv02:Unknown UEF block ID %04x", id);
            }

            fseek(f, chunk->file_offset, FF_SEEK_SET);
        }

        fseek(f, chunk->length, FF_SEEK_CUR);
    }

    /*DEBUG(1, "OK!");*/

    *p_bit_pos = bit_pos;
    return chunk->bit_offset_end ? chunk : 0;
}

static uint8_t GetBitAtPos(FF_FILE* f, uint32_t bit_pos)
{
    ChunkInfo* info = GetChunkAtPosFile(f, &bit_pos);

    if (!info) {
        return 0;
    }

    uint16_t id = info->id;

    if (id == UEF_gapID) {
        return 0;

    } else if (id == UEF_highToneID) {
        return 1;
    }

    if (id == UEF_tapeID) {

        uint32_t byte_offset = bit_pos / 10;
        uint32_t bit_offset = bit_pos - byte_offset * 10;

        if (bit_offset == 0) {
            return UEF_startBit;
        }

        if (bit_offset == 9) {
            return UEF_stopBit;
        }

        uint8_t byte;
        fseek(f, info->file_offset + byte_offset, FF_SEEK_SET);
        fread(&byte, 1, sizeof(byte), f);

        bit_offset -= 1;        // E (0,7)
        Assert(bit_offset < 8);

        return (byte & (1 << bit_offset)) ? 1 : 0;
    }

    Assert(id == UEF_highDummyID);

    if ((bit_pos < info->pre_carrier) || (bit_pos >= info->pre_carrier + 20)) {
        return 1;
    }

    bit_pos -= info->pre_carrier;
    bit_pos %= 10;

    if (bit_pos == 0) {
        return UEF_startBit;
    }

    if (bit_pos == 9) {
        return UEF_stopBit;
    }

    bit_pos -= 1;       // E (0,7)
    Assert(bit_pos < 8);
    uint8_t byte = 'A';

    return (byte & (1 << bit_pos)) ? 1 : 0;
}

// *** UEF ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void FileIO_Drv02_UEF_Process(uint8_t ch, fch_t* pDrive, uint8_t drive_number, uint8_t dir, uint8_t* fbuf, uint32_t addr, uint16_t size)
{
    uint16_t cur_size = 0;
    uint32_t act_size = 0;
    drv02_desc_t* pDesc = pDrive->pDesc;

    if (dir) {    // write not supported
        WARNING("UEF Write not supported!");
        return;
    }

    // out-of-bounds read offsets trap here
    if (addr > pDesc->file_size) {
        addr = pDesc->file_size;
    }

    while (size) {
        cur_size = size;

        if (cur_size > DRV02_BUF_SIZE) {
            cur_size = DRV02_BUF_SIZE;
        }

        act_size = cur_size;

        // artifically clamp size to the end of the bit stream
        if ((addr + act_size) > pDesc->file_size) {
            act_size = pDesc->file_size - addr;
        }

        if (DRV02_DEBUG) {
            DEBUG(1, "Drv02:Process Ch%d Drive:%02X Addr:%08X Size:%04X", ch, drive_number, addr, cur_size);
        }

        HARDWARE_TICK tick = Timer_Get(0);

        // this is a very naive conversion, but it'll have to do for now..
        for (uint32_t pos = 0; pos < act_size; ++pos) {
            uint8_t val = 0;

            for (uint32_t bit = 0; bit < 8; ++bit) {
                val = val << 1;
                val = val | GetBitAtPos(pDrive->fSource, ((addr + pos) << 3) + bit);
            }

            fbuf[pos] = val;
        }

        uint32_t ms = Timer_Convert(Timer_Get(0) - tick);
        (void)ms;
        DEBUG(0, "Drv02:Process %04X bytes read/converted in %d ms. (%d bps)", cur_size, ms, 1000 * cur_size / ms);

        if (DRV02_DEBUG) {
            DEBUG(1, "Drv02:bytes read:%04X", act_size);
        }

        /*DumpBuffer(fbuf, cur_size);*/

        SPI_EnableFileIO();
        rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_FIFO_W));
        SPI_WriteBufferSingle(fbuf, act_size);
        SPI_DisableFileIO();

        if (act_size != cur_size) {
            FileIO_FCh_WriteStat(ch, DRV02_STAT_TRANS_ACK_TRUNC_ERR); // truncated
            return;
        }

        addr += cur_size;
        size -= cur_size;

    }  // end of transfer loop

    // signal transfer done
    FileIO_FCh_WriteStat(ch, DRV02_STAT_TRANS_ACK_OK); // ok

}

void FileIO_Drv02_Process(uint8_t ch, fch_t handle[2][FCH_MAX_NUM], uint8_t status) // amiga
{
    static int error_shown = 0;

    // file buffer
    uint8_t  fbuf[DRV02_BUF_SIZE];

    uint8_t  dir          = 0;
    uint8_t  drive_number = 0;
    uint16_t size = 0;
    uint32_t addr = 0;

    uint8_t  goes = FILEIO_PROCESS_LIMIT; // limit number of requests.

    do {
        dir          = (status >> 2) & 0x01; // high is write
        drive_number = (status >> 4) & 0x03;

        // validate request
        if (!FileIO_FCh_GetInserted(ch, drive_number)) {
            if (!error_shown) {
                DEBUG(1, "Drv02:Process Ch:%d Drive:%d not mounted", ch, drive_number);
                error_shown = 1;
            }

            FileIO_FCh_WriteStat(ch, DRV02_STAT_REQ_ACK); // ack
            FileIO_FCh_WriteStat(ch, DRV02_STAT_TRANS_ACK_ABORT_ERR); // err
            return;

        } else {
            error_shown = 0;
        }

        fch_t* pDrive = (fch_t*) &handle[ch][drive_number]; // get base
        drv02_desc_t* pDesc = pDrive->pDesc;

        SPI_EnableFileIO();
        rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_CMD_R | 0x0));
        rSPI(0x00); // dummy
        size  =  rSPI(0);
        size |= (rSPI(0) << 8);
        addr  =  rSPI(0);
        addr |= (rSPI(0) << 8);
        addr |= (rSPI(0) << 16);
        addr |= (rSPI(0) << 24);
        SPI_DisableFileIO();

        FileIO_FCh_WriteStat(ch, DRV02_STAT_REQ_ACK); // ack

        if (size > 0x2000) {
            DEBUG(1, "Drv02:warning large size request:%04X", size);
        }

        if (pDesc->format == RAW) {
            FileIO_Drv02_RAW_Process(ch, pDrive, drive_number, dir, fbuf, addr, size);
        }

        if (pDesc->format == UEF) {
            FileIO_Drv02_UEF_Process(ch, pDrive, drive_number, dir, fbuf, addr, size);
        }

        // any more to do?
        status = FileIO_FCh_GetStat(ch);
        goes --;
    } while ((status & FILEIO_REQ_ACT) && goes);
}

uint8_t FileIO_Drv02_InsertInit(uint8_t ch, uint8_t drive_number, fch_t* pDrive, char* ext)
{
    DEBUG(1, "Drv02:InsertInit");

    pDrive->pDesc = calloc(1, sizeof(drv02_desc_t)); // 0 everything
    memset(&s_ChunkData, 0x00, sizeof(ChunkInfo));

    if (pDrive->pDesc == NULL) {
        WARNING("Drv02:Failed to allocate memory.");
        return (1);
    }

    drv02_desc_t* pDesc = pDrive->pDesc;

    pDesc->file_size =  pDrive->fSource->Filesize;

    if (strnicmp(ext, "RAW", 3) == 0) {
        pDesc->format = RAW;

    } else if (strnicmp(ext, "UEF", 3) == 0) {

        typedef struct {
            char    ueftag[10];
            uint8_t minor_version;
            uint8_t major_version;
        } UEF_header;
        UEF_header header;

        FF_Seek(pDrive->fSource, 0, FF_SEEK_SET);

        if (fread(&header, 1, sizeof(UEF_header), pDrive->fSource) != sizeof(UEF_header)) {
            ERROR("Couldn't read file header");

        } else if (memcmp(header.ueftag, "UEF File!\0", sizeof(header.ueftag)) != 0) {
            ERROR("UEF file header mismatch");
            WARNING("File compressed?");

        } else {
            
            pDesc->format = UEF;

            HARDWARE_TICK before = Timer_Get(0);
            uint32_t numbits = 0xffffffff;
            GetChunkAtPosFile(pDrive->fSource, &numbits);
            DEBUG(1, "Drv02: GetChunkAtPosFile in %d ms.", Timer_Convert(Timer_Get(0) - before));

            numbits = ~numbits;

            uint32_t bits_per_second = 1225;
            DEBUG(1, "Bit length  : %d", numbits);
            DEBUG(1, "Wave length : %ds", numbits / bits_per_second);
            DEBUG(1, "Byte length : %d", (numbits + 7) / 8);

            pDesc->file_size = (numbits + 7) / 8;
        }
    }

    // NOTE, core may still be in reset
    // select drive
    SPI_EnableFileIO();
    rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_CMD_W | 0x0));
    rSPI(drive_number);
    SPI_DisableFileIO();

    // write file size
    SPI_EnableFileIO();
    rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_CMD_W | 0x4));
    rSPI((uint8_t)(pDesc->file_size      ));
    rSPI((uint8_t)(pDesc->file_size >>  8));
    rSPI((uint8_t)(pDesc->file_size >> 16));
    rSPI((uint8_t)(pDesc->file_size >> 24));
    SPI_DisableFileIO();

    DEBUG(1, "Drv02:Size   : %lu (%lu kB)", pDesc->file_size, pDesc->file_size);

    return (0);
}
