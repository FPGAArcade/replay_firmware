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
#include "hardware/timer.h"
#include "messaging.h"
#include "hardblocks.h"
#include "card.h"
#include <stddef.h>

const uint8_t DRV08_DEBUG = 0;
/*#define DRV08_PARAM_DEBUG 1;*/

#define DRV08_BUF_SIZE 512 // THIS must be = or > than BLK SIZE
#define DRV08_BLK_SIZE 512
#define DRV08_MAX_READ_BURST 7 // number of sectors FIFO can hold from AF de-asserted (could have <1 sector in FIFO still)
/*#define DRV08_MAX_READ_BURST 1 // number of sectors FIFO can hold from AF de-asserted (could have <1 sector in FIFO still)*/

// status bits
#define DRV08_STATUS_END 0x80
#define DRV08_STATUS_IRQ 0x10
#define DRV08_STATUS_RDY 0x08
#define DRV08_STATUS_REQ 0x04
#define DRV08_STATUS_ERR 0x01

// error bits
#define DRV08_ERROR_BBK  0x80 // bad block is detected
#define DRV08_ERROR_UNC  0x40 // uncorrectable error
#define DRV08_ERROR_IDNF 0x10 // requested sector ID is in error/not found
#define DRV08_ERROR_ABRT 0x04 // Command has been aborted due to status (not ready / fault / invalid command)
#define DRV08_ERROR_AMNF 0x01 // General error

// commands
#define DRV08_CMD_RECALIBRATE                  0x10  //1X
#define DRV08_CMD_READ_SECTORS                 0x20
#define DRV08_CMD_WRITE_SECTORS                0x30
#define DRV08_CMD_READ_VERIFY                  0x40
#define DRV08_CMD_EXECUTE_DEVICE_DIAGNOSTIC    0x90
#define DRV08_CMD_INITIALIZE_DEVICE_PARAMETERS 0x91
#define DRV08_CMD_READ_MULTIPLE                0xC4
#define DRV08_CMD_WRITE_MULTIPLE               0xC5
#define DRV08_CMD_SET_MULTIPLE_MODE            0xC6
#define DRV08_CMD_IDENTIFY_DEVICE              0xEC


typedef enum {
    XXX, // unsupported
    HDF,
    HDF_NAKED,
    MMC
} drv08_format_t;

typedef struct {
    uint8_t b[DRV08_BUF_SIZE];
} drv08_block_t;

typedef struct {
    union {
        struct {
            union {
                tRigidDiskBlock     rdsk;
                uint8_t             block0[DRV08_BUF_SIZE];
            };
            union {
                tPartitionBlock     part;
                uint8_t             block1[DRV08_BUF_SIZE];
            };
            union {
                tFileSysHeaderBlock fshd;
                uint8_t             block2[DRV08_BUF_SIZE];
            };
        };
        drv08_block_t               blocks[3];
    };
} __attribute__ ((packed)) drv08_rdb_t;

typedef struct {
    drv08_format_t format;

    uint32_t file_size;
    uint16_t cylinders;
    uint16_t heads;
    uint16_t sectors;
    uint16_t sectors_per_block;
    uint32_t index[1024];
    uint32_t index_size;

    drv08_rdb_t hdf_rdb;
    uint32_t hdf_dostype;
    uint32_t lba_offset;
} drv08_desc_t;

void Drv08_SwapBytes(uint8_t* ptr, uint32_t len)
{
    uint8_t x;
    len >>= 1;

    while (len--) {
        x = ptr[0];
        ptr[0] = ptr[1];
        ptr[1] = x;
        ptr += 2;
    }
}

uint32_t Drv08_chs2lba(drv08_desc_t* pDesc, uint16_t cylinder, uint8_t head, uint16_t sector)
{
    return (cylinder * pDesc->heads + head) * pDesc->sectors + sector - 1;
}

void Drv08_IdentifyDevice(fch_t* pDrive, drv08_desc_t* pDesc, uint16_t* pBuffer) // buffer needs to be >=512 bytes
{
    // builds Identify Device struct

    char* p;
    uint32_t total_sectors = pDesc->cylinders * pDesc->heads * pDesc->sectors;

    memset(pBuffer, 0, 512);

    pBuffer[0] = 1 << 6; // hard disk
    pBuffer[1] = pDesc->cylinders; // cyl count
    pBuffer[3] = pDesc->heads;     // head count
    pBuffer[6] = pDesc->sectors;   // sectors per track
    memcpy((char*)&pBuffer[10], "FPGA ARCADE ATA DRV ", 20); // serial number
    memcpy((char*)&pBuffer[23], ".100    ", 8); // firmware version
    //
    p = (char*)&pBuffer[27];
    memcpy(p, "Replay                                  ", 40); // model name - byte swapped
    p += 8;
    strncpy(p, (char*)pDrive->name, 16);     // file name as part of model name - byte swapped
    Drv08_SwapBytes((uint8_t*)&pBuffer[27], 40);
    //
    pBuffer[47] = 0x8010; //maximum sectors per block in Read/Write Multiple command (16 in this case) 8K bytes (4K * 16)
    pBuffer[53] = 1;
    pBuffer[54] = pDesc->cylinders;
    pBuffer[55] = pDesc->heads;
    pBuffer[56] = pDesc->sectors;
    pBuffer[57] = (uint16_t) total_sectors;
    pBuffer[58] = (uint16_t)(total_sectors >> 16);
    pBuffer[60] = (uint16_t) total_sectors;
    pBuffer[61] = (uint16_t)(total_sectors >> 16);
}

/*--$DA2000 Data               < tf0*/
/*--$DA2004 Error / Feature    < tf1*/
/*--$DA2008 SectorCount        < tf2*/
/*--$DA200C SectorNumber       < tf3*/
/*--$DA2010 CylinderLow        < tf4*/
/*--$DA2014 CylinderHigh       < tf5*/
/*--$DA2018 Device/Head        < tf6*/
/*--$DA201C Status / Command   < tf7*/
static inline void Drv08_WriteTaskFile(uint8_t ch, uint8_t error, uint8_t sector_count, uint8_t sector_number, uint8_t cylinder_low, uint8_t cylinder_high, uint8_t drive_head)
{
    SPI_EnableFileIO();
    rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_CMD_W)); // write task file registers command
    rSPI(0x00);
    rSPI(error); // error
    rSPI(sector_count); // sector count
    rSPI(sector_number); //sector number
    rSPI(cylinder_low); // cylinder low
    rSPI(cylinder_high); // cylinder high
    rSPI(drive_head); // drive/head
    SPI_DisableFileIO();
}


