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

const uint8_t DRV00_DEBUG = 0;
#define DRV00_DEBUG_STATS 0

#define DRV00_STAT_REQ_ACK             0x01
#define DRV00_STAT_TRANS_ACK_OK        0x02
#define DRV00_STAT_TRANS_ACK_SEEK_ERR  0x12
#define DRV00_STAT_TRANS_ACK_TRUNC_ERR 0x22
#define DRV00_STAT_TRANS_ACK_ABORT_ERR 0x42

#define DRV00_BUF_SIZE 512
#define DRV00_BLK_SIZE 512

typedef struct {
    uint32_t file_size;
} drv00_desc_t;

void FileIO_Drv00_Process(uint8_t ch, fch_t handle[2][FCH_MAX_NUM], uint8_t status) // amiga
{
    static int error_shown = 0;

    // file buffer
    uint8_t  fbuf[DRV00_BUF_SIZE];

    uint8_t  dir          = 0;
    uint8_t  drive_number = 0;
    uint16_t size = 0;
    uint16_t cur_size = 0;
    uint32_t addr = 0;
    uint32_t act_size = 0;

    uint8_t  goes = FILEIO_PROCESS_LIMIT; // limit number of requests.

    // stats
#if DRV00_DEBUG_STATS
    uint32_t stats_bytes_processed = 0;
    uint32_t stats_seek_ticks = 0;
    uint32_t stats_read_ticks = 0;
    uint32_t stats_send_ticks = 0;

    HARDWARE_TICK stats_begin = Timer_Get(0);
#endif

    do {
        dir          = (status >> 2) & 0x01; // high is write
        drive_number = (status >> 4) & 0x03;

        // validate request
        if (!FileIO_FCh_GetInserted(ch, drive_number)) {
            if (!error_shown) {
                DEBUG(1, "Drv00:Process Ch:%d Drive:%d not mounted", ch, drive_number);
                error_shown = 1;
            }

            FileIO_FCh_WriteStat(ch, DRV00_STAT_REQ_ACK); // ack
            FileIO_FCh_WriteStat(ch, DRV00_STAT_TRANS_ACK_ABORT_ERR); // err
            return;

        } else {
            error_shown = 0;
        }

        fch_t* pDrive = (fch_t*) &handle[ch][drive_number]; // get base

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

        FileIO_FCh_WriteStat(ch, DRV00_STAT_REQ_ACK); // ack

        if (size > 0x2000) {
            DEBUG(1, "Drv00:warning large size request:%04X", size);
        }

#if DRV00_DEBUG_STATS
        HARDWARE_TICK stats_begin_seek = Timer_Get(0);
#endif

        if (FF_Seek(pDrive->fSource, addr, FF_SEEK_SET)) {
            WARNING("Drv00:Seek error");
            FileIO_FCh_WriteStat(ch, DRV00_STAT_TRANS_ACK_SEEK_ERR);
            return;
        }

#if DRV00_DEBUG_STATS
        stats_seek_ticks += (Timer_Get(0) - stats_begin_seek);
        stats_bytes_processed += size;
#endif

        while (size) {
            cur_size = size;

            if (cur_size > DRV00_BUF_SIZE) {
                cur_size = DRV00_BUF_SIZE;
            }

            if (DRV00_DEBUG) {
                DEBUG(1, "Drv00:Process Ch%d Drive:%02X Addr:%08X Size:%04X", ch, drive_number, addr, size);
            }

            // align to 512 byte boundaries if possible
            uint32_t addr_offset = addr & 0x1FF;

            if (addr_offset != 0) {
                if (DRV00_DEBUG) {
                    DEBUG(1, "Drv00:non-aligned:%08X", addr);
                }

                addr_offset = 0x200 - addr_offset;

                if (cur_size > addr_offset) {
                    cur_size = addr_offset;

                    if (DRV00_DEBUG) {
                        DEBUG(1, "Drv00:new size:%04X", cur_size);
                    }
                }
            }

            const uint8_t burstable = (dir == 0) /* read */ & ((addr & 0x1FF) == 0) /* sector aligned */ && (cur_size >= DRV00_BLK_SIZE) /* at least one block*/;

            if (dir) { // write
                // request should not be asserted if data is not ready
                // write will fail if read only
                if (pDrive->status & FILEIO_STAT_READONLY_OR_PROTECTED) {
                    WARNING("Drv00:W Read only disk!");
                    FileIO_FCh_WriteStat(ch, DRV00_STAT_TRANS_ACK_TRUNC_ERR); // truncated
                    return;
                }

                SPI_EnableFileIO();
                rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_FIFO_R));
                SPI_ReadBufferSingle(fbuf, cur_size);
                SPI_DisableFileIO();
                /*DumpBuffer(FDD_fBuf,cur_size);*/

                act_size = FF_Write(pDrive->fSource, cur_size, 1, fbuf);

                if (act_size != cur_size) {
                    WARNING("Drv00:!! Write Fail!!");
                    FileIO_FCh_WriteStat(ch, DRV00_STAT_TRANS_ACK_TRUNC_ERR); // truncated
                    return;
                }

            } else if (burstable) {
                uint32_t block_count = size / DRV00_BLK_SIZE;
                cur_size = block_count * DRV00_BLK_SIZE;

                // on entry assumes file is in correct position
                // no flow control check, FPGA must be able to sink entire transfer.
                SPI_EnableFileIO();
                rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_FIFO_W));
                SPI_EnableDirect();
                SPI_DisableFileIO();