FF_ERROR Drv08_HardFileSeek(fch_t* pDrive, drv08_desc_t* pDesc, uint32_t lba)
{
    if (pDesc->format == MMC) {
        return FF_ERR_NONE;
    }

    HARDWARE_TICK time = Timer_Get(0);

    FF_IOMAN*  pIoman;// = pDrive->fSource->pIoman;
    (void)pIoman;

    uint32_t lba_byte = lba << 9;

    if (pDesc->format == HDF_NAKED) {
        if (DRV08_DEBUG) {
            DEBUG(1, "Drv08:Seek to LBA %d (naked) -> %d (real) (offset = %d)", lba, lba - pDesc->lba_offset, pDesc->lba_offset);
        }

        if (lba < pDesc->lba_offset) {
            DEBUG(1, "Drv08:Seek to LBA %d < %d offset => failed", lba, pDesc->lba_offset);
            return (FF_ERR_IOMAN_OUT_OF_BOUNDS_READ | FF_SEEK);
        }

        lba -= pDesc->lba_offset;
        lba_byte = lba << 9;
    }

#if 0
    pIoman = pDrive->fSource->pIoman;

    // first check if we are moving to the same cluster
    FF_T_UINT32 nNewCluster = FF_getClusterChainNumber(pIoman, lba_byte, 1);

    if ((nNewCluster < pDrive->fSource->CurrentCluster) || (nNewCluster > (pDrive->fSource->CurrentCluster + 1)) ) {
        // reposition using table
        uint16_t idx = lba >> (pDesc->index_size - 9); // 9 as lba is in 512 byte sectors
        uint32_t pos = lba_byte & (-1 << pDesc->index_size);

        nNewCluster = FF_getClusterChainNumber(pIoman, pos, 1);

        Assert(idx < 1024);

        // The current cluster index is not yet known;
        //  *) find the first valid cluster,
        //  *) step through all indices up to the current one,
        //  *) and fill out the index cluster table
        if (pDesc->index[idx] == 0xffffffff) {
            // find the first and the last indices
            uint16_t start = idx;

            // step backwards until we find a valid cluster
            for (; start ; --start)
                if (pDesc->index[start] != 0xffffffff) {
                    break;
                }

            uint64_t step = 1 << pDesc->index_size;
            uint64_t filepos = start * step;

            for (uint16_t i = start; i <= idx; ++i, filepos += step) {
                if (pDesc->index[i] != 0xffffffff) {
                    continue;
                }

                FF_Seek(pDrive->fSource, filepos, FF_SEEK_SET);
                //DEBUG(1,"index %08x %08x %08x @ %08x", (int)filepos,  pDrive->fSource->AddrCurrentCluster, pDrive->fSource->CurrentCluster, (int)i);
                Assert(i < 1024);
                pDesc->index[i] = pDrive->fSource->AddrCurrentCluster;
            }
        }

        uint32_t index_cluster = pDesc->index[idx];
        Assert(index_cluster != 0xffffffff);

        pDrive->fSource->FilePointer        = pos;
        pDrive->fSource->CurrentCluster     = nNewCluster;
        pDrive->fSource->AddrCurrentCluster = index_cluster;

        //DEBUG(1,"seek JUMP lba*512 %08X, pos %08x, idx %d, newcluster %08X index_cluster %08X", lba_byte, pos, idx, nNewCluster, index_cluster);
    }

#endif

    FF_ERROR err = FF_Seek(pDrive->fSource, lba_byte, FF_SEEK_SET);

    time = Timer_Get(0) - time;

    if (Timer_Convert(time) > 100) {
        DEBUG(1, "Long seek time %lu ms.", Timer_Convert(time));
    }

    return err;
}

void Drv08_BufferSend(uint8_t ch, fch_t* pDrive, uint8_t* pBuffer)
{
    /*DumpBuffer(pBuffer, DRV08_BLK_SIZE);*/
    SPI_EnableFileIO();
    rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_FIFO_W));
    SPI_WriteBufferSingle(pBuffer, DRV08_BLK_SIZE);
    SPI_DisableFileIO();
}

void Drv08_FileReadSend(uint8_t ch, fch_t* pDrive, uint8_t* pBuffer)
{

    uint32_t bytes_r = FF_Read(pDrive->fSource, DRV08_BLK_SIZE, 1, pBuffer);

    /*DumpBuffer(pBuffer,DRV08_BLK_SIZE);*/

    // add error handling
    if (bytes_r != DRV08_BLK_SIZE) {
        DEBUG(1, "Drv08:!! Read Fail!!");
    }

    SPI_EnableFileIO();
    rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_FIFO_W));
    SPI_WriteBufferSingle(pBuffer, DRV08_BLK_SIZE);
    SPI_DisableFileIO();
}

void Drv08_FileReadSendDirect(uint8_t ch, fch_t* pDrive, uint8_t sector_count)
{
    // on entry assumes file is in correct position
    // no flow control check, FPGA must be able to sink entire transfer.
    SPI_EnableFileIO();
    rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_FIFO_W));
    SPI_EnableDirect();
    SPI_DisableFileIO();

    uint32_t bytes_r = FF_ReadDirect(pDrive->fSource, DRV08_BLK_SIZE, sector_count);

    SPI_DisableDirect();

    // add error handling
    if (bytes_r != (DRV08_BLK_SIZE * sector_count)) {
        DEBUG(1, "Drv08:!! Direct Read Fail!!");
    }

}

void Drv08_FileWrite(uint8_t ch, fch_t* pDrive, uint8_t* pBuffer)
{
    // fix error handling
    uint32_t bytes_w = FF_Write(pDrive->fSource, DRV08_BLK_SIZE, 1, pBuffer);

    if (bytes_w != DRV08_BLK_SIZE) {
        DEBUG(1, "Drv08:!! Write Fail!!");
    }
}

extern FF_IOMAN* pIoman;        // fixme!

void Drv08_CardReadSend(uint8_t ch, uint32_t lba, uint32_t numblocks, uint8_t* pBuffer)
{
    FF_FlushCache(pIoman);

    while (numblocks--) {

        FF_ERROR err = Card_ReadM(pBuffer, lba++, 1, NULL);

        /*DumpBuffer(pBuffer, DRV08_BLK_SIZE);*/

        // add error handling
        if (err != FF_ERR_NONE) {
            DEBUG(1, "Drv08:!! CardRead Fail!!");
        }

        SPI_EnableFileIO();
        rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_FIFO_W));
        SPI_WriteBufferSingle(pBuffer, DRV08_BLK_SIZE);
        SPI_DisableFileIO();

    }
}

void Drv08_CardReadSendDirect(uint8_t ch, uint32_t lba, uint32_t numblocks, uint8_t* pBuffer)
{
    SPI_EnableFileIO();
    rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_FIFO_W));
    SPI_EnableDirect();
    SPI_DisableFileIO();

    FF_FlushCache(pIoman);
    FF_ERROR err = Card_ReadM(NULL, lba, numblocks, NULL);

    SPI_DisableDirect();

    if (err != FF_ERR_NONE) {
        DEBUG(1, "Drv08:!! CardRead Fail!!");
    }
}

void Drv08_CardWrite(uint8_t ch, uint32_t lba, uint8_t* pBuffer)
{
    FF_ERROR err = Card_WriteM(pBuffer, lba, 1, NULL);
    FF_FlushCache(pIoman);

    if (err != FF_ERR_NONE) {
        DEBUG(1, "Drv08:!! CardWrite Fail!!");
    }
}

static inline void Drv08_GetParams(uint8_t tfr[8], drv08_desc_t* pDesc,  // inputs
                                   uint16_t* sector, uint16_t* cylinder, uint8_t* head, uint16_t* sector_count, uint32_t* lba, uint8_t* lba_mode)
{
    // given the register file, extract address and sector count.
    // then convert this into an LBA.

    *sector_count = tfr[2];
    *sector       = tfr[3]; // note, 8>16 bit
    *cylinder     = tfr[4] | tfr[5] << 8;
    *head         = tfr[6] & 0x0F;
    *lba_mode     = (tfr[6] & 0x40);

    if (*sector_count == 0) {
        *sector_count = 0x100;
    }

    if (*lba_mode) {
        *lba = (*head) << 24 | (*cylinder) << 8 | (*sector);

    } else {
        *lba = ( ((*cylinder) * pDesc->heads + (*head)) * pDesc->sectors) + (*sector) - 1;
    }

    if (DRV08_DEBUG) {
#if 1 //def DRV08_PARAM_DEBUG
        DEBUG(1, "sector   : %d", *sector);
        DEBUG(1, "cylinder : %d", *cylinder);
        DEBUG(1, "head     : %d", *head);
        DEBUG(1, "lba      : %ld", *lba);
        DEBUG(1, "count    : %d", *sector_count);
#endif
    }
}

static inline void Drv08_UpdateParams(uint8_t ch, uint8_t tfr[8], uint16_t sector, uint16_t cylinder, uint8_t head, uint32_t lba, uint8_t lba_mode)
{
    // note, we can only update the taskfile when BUSY
    // all params inputs
    uint8_t drive = tfr[6] & 0xF0;

    SPI_EnableFileIO();
    rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_CMD_W | 0x03)); // write task file registers command

    if (lba_mode) {
        rSPI((uint8_t) (lba      ));
        rSPI((uint8_t) (lba >>  8));
        rSPI((uint8_t) (lba >> 16));
        rSPI((uint8_t) (drive | ( (lba >> 24) & 0x0F)));

    } else {
        rSPI((uint8_t)  sector);
        rSPI((uint8_t)  cylinder);
        rSPI((uint8_t) (cylinder >> 8));
        rSPI((uint8_t) (drive | (head & 0x0F)));
    }

    SPI_DisableFileIO();
}

static inline void Drv08_IncParams(drv08_desc_t* pDesc,  // inputs
                                   uint16_t* sector, uint16_t* cylinder, uint8_t* head, uint32_t* lba)
{
    // increment address.
    // At command completion, the Command Block Registers contain the cylinder, head and sector number of the last sector read
    if (*sector == pDesc->sectors) {
        *sector = 1;
        (*head)++;

        if (*head == pDesc->heads) {
            *head = 0;
            (*cylinder)++;
        }

    } else {
        (*sector)++;
    }

    // and the LBA
    (*lba)++;
}
//
//
//
void Drv08_ATA_Handle(uint8_t ch, fch_t handle[2][FCH_MAX_NUM])
{

    // file buffer
    // to do, check how expensive this allocation is every time.
    // static buffer?

    uint16_t  fbuf16[DRV08_BUF_SIZE / 2]; // to get 16 bit alignment for id.
    uint8_t*  fbuf = (uint8_t*) fbuf16; // reuse the buffer

    uint8_t  tfr[8];
    uint16_t i;
    uint8_t  unit         = 0;
    uint16_t sector_count = 0;
    uint16_t block_count  = 0;

    uint16_t sector   = 0;
    uint16_t cylinder = 0;
    uint8_t  head     = 0;
    uint32_t lba      = 0;
    uint8_t  lba_mode = 0;
    uint8_t  first    = 1;

    // read task file
    SPI_EnableFileIO();
    rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_CMD_R | 0x0));
    rSPI(0x00); // dummy

    for (i = 0; i < 8; i++) {
        tfr[i] = rSPI(0);
    }

    SPI_DisableFileIO();

    unit = tfr[6] & 0x10 ? 1 : 0; // master/slave selection
    // 0 = master, 1 = slave

    //
    if (DRV08_DEBUG)
        DEBUG(1, "Drv08:CMD %02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X u:%d",
              tfr[0], tfr[1], tfr[2], tfr[3], tfr[4], tfr[5], tfr[6], tfr[7], unit);


    fch_t* pDrive = &handle[ch][unit]; // get base
    drv08_desc_t* pDesc = pDrive->pDesc;

    if (!FileIO_FCh_GetInserted(ch, unit)) {
        DEBUG(1, "Drv08:Process Ch:%d Unit:%d not mounted", ch, unit);
        Drv08_WriteTaskFile (ch, DRV08_ERROR_ABRT, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
        FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ | DRV08_STATUS_ERR);
        return;
    }

    //
    if        ((tfr[7] & 0xF0) == DRV08_CMD_RECALIBRATE) { // Recalibrate 0x10-0x1F (class 3 command: no data)
        //
        if (DRV08_DEBUG) {
            DEBUG(1, "Drv08:Recalibrate");
        }

        //
        Drv08_WriteTaskFile (ch, 0, 0, 1, 0, 0, tfr[6] & 0xF0);
        FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ);
        //

    } else if (tfr[7] == DRV08_CMD_EXECUTE_DEVICE_DIAGNOSTIC) {
        //
        if (DRV08_DEBUG) {
            DEBUG(1, "Drv08:Execute Device Diagnostic");
        }

        //
        Drv08_WriteTaskFile (ch, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00);
        FileIO_FCh_WriteStat(ch, DRV08_STATUS_END);
        //

    } else if (tfr[7] == DRV08_CMD_IDENTIFY_DEVICE) {
        //
        if (DRV08_DEBUG) {
            DEBUG(1, "Drv08:Identify Device");
        }

        //
        Drv08_IdentifyDevice(pDrive, pDesc, fbuf16);
        Drv08_WriteTaskFile (ch, 0, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);

        FileIO_FCh_WriteStat(ch, DRV08_STATUS_RDY); // pio in (class 1) command type
        SPI_EnableFileIO();
        rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_FIFO_W)); // write data command*/

        for (i = 0; i < 256; i++) {
            rSPI((uint8_t) fbuf16[i]);
            rSPI((uint8_t)(fbuf16[i] >> 8));
        }

        SPI_DisableFileIO();
        FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ);
        //

    } else if (tfr[7] == DRV08_CMD_INITIALIZE_DEVICE_PARAMETERS) {
        //
        if (DRV08_DEBUG) {
            DEBUG(1, "Drv08:Initialize Device Parameters");
        }

        //
        Drv08_WriteTaskFile (ch, 0, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
        FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ);
        //

    } else if (tfr[7] == DRV08_CMD_SET_MULTIPLE_MODE) {
        //
        if (DRV08_DEBUG) {
            DEBUG(1, "Drv08:Set Multiple Mode");
        }

        //
        pDesc->sectors_per_block = tfr[2];
        FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ);
        //

    } else if (tfr[7] == DRV08_CMD_READ_SECTORS) {
        //
        if (DRV08_DEBUG) {
            DEBUG(1, "Drv08:Read Sectors");
        }

        //
        Drv08_GetParams(tfr, pDesc, &sector, &cylinder, &head, &sector_count, &lba, &lba_mode);

        uint32_t lba_naked = lba < pDesc->lba_offset ? pDesc->lba_offset : lba;

        if (Drv08_HardFileSeek(pDrive, pDesc, lba_naked) != FF_ERR_NONE) {
            WARNING("Drv08:Read from invalid LBA");
            Drv08_WriteTaskFile (ch, DRV08_ERROR_ABRT, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
            FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ | DRV08_STATUS_ERR);
            return;
        }

        while (sector_count) {
            FileIO_FCh_WriteStat(ch, DRV08_STATUS_RDY); // pio in (class 1) command type
            FileIO_FCh_WaitStat (ch, FILEIO_REQ_OK_FM_ARM, FILEIO_REQ_OK_FM_ARM); // wait for nearly empty buffer
            FileIO_FCh_WriteStat(ch, DRV08_STATUS_IRQ);

            if (!first) {
                Drv08_IncParams(pDesc, &sector, &cylinder, &head, &lba);
                Drv08_UpdateParams(ch, tfr, sector, cylinder,  head, lba, lba_mode);
            }

            first = 0;

            if (pDesc->format == MMC) {
                Drv08_CardReadSendDirect(ch, lba, 1, fbuf);

            } else if (lba < pDesc->lba_offset) {
                Drv08_BufferSend(ch, pDrive, pDesc->hdf_rdb.blocks[lba % 3].b);

            } else {
                /*Drv08_FileReadSend(ch, pDrive, fbuf);*/
                Drv08_FileReadSendDirect(ch, pDrive, 1); // read and send block
            }


            sector_count--; // decrease sector count
        }

        //

    } else if (tfr[7] == DRV08_CMD_READ_MULTIPLE) { // Read Multiple Sectors (multiple sector transfer per IRQ)
        //
        if (DRV08_DEBUG) {
            DEBUG(1, "Drv08:Read Sectors Multiple");
        }

        //
        if (pDesc->sectors_per_block == 0) {
            WARNING("Drv08:Set Multiple not done");
            Drv08_WriteTaskFile (ch, DRV08_ERROR_ABRT, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
            FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ | DRV08_STATUS_ERR);
            return;
        }

        FileIO_FCh_WriteStat(ch, DRV08_STATUS_RDY); // pio in (class 1) command type

        Drv08_GetParams(tfr, pDesc, &sector, &cylinder, &head, &sector_count, &lba, &lba_mode);

        uint32_t lba_naked = lba < pDesc->lba_offset ? pDesc->lba_offset : lba;

        if (Drv08_HardFileSeek(pDrive, pDesc, lba_naked) != FF_ERR_NONE) {
            WARNING("Drv08:Read Multiple bad LBA");
            Drv08_WriteTaskFile (ch, DRV08_ERROR_ABRT, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
            FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ | DRV08_STATUS_ERR);
            return;
        }

        while (sector_count) {
            block_count = sector_count;

            if (block_count > pDesc->sectors_per_block) {
                block_count = pDesc->sectors_per_block;
            }

            FileIO_FCh_WriteStat(ch, DRV08_STATUS_IRQ);

            // calc final sector number in block
            i = block_count;

            while (i--) {
                if (!first) {
                    Drv08_IncParams(pDesc, &sector, &cylinder, &head, &lba);
                }

                first = 0;
            }

            // update, this should be done at the end really, but we need to make sure the transfer has not completed
            Drv08_UpdateParams(ch, tfr, sector, cylinder,  head, lba, lba_mode);

            // do transfer
            while (block_count) {
                i = block_count;

                if (i > DRV08_MAX_READ_BURST) {
                    i = DRV08_MAX_READ_BURST;
                }

                FileIO_FCh_WaitStat (ch, FILEIO_REQ_OK_FM_ARM, FILEIO_REQ_OK_FM_ARM);

                if (pDesc->format == MMC) {
                    Drv08_CardReadSendDirect(ch, lba_naked, i, fbuf);
                    lba_naked += i;

                } else if (lba < pDesc->lba_offset) {
                    Drv08_BufferSend(ch, pDrive, pDesc->hdf_rdb.blocks[lba % 3].b);

                } else {
                    Drv08_FileReadSendDirect(ch, pDrive, i); // read and send block(s)
                }

                sector_count -= i;
                block_count -= i;
            } // end block

        }  // end sector

        //

    } else if (tfr[7] == DRV08_CMD_READ_VERIFY) { // Read verify
        //
        if (DRV08_DEBUG) {
            DEBUG(1, "Drv08:Read Verify");
        }

        //
        Drv08_WriteTaskFile (ch, 0, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);

        FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ); // IRQ or not?

    } else if (tfr[7] == DRV08_CMD_WRITE_SECTORS) {  // write sectors
        //
        if (DRV08_DEBUG) {
            DEBUG(1, "Drv08:Write Sectors");
        }

        //
        if (pDrive->status & FILEIO_STAT_READONLY_OR_PROTECTED) {
            WARNING("Drv08:W Read only disk!");
            Drv08_WriteTaskFile (ch, DRV08_ERROR_ABRT, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
            FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ | DRV08_STATUS_ERR);
            return;
        }

        //
        FileIO_FCh_WriteStat(ch, DRV08_STATUS_REQ); // pio out (class 2) command type
        Drv08_GetParams(tfr, pDesc, &sector, &cylinder, &head, &sector_count, &lba, &lba_mode);

        uint32_t lba_naked = lba < pDesc->lba_offset ? pDesc->lba_offset : lba;

        if (Drv08_HardFileSeek(pDrive, pDesc, lba) != FF_ERR_NONE) {
            WARNING("Drv08:Write to invalid LBA");
            Drv08_WriteTaskFile (ch, DRV08_ERROR_ABRT, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
            FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ | DRV08_STATUS_ERR);
            return;
        }

        if (lba == 0 && pDesc->format == MMC) {
            WARNING("Drv08:Write to MBR disabled");
            Drv08_WriteTaskFile (ch, DRV08_ERROR_ABRT, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
            FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ | DRV08_STATUS_ERR);
            return;
        }

        while (sector_count) {
            FileIO_FCh_WaitStat (ch, FILEIO_REQ_OK_TO_ARM, FILEIO_REQ_OK_TO_ARM); // wait for data

            // fetch
            SPI_EnableFileIO();
            rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_FIFO_R));
            SPI_ReadBufferSingle(fbuf, DRV08_BLK_SIZE);
            SPI_DisableFileIO();

            // safe to do after fetch, STATUS_END clears busy
            if (!first) {
                Drv08_IncParams(pDesc, &sector, &cylinder, &head, &lba);
                Drv08_UpdateParams(ch, tfr, sector, cylinder,  head, lba, lba_mode);
            }

            first = 0;

            sector_count--; // decrease sector count

            // release core
            if (sector_count) {
                FileIO_FCh_WriteStat(ch, DRV08_STATUS_IRQ);

            } else {
                FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ);    // last one
            }

            // optimal to put this after the status update, but then we cannot indicate write failure
            // write to file
            if (pDesc->format == MMC) {
                Drv08_CardWrite(ch, lba_naked++, fbuf);

            } else {
                Drv08_FileWrite(ch, pDrive, fbuf);
            }
        }

    } else if (tfr[7] == DRV08_CMD_WRITE_MULTIPLE) { // write sectors
        //
        if (DRV08_DEBUG) {
            DEBUG(1, "Drv08:Write Sectors Multiple");
        }

        //
        if (pDrive->status & FILEIO_STAT_READONLY_OR_PROTECTED) {
            WARNING("Drv08:W Read only disk!");
            Drv08_WriteTaskFile (ch, DRV08_ERROR_ABRT, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
            FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ | DRV08_STATUS_ERR);
            return;
        }

        //
        if (pDesc->sectors_per_block == 0) {
            WARNING("Drv08:Set Multiple not done");
            Drv08_WriteTaskFile (ch, DRV08_ERROR_ABRT, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
            FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ | DRV08_STATUS_ERR);
            return;
        }

        FileIO_FCh_WriteStat(ch, DRV08_STATUS_REQ); // pio out (class 2) command type

        Drv08_GetParams(tfr, pDesc, &sector, &cylinder, &head, &sector_count, &lba, &lba_mode);

        uint32_t lba_naked = lba < pDesc->lba_offset ? pDesc->lba_offset : lba;

        if (Drv08_HardFileSeek(pDrive, pDesc, lba) != FF_ERR_NONE) {
            WARNING("Drv08:Write Multiple bad LBA");
            Drv08_WriteTaskFile (ch, DRV08_ERROR_ABRT, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
            FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ | DRV08_STATUS_ERR);
            return;
        }

        if (lba == 0 && pDesc->format == MMC) {
            WARNING("Drv08:Write to MBR disabled");
            Drv08_WriteTaskFile (ch, DRV08_ERROR_ABRT, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
            FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ | DRV08_STATUS_ERR);
            return;
        }

        while (sector_count) {
            block_count = sector_count;

            if (block_count > pDesc->sectors_per_block) {
                block_count = pDesc->sectors_per_block;
            }

            while (block_count) {

                FileIO_FCh_WaitStat (ch, FILEIO_REQ_OK_TO_ARM, FILEIO_REQ_OK_TO_ARM); // wait for data

                // fetch
                SPI_EnableFileIO();
                rSPI(FCH_CMD(ch, FILEIO_FCH_CMD_FIFO_R));
                SPI_ReadBufferSingle(fbuf, DRV08_BLK_SIZE);
                SPI_DisableFileIO();

                // write to file
                if (pDesc->format == MMC) {
                    Drv08_CardWrite(ch, lba_naked++, fbuf);

                } else {
                    Drv08_FileWrite(ch, pDrive, fbuf);
                }

                if (!first) {
                    Drv08_IncParams(pDesc, &sector, &cylinder, &head, &lba);
                }

                first = 0;

                block_count--;
                sector_count--; // decrease sector count
            }

            Drv08_UpdateParams(ch, tfr, sector, cylinder,  head, lba, lba_mode);

            // release core
            if (sector_count) {
                FileIO_FCh_WriteStat(ch, DRV08_STATUS_IRQ);

            } else {
                FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ);    // last one
            }

        } // end sector

        //

    } else {
        //
        WARNING("Drv08:Unknown ATA command:");
        WARNING("%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X",
                tfr[0], tfr[1], tfr[2], tfr[3], tfr[4], tfr[5], tfr[6], tfr[7]);
        Drv08_WriteTaskFile(ch, DRV08_ERROR_ABRT, tfr[2], tfr[3], tfr[4], tfr[5], tfr[6]);
        FileIO_FCh_WriteStat(ch, DRV08_STATUS_END | DRV08_STATUS_IRQ | DRV08_STATUS_ERR);
    }
}