#if DRV00_DEBUG_STATS
                HARDWARE_TICK stats_begin_read = Timer_Get(0);
#endif
                act_size = FF_ReadDirect(pDrive->fSource, cur_size, 1);
#if DRV00_DEBUG_STATS
                stats_read_ticks += (Timer_Get(0) - stats_begin_read);
#endif
                SPI_DisableDirect();

                if (DRV00_DEBUG) {
                    DEBUG(1, "Drv00:bytes direct-read:%04X", act_size);
                }

                if (act_size != cur_size) {
                    FileIO_FCh_WriteStat(ch, DRV00_STAT_TRANS_ACK_TRUNC_ERR); // truncated
                    return;
                }

            } else {
                // enough faffing, do the read
#if DRV00_DEBUG_STATS
                HARDWARE_TICK stats_begin_read = Timer_Get(0);
#endif
                act_size = FF_Read(pDrive->fSource, cur_size, 1, fbuf);
#if DRV00_DEBUG_STATS
                stats_read_ticks += (Timer_Get(0) - stats_begin_read);
#endif

                if (DRV00_DEBUG) {
                    DEBUG(1, "Drv00:bytes read:%04X", act_size);
                }

                /*DumpBuffer(FDD_fBuf,cur_size);*/

#if DRV00_DEBUG_STATS
                HARDWARE_TICK stats_begin_send = Timer_Get(0);
#endif
                SPI_EnableFileIO();
                rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_FIFO_W));
                SPI_WriteBufferSingle(fbuf, act_size);
                SPI_DisableFileIO();
#if DRV00_DEBUG_STATS
                stats_send_ticks += (Timer_Get(0) - stats_begin_send);
#endif

                if (act_size != cur_size) {
                    FileIO_FCh_WriteStat(ch, DRV00_STAT_TRANS_ACK_TRUNC_ERR); // truncated
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
                        FileIO_FCh_WriteStat(ch, DRV00_STAT_TRANS_ACK_ABORT_ERR); // err
                        return;
                    }

                } else {
                    if (FileIO_FCh_WaitStat(ch, FILEIO_REQ_OK_FM_ARM, FILEIO_REQ_OK_FM_ARM)) {
                        FileIO_FCh_WriteStat(ch, DRV00_STAT_TRANS_ACK_ABORT_ERR); // err
                        return;
                    }
                }
            } // end of wait

        }  // end of transfer loop

        // signal transfer done
        FileIO_FCh_WriteStat(ch, DRV00_STAT_TRANS_ACK_OK); // ok
        // any more to do?
        status = FileIO_FCh_GetStat(ch);
        goes --;
    } while ((status & FILEIO_REQ_ACT) && goes);

    // stats
#if DRV00_DEBUG_STATS

    if (!dir) {
        uint32_t total_ticks = (Timer_Get(0) - stats_begin);
        uint8_t reqs_processed = FILEIO_PROCESS_LIMIT - goes;
        DEBUG(0, "Drv00: requests processed: %d", reqs_processed);
        DEBUG(0, "Drv00: bytes processed: %d", stats_bytes_processed);
        DEBUG(0, "Drv00: time spent: %d ms", Timer_Convert(total_ticks));
        DEBUG(0, "Drv00: time spent in seek: %d ms", Timer_Convert(stats_seek_ticks));
        DEBUG(0, "Drv00: time spent in read: %d ms", Timer_Convert(stats_read_ticks));
        DEBUG(0, "Drv00: time spent in send: %d ms", Timer_Convert(stats_send_ticks));
        DEBUG(0, "Drv00: average speed: %d bytes / ms", stats_bytes_processed / Timer_Convert(total_ticks));
    }

#endif

}

uint8_t FileIO_Drv00_InsertInit(uint8_t ch, uint8_t drive_number, fch_t* pDrive, char* ext)
{
    DEBUG(1, "Drv00:InsertInit");

    pDrive->pDesc = calloc(1, sizeof(drv00_desc_t)); // 0 everything

    if (pDrive->pDesc == NULL) {
        WARNING("Drv00:Failed to allocate memory.");
        return (1);
    }

    drv00_desc_t* pDesc = pDrive->pDesc;

    pDesc->file_size =  FF_Size(pDrive->fSource); //->Filesize;

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

    DEBUG(1, "Drv00:Size   : %lu (%lu kB)", pDesc->file_size, pDesc->file_size);

    return (0);
}