// insert functions
uint8_t Drv08_GetHardfileType(fch_t* pDrive, drv08_desc_t* pDesc)
{
    uint8_t fbuf[DRV08_BLK_SIZE];
    uint32_t i = 0;

    FF_Seek(pDrive->fSource, 0, FF_SEEK_SET);

    for (i = 0; i < 16; ++i) { // check first 16 blocks
        FF_Read(pDrive->fSource, DRV08_BLK_SIZE, 1, fbuf);

        /*DumpBuffer(fbuf, 32);*/

        if (!memcmp(fbuf, "RDSK", 4)) {
            INFO("Drv08:RDB OK");
            return (0);
        }

        if ( (!memcmp(fbuf, "DOS", 3)) || (!memcmp(fbuf, "PFS", 3)) || (!memcmp(fbuf, "SFS", 3)) ) {
            pDesc->format = HDF_NAKED;
            pDesc->hdf_dostype = (fbuf[0] << 24) | (fbuf[1] << 16) | (fbuf[2] << 8) | fbuf[3];
            WARNING("Drv08:RDB MISSING - %08x", pDesc->hdf_dostype);
            return (0);
        }
    }

    WARNING("Drv08:Unknown HDF format");
    return (1);
}

#define READ_BE_32B(x)  ((uint32_t) (( ((uint8_t*)&x)[0] << 24) | (((uint8_t*)&x)[1] << 16) | (((uint8_t*)&x)[2] << 8) | ((uint8_t*)&x)[3]))
#define WRITE_BE_32B(x,val) \
    ((uint8_t*)&x)[0] = (uint8_t) (((val) >> 24) & 0xff); \
    ((uint8_t*)&x)[1] = (uint8_t) (((val) >> 16) & 0xff); \
    ((uint8_t*)&x)[2] = (uint8_t) (((val)  >> 8) & 0xff); \
    ((uint8_t*)&x)[3] = (uint8_t)  ((val)        & 0xff);


static uint32_t CalcBEsum(void* block, uint32_t numlongs)
{
    uint32_t* p = (uint32_t*)block;
    int32_t chksum = 0;

    if (numlongs > 512 / sizeof(uint32_t)) {
        numlongs = 512 / sizeof(uint32_t);
    }

    for (int i = 0; i < numlongs; i++ ) {
        uint32_t v = READ_BE_32B(*p);
        chksum += (int32_t)v;
        p++;
    }

    return chksum;
}

static void Drv08_PrintRDB(fch_t* pDrive, drv08_desc_t* pDesc)
{
    tRigidDiskBlock rdsk;

    FF_Seek(pDrive->fSource, 0, FF_SEEK_SET);
    FF_Read(pDrive->fSource, sizeof(rdsk), 1, (uint8_t*)&rdsk);

    if (CalcBEsum(&rdsk, READ_BE_32B(rdsk.rdb_SummedLongs)) != 0) {
        DEBUG(2, ">   INVALID RDSK CHECKSUM");
        return;
    }

    rdsk.rdb_DiskVendor[sizeof(rdsk.rdb_DiskVendor) - 1] = 0;
    rdsk.rdb_DiskProduct[sizeof(rdsk.rdb_DiskProduct) - 1] = 0;
    rdsk.rdb_DiskRevision[sizeof(rdsk.rdb_DiskRevision) - 1] = 0;
    rdsk.rdb_ControllerVendor[sizeof(rdsk.rdb_ControllerVendor) - 1] = 0;
    rdsk.rdb_ControllerProduct[sizeof(rdsk.rdb_ControllerProduct) - 1] = 0;
    rdsk.rdb_ControllerRevision[sizeof(rdsk.rdb_ControllerRevision) - 1] = 0;
    rdsk.rdb_DriveInitName[sizeof(rdsk.rdb_DriveInitName) - 1] = 0;

    DEBUG(2, "rdb_ID                = 0x%08x", READ_BE_32B(rdsk.rdb_ID));
    DEBUG(2, "rdb_SummedLongs       = 0x%08x", READ_BE_32B(rdsk.rdb_SummedLongs));
    DEBUG(2, "rdb_ChkSum            = 0x%08x", READ_BE_32B(rdsk.rdb_ChkSum));
    DEBUG(2, "rdb_HostID            = 0x%08x", READ_BE_32B(rdsk.rdb_HostID));
    DEBUG(2, "rdb_BlockBytes        = 0x%08x", READ_BE_32B(rdsk.rdb_BlockBytes));
    DEBUG(2, "rdb_Flags             = 0x%08x", READ_BE_32B(rdsk.rdb_Flags));
    DEBUG(2, "rdb_BadBlockList      = 0x%08x", READ_BE_32B(rdsk.rdb_BadBlockList));
    DEBUG(2, "rdb_PartitionList     = 0x%08x", READ_BE_32B(rdsk.rdb_PartitionList));
    DEBUG(2, "rdb_FileSysHeaderList = 0x%08x", READ_BE_32B(rdsk.rdb_FileSysHeaderList));
    DEBUG(2, "rdb_DriveInit         = 0x%08x", READ_BE_32B(rdsk.rdb_DriveInit));
    DEBUG(2, "rdb_Cylinders         = 0x%08x", READ_BE_32B(rdsk.rdb_Cylinders));
    DEBUG(2, "rdb_Sectors           = 0x%08x", READ_BE_32B(rdsk.rdb_Sectors));
    DEBUG(2, "rdb_Heads             = 0x%08x", READ_BE_32B(rdsk.rdb_Heads));
    DEBUG(2, "rdb_Interleave        = 0x%08x", READ_BE_32B(rdsk.rdb_Interleave));
    DEBUG(2, "rdb_Park              = 0x%08x", READ_BE_32B(rdsk.rdb_Park));
    DEBUG(2, "rdb_WritePreComp      = 0x%08x", READ_BE_32B(rdsk.rdb_WritePreComp));
    DEBUG(2, "rdb_ReducedWrite      = 0x%08x", READ_BE_32B(rdsk.rdb_ReducedWrite));
    DEBUG(2, "rdb_StepRate          = 0x%08x", READ_BE_32B(rdsk.rdb_StepRate));
    DEBUG(2, "rdb_RDBBlocksLo       = 0x%08x", READ_BE_32B(rdsk.rdb_RDBBlocksLo));
    DEBUG(2, "rdb_RDBBlocksHi       = 0x%08x", READ_BE_32B(rdsk.rdb_RDBBlocksHi));
    DEBUG(2, "rdb_LoCylinder        = 0x%08x", READ_BE_32B(rdsk.rdb_LoCylinder));
    DEBUG(2, "rdb_HiCylinder        = 0x%08x", READ_BE_32B(rdsk.rdb_HiCylinder));
    DEBUG(2, "rdb_CylBlocks         = 0x%08x", READ_BE_32B(rdsk.rdb_CylBlocks));
    DEBUG(2, "rdb_AutoParkSeconds   = 0x%08x", READ_BE_32B(rdsk.rdb_AutoParkSeconds));
    DEBUG(2, "rdb_HighRDSKBlock     = 0x%08x", READ_BE_32B(rdsk.rdb_HighRDSKBlock));
    DEBUG(2, "rdb_Reserved4         = 0x%08x", READ_BE_32B(rdsk.rdb_Reserved4));

    DEBUG(2, "rdb_DiskVendor        = '%s'", rdsk.rdb_DiskVendor);
    DEBUG(2, "rdb_DiskProduct       = '%s'", rdsk.rdb_DiskProduct);
    DEBUG(2, "rdb_DiskRevision      = '%s'", rdsk.rdb_DiskRevision);
    DEBUG(2, "rdb_ControllerVendor  = '%s'", rdsk.rdb_ControllerVendor);
    DEBUG(2, "rdb_ControllerProduct = '%s'", rdsk.rdb_ControllerProduct);
    DEBUG(2, "rdb_ControllerRevision= '%s'", rdsk.rdb_ControllerRevision);
    DEBUG(2, "rdb_DriveInitName     = '%s'", rdsk.rdb_DriveInitName);

    if (READ_BE_32B(rdsk.rdb_ID) != IDNAME_RIGIDDISK) {
        DEBUG(2, ">   INVALID RDSK BLOCK");
        return;
    }

    uint32_t blockBytes = READ_BE_32B(rdsk.rdb_BlockBytes);

    tPartitionBlock part;

    for (uint32_t partList = READ_BE_32B(rdsk.rdb_PartitionList); partList != 0xffffffff; partList = READ_BE_32B(part.pb_Next)) {
        DEBUG(2, ">PART @ %08x", partList);

        FF_Seek(pDrive->fSource, partList * blockBytes, FF_SEEK_SET);
        FF_Read(pDrive->fSource, sizeof(part), 1, (uint8_t*)&part);

        if (CalcBEsum(&part, READ_BE_32B(part.pb_SummedLongs)) != 0) {
            DEBUG(2, ">   INVALID PART CHECKSUM");
            break;
        }

        part.pb_DriveName[part.pb_DriveName[0] + 1] = 0;

        DEBUG(2, ">   pb_ID             = 0x%08x", READ_BE_32B(part.pb_ID));
        DEBUG(2, ">   pb_SummedLongs    = 0x%08x", READ_BE_32B(part.pb_SummedLongs));
        DEBUG(2, ">   pb_ChkSum         = 0x%08x", READ_BE_32B(part.pb_ChkSum));
        DEBUG(2, ">   pb_HostID         = 0x%08x", READ_BE_32B(part.pb_HostID));
        DEBUG(2, ">   pb_Next           = 0x%08x", READ_BE_32B(part.pb_Next));
        DEBUG(2, ">   pb_Flags          = 0x%08x", READ_BE_32B(part.pb_Flags));
        DEBUG(2, ">   pb_DevFlags       = 0x%08x", READ_BE_32B(part.pb_DevFlags));
        DEBUG(2, ">   pb_DriveName      = '%s'", &part.pb_DriveName[1]);

        for (int i = 0; i < (sizeof(part.pb_Environment) / sizeof(part.pb_Environment[0])); ++i) {
            DEBUG(2, ">   pb_Environment[%2d]= 0x%08x", i, READ_BE_32B(part.pb_Environment[i]));
        }

        if (READ_BE_32B(part.pb_ID) != IDNAME_PARTITION) {
            DEBUG(2, ">   INVALID PART BLOCK");
            break;
        }
    }


    tFileSysHeaderBlock fshd;

    for (uint32_t fshdList = READ_BE_32B(rdsk.rdb_FileSysHeaderList); fshdList != 0xffffffff; fshdList = READ_BE_32B(fshd.fhb_Next)) {
        DEBUG(2, ">FSHD @ %08x", fshdList);

        FF_Seek(pDrive->fSource, fshdList * blockBytes, FF_SEEK_SET);
        FF_Read(pDrive->fSource, sizeof(fshd), 1, (uint8_t*)&fshd);

        if (CalcBEsum(&fshd, READ_BE_32B(fshd.fhb_SummedLongs)) != 0) {
            DEBUG(2, ">   INVALID FSHD CHECKSUM");
            break;
        }

        fshd.fhb_FileSysName[sizeof(fshd.fhb_FileSysName) - 1] = 0;

        DEBUG(2, ">   fhb_ID            = %08x", READ_BE_32B(fshd.fhb_ID));
        DEBUG(2, ">   fhb_SummedLongs   = %08x", READ_BE_32B(fshd.fhb_SummedLongs));
        DEBUG(2, ">   fhb_ChkSum        = %08x", READ_BE_32B(fshd.fhb_ChkSum));
        DEBUG(2, ">   fhb_HostID        = %08x", READ_BE_32B(fshd.fhb_HostID));
        DEBUG(2, ">   fhb_Next          = %08x", READ_BE_32B(fshd.fhb_Next));
        DEBUG(2, ">   fhb_Flags         = %08x", READ_BE_32B(fshd.fhb_Flags));
        DEBUG(2, ">   fhb_DosType       = %08x", READ_BE_32B(fshd.fhb_DosType));
        DEBUG(2, ">   fhb_Version       = %08x", READ_BE_32B(fshd.fhb_Version));
        DEBUG(2, ">   fhb_PatchFlags    = %08x", READ_BE_32B(fshd.fhb_PatchFlags));
        DEBUG(2, ">   fhb_Type          = %08x", READ_BE_32B(fshd.fhb_Type));
        DEBUG(2, ">   fhb_Task          = %08x", READ_BE_32B(fshd.fhb_Task));
        DEBUG(2, ">   fhb_Lock          = %08x", READ_BE_32B(fshd.fhb_Lock));
        DEBUG(2, ">   fhb_Handler       = %08x", READ_BE_32B(fshd.fhb_Handler));
        DEBUG(2, ">   fhb_StackSize     = %08x", READ_BE_32B(fshd.fhb_StackSize));
        DEBUG(2, ">   fhb_Priority      = %08x", READ_BE_32B(fshd.fhb_Priority));
        DEBUG(2, ">   fhb_Startup       = %08x", READ_BE_32B(fshd.fhb_Startup));
        DEBUG(2, ">   fhb_SegListBlocks = %08x", READ_BE_32B(fshd.fhb_SegListBlocks));
        DEBUG(2, ">   fhb_GlobalVec     = %08x", READ_BE_32B(fshd.fhb_GlobalVec));
        DEBUG(2, ">   fhb_FileSysName   = '%s'", fshd.fhb_FileSysName);

        if (READ_BE_32B(fshd.fhb_ID) != IDNAME_FILESYSHEADER) {
            DEBUG(2, ">   INVALID FSHD BLOCK");
            break;
        }
    }
}


void Drv08_GetHardfileGeometry(fch_t* pDrive, drv08_desc_t* pDesc)
{
    // this function comes from WinUAE, should return the same CHS as WinUAE
    uint32_t total;
    uint32_t i;
    uint32_t head = 0;
    uint32_t cyl  = 0;
    uint32_t spt  = 0;
    uint32_t sptt[] = { 63, 127, 255, 0 };
    uint32_t size = pDesc->file_size;

    if (size == 0) {
        return;
    }

    total = size / 512;

    for (i = 0; sptt[i] != 0; i++) {
        spt = sptt[i];

        for (head = 4; head <= 16; head++) {
            cyl = total / (head * spt);

            if (size <= 512 * 1024 * 1024) {
                if (cyl <= 1023) {
                    break;
                }

            } else {
                if (cyl < 16383) {
                    break;
                }

                if (cyl < 32767 && head >= 5) {
                    break;
                }

                if (cyl <= 65535) {
                    break;
                }
            }
        }

        if (head <= 16) {
            break;
        }
    }

    pDesc->cylinders         = (unsigned short)cyl;
    pDesc->heads             = (unsigned short)head;
    pDesc->sectors           = (unsigned short)spt;
    pDesc->sectors_per_block = 0; // catch if not set to !=0
}

void Drv08_BuildHardfileIndex(fch_t* pDrive, drv08_desc_t* pDesc)
{
#if 0
    pDesc->index_size = 16; // indexing size

    uint64_t i; // 64 as the last index (> filesize) can cause a 32 bit int to wrap
    uint64_t j;

    i = pDesc->file_size >> 10; // file size divided by 1024 (index table size)
    j = 1 << pDesc->index_size;

    while (j < i) { // find greater or equal power of two
        j <<= 1;
        pDesc->index_size++;
    }

    // index 22 for 4G file 0 - FFFFFFFF
    // 2^22 = 0040,0000 step size
    //
    DEBUG(1, "index size %08X j %08X", pDesc->index_size, j); // j step size
    //  Build table on-the-fly instead ; see Drv08_HardFileSeek()
#if 0
    uint32_t idx = 0;

    for (i = 0; i < pDesc->file_size; i += j) {
        FF_Seek(pDrive->fSource, i, FF_SEEK_SET);
        //DEBUG(1,"index %08x %08x %08x",i,  pDrive->fSource->AddrCurrentCluster, pDrive->fSource->CurrentCluster);
        Assert(idx < 1024);
        pDesc->index[idx++] = pDrive->fSource->AddrCurrentCluster;
        // call me paranoid
    };

#endif
    memset(pDesc->index, 0xff, sizeof(pDesc->index));

#endif
}

void Drv08_CreateRDB(drv08_desc_t* pDesc, uint8_t drive_number)
{
    Assert(sizeof(drv08_rdb_t) == 3 * DRV08_BLK_SIZE);
    Assert(offsetof(drv08_rdb_t, rdsk) == 0 * DRV08_BLK_SIZE);
    Assert(offsetof(drv08_rdb_t, part) == 1 * DRV08_BLK_SIZE);
    Assert(offsetof(drv08_rdb_t, fshd) == 2 * DRV08_BLK_SIZE);

    uint32_t lba = pDesc->file_size / 512;

    for (int sec = 1; sec < 255; sec += 2)
        if ((lba % sec) == 0) {
            pDesc->sectors = sec;
        }

    lba /= pDesc->sectors;

    for (int head = 4; head < 16; ++head)
        if ((lba % head) == 0) {
            pDesc->heads = head;
        }

    lba /= pDesc->heads;
    pDesc->cylinders = lba;

    DEBUG(1, "CHS : %u.%u.%u", pDesc->cylinders, pDesc->heads, pDesc->sectors);

    const uint32_t partition_offset = 2; // cylinders
    pDesc->lba_offset = pDesc->sectors * pDesc->heads * partition_offset;
    pDesc->cylinders += partition_offset;

    memset(&pDesc->hdf_rdb, 0x00, sizeof(pDesc->hdf_rdb));

    tRigidDiskBlock* rdsk = &pDesc->hdf_rdb.rdsk;
    tPartitionBlock* part = &pDesc->hdf_rdb.part;
    tFileSysHeaderBlock* fshd = &pDesc->hdf_rdb.fshd;

    // RigidDiskBlock
    {
        WRITE_BE_32B(rdsk->rdb_ID, IDNAME_RIGIDDISK);
        WRITE_BE_32B(rdsk->rdb_SummedLongs, 0x40);
        WRITE_BE_32B(rdsk->rdb_HostID, 0x7);
        WRITE_BE_32B(rdsk->rdb_BlockBytes, DRV08_BLK_SIZE);
        WRITE_BE_32B(rdsk->rdb_Flags, RDBFF_DISKID | RDBFF_LASTLUN);
        WRITE_BE_32B(rdsk->rdb_BadBlockList, 0xffffffff);
        WRITE_BE_32B(rdsk->rdb_PartitionList, 0x01);
        WRITE_BE_32B(rdsk->rdb_FileSysHeaderList, 0x02);
        WRITE_BE_32B(rdsk->rdb_DriveInit, 0xffffffff);

        WRITE_BE_32B(rdsk->rdb_Cylinders, pDesc->cylinders);
        WRITE_BE_32B(rdsk->rdb_Sectors, pDesc->sectors);
        WRITE_BE_32B(rdsk->rdb_Heads, pDesc->heads);
        WRITE_BE_32B(rdsk->rdb_Interleave, 0x01);
        WRITE_BE_32B(rdsk->rdb_Park, pDesc->cylinders);
        WRITE_BE_32B(rdsk->rdb_WritePreComp, pDesc->cylinders);
        WRITE_BE_32B(rdsk->rdb_ReducedWrite, pDesc->cylinders);
        WRITE_BE_32B(rdsk->rdb_StepRate, 0x03);
        WRITE_BE_32B(rdsk->rdb_RDBBlocksLo, 0);
        WRITE_BE_32B(rdsk->rdb_RDBBlocksHi, pDesc->lba_offset - 1);
        WRITE_BE_32B(rdsk->rdb_LoCylinder, partition_offset);
        WRITE_BE_32B(rdsk->rdb_HiCylinder, pDesc->cylinders - 1);
        WRITE_BE_32B(rdsk->rdb_CylBlocks, pDesc->sectors * pDesc->heads);
        WRITE_BE_32B(rdsk->rdb_HighRDSKBlock, 0x03);

        strcpy(rdsk->rdb_DiskVendor, "Replay");
        strcpy(rdsk->rdb_DiskProduct, "Naked");
        strcpy(rdsk->rdb_DiskRevision, "1.0");

        uint32_t chksum = CalcBEsum(rdsk, READ_BE_32B(rdsk->rdb_SummedLongs));
        WRITE_BE_32B(rdsk->rdb_ChkSum, (~chksum) + 1);
    }
    // PartitionBlock
    {
        WRITE_BE_32B(part->pb_ID, IDNAME_PARTITION);
        WRITE_BE_32B(part->pb_SummedLongs, 0x40);
        WRITE_BE_32B(part->pb_HostID, 0x07);
        WRITE_BE_32B(part->pb_Next, 0xffffffff);
        WRITE_BE_32B(part->pb_Flags, PBFF_BOOTABLE);

        // drive name as BSTR
        const char driveName[] = "FPGA0";

        part->pb_DriveName[0] = strlen(driveName);
        memcpy(&part->pb_DriveName[1], driveName, sizeof(driveName));
        part->pb_DriveName[part->pb_DriveName[0]] += drive_number;

        tAmigaPartitionEnvironment* ape = (tAmigaPartitionEnvironment*)&part->pb_Environment[0];

        WRITE_BE_32B(ape->ape_Entries, 0x10);
        WRITE_BE_32B(ape->ape_BlockSize, DRV08_BLK_SIZE / 4);
        WRITE_BE_32B(ape->ape_Heads, pDesc->heads);
        WRITE_BE_32B(ape->ape_SectorsPerBlock, 0x01);
        WRITE_BE_32B(ape->ape_BlocksPerTrack, pDesc->sectors);
        WRITE_BE_32B(ape->ape_Reserved, 0x02);
        WRITE_BE_32B(ape->ape_LoCylinder, 2);
        WRITE_BE_32B(ape->ape_HiCylinder, pDesc->cylinders - 1);
        WRITE_BE_32B(ape->ape_NumBuffers, 0x1e);
        WRITE_BE_32B(ape->ape_MaxTransferRate, 0x1fe00);
        WRITE_BE_32B(ape->ape_MaxTransferMask, 0x7ffffffe);
        WRITE_BE_32B(ape->ape_BootPriority, 0);
        WRITE_BE_32B(ape->ape_DosType, pDesc->hdf_dostype);

        uint32_t chksum = CalcBEsum(part, READ_BE_32B(part->pb_SummedLongs));
        WRITE_BE_32B(part->pb_ChkSum, (~chksum) + 1);
    }
    // FileSysHeaderBlock
    {
        WRITE_BE_32B(fshd->fhb_ID, IDNAME_FILESYSHEADER);
        WRITE_BE_32B(fshd->fhb_SummedLongs, 0x40);
        WRITE_BE_32B(fshd->fhb_HostID, 0x07);
        WRITE_BE_32B(fshd->fhb_Next, 0xffffffff);
        WRITE_BE_32B(fshd->fhb_DosType, pDesc->hdf_dostype);
        WRITE_BE_32B(fshd->fhb_Version, 0x00280001);
        WRITE_BE_32B(fshd->fhb_PatchFlags, 0x00000180);
        WRITE_BE_32B(fshd->fhb_SegListBlocks, 0xffffffff);
        WRITE_BE_32B(fshd->fhb_GlobalVec, 0xffffffff);

        uint32_t chksum = CalcBEsum(fshd, READ_BE_32B(fshd->fhb_SummedLongs));
        WRITE_BE_32B(fshd->fhb_ChkSum, (~chksum) + 1);
    }
}

//
// interface
//

void FileIO_Drv08_Process(uint8_t ch, fch_t handle[2][FCH_MAX_NUM], uint8_t status) // ata
{
    // only one format supported at the moment.
    // we don't know what drive yet, status does not contain unit id (although it could ...)
    Drv08_ATA_Handle(ch, handle);
}

uint8_t FileIO_Drv08_InsertInit(uint8_t ch, uint8_t drive_number, fch_t* pDrive, char* ext)
{
    DEBUG(1, "Drv08:InsertInit");

    HARDWARE_TICK time = Timer_Get(0);

    //pDrive points to the base fch_t struct for this unit. It contains a pointer (pDesc) to our drv08_desc_t

    pDrive->pDesc = calloc(1, sizeof(drv08_desc_t)); // 0 everything

    if (pDrive->pDesc == NULL) {
        WARNING("Drv08:Failed to allocate memory.");
        return (1);
    }

    drv08_desc_t* pDesc = pDrive->pDesc;

    pDesc->format    = (drv08_format_t)XXX;

    if (strnicmp(ext, "HDF", 3) == 0) {
        //
        pDesc->format    = (drv08_format_t)HDF;
        pDesc->file_size =  FF_Size(pDrive->fSource); //->Filesize;

    } else if (strnicmp(ext, "?MMC", 4) == 0) {

        pDesc->format    = (drv08_format_t)MMC;

        uint64_t lba = Card_GetCapacity() / 512;

        DEBUG(1, "MMC LBA = %08x%08x", (uint32_t)(lba >> 32), (uint32_t)(lba & 0xffffffff));

        pDesc->file_size = (uint32_t)(lba);

        for (int sec = 1; sec < 255; sec += 1)
            if ((lba % sec) == 0) {
                pDesc->sectors = sec;
            }

        lba /= pDesc->sectors;

        for (int head = 4; head < 16; ++head)
            if ((lba % head) == 0) {
                pDesc->heads = head;
            }

        lba /= pDesc->heads;
        pDesc->cylinders = lba;

        INFO("SIZE: %lu sectors (%lu MB)", pDesc->file_size, pDesc->file_size >> 11);
        INFO("CHS : %u.%u.%u --> %lu MB", pDesc->cylinders, pDesc->heads, pDesc->sectors,
             ((((unsigned long) pDesc->cylinders) * pDesc->heads * pDesc->sectors) >> 11));

        // skip the rest of the setup - we're all done
        return (0);

    } else {
        WARNING("Drv08:Unsupported format.");
        return (1);
    }

    // common stuff (as only HDF supported for now...

    if (Drv08_GetHardfileType(pDrive, pDesc)) {
        return (1);
    }

    Drv08_GetHardfileGeometry(pDrive, pDesc);
    Drv08_BuildHardfileIndex(pDrive, pDesc);
    time = Timer_Get(0) - time;

    if (pDesc->format == HDF_NAKED) {
        Drv08_CreateRDB(pDesc, drive_number);
    }

    INFO("SIZE: %lu (%lu MB)", pDesc->file_size, pDesc->file_size >> 20);
    INFO("CHS : %u.%u.%u --> %lu MB", pDesc->cylinders, pDesc->heads, pDesc->sectors,
         ((((unsigned long) pDesc->cylinders) * pDesc->heads * pDesc->sectors) >> 11));
    INFO("Opened in %lu ms", Timer_Convert(time));

    if (pDesc->format == HDF && 2 <= debuglevel) {
        Drv08_PrintRDB(pDrive, pDesc);
    }

    return (0);
}

