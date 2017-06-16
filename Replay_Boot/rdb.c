#include "rdb.h"
#include "card.h"
#include "hardblocks.h"
#include "messaging.h"
#include <stdint.h>

#define BLK_SIZE 512

typedef struct {
	uint8_t		active;
	uint8_t		start_head;
	uint8_t		start_sector:6;
	uint8_t		start_cylinder_hi:2;
	uint8_t		start_cylinder_lo;
	uint8_t		file_system;
	uint8_t		end_head;
	uint8_t		end_sector:6;
	uint8_t		end_cylinder_hi:2;
	uint8_t		end_cylinder_lo;
	uint32_t	first_block;
	uint32_t	total_blocks;
} __attribute__ ((packed)) tPartition;

typedef struct {
	uint8_t		bootstrap[0x1b8];
	uint8_t		serial[4];
	uint8_t 	reserved[2];
	tPartition	partition[4];
	uint8_t		signature[2];
} __attribute__ ((packed)) tMBR;

#define cyl(hi,lo) (((hi) << 8) | (lo))
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

    for (int i = 0; i < numlongs; i++ ) {
        uint32_t v = READ_BE_32B(*p);
        chksum += (int32_t)v;
        p++;
    }

    return chksum;
}


void SynchronizeRdbWithMbr()
{
	uint8_t num_partitions = 0;

	Assert(sizeof(tMBR) == 512);
	// 1. read the mbr
	tMBR mbr;
	FF_ERROR err = Card_ReadM((uint8_t*)&mbr, 0, 1, NULL);
	if (err != FF_ERR_NONE) {
		WARNING("Reading MBR failed! %08x", err);
		return;
	}

	DumpBuffer((uint8_t*)&mbr, sizeof(mbr));

	DEBUG(1,"MBR serial : %02x%02x%02x%02x", mbr.serial[0], mbr.serial[1], mbr.serial[2], mbr.serial[3]);
	DEBUG(1,"MBR signature : %02x%02x", mbr.signature[0], mbr.signature[1]);

	for (int i = 0; i < 4; ++i) {
		tPartition* part = &mbr.partition[i];
		if (part->active != 0x80 && (part->active != 0 || part->file_system == 0)) {
			break;
		}

		num_partitions++;

		DEBUG(1,"MBR partition %d :", i);
		DEBUG(1,"                   active = %d", part->active);
		DEBUG(1,"                   start_head = 0x%02x", part->start_head);
		DEBUG(1,"                   start_sector = 0x%02x", part->start_sector);
		DEBUG(1,"                   start_cylinder = 0x%03x", cyl(part->start_cylinder_hi, part->start_cylinder_lo));
		DEBUG(1,"                   file_system = %02x", part->file_system);
		DEBUG(1,"                   end_head = 0x%02x", part->end_head);
		DEBUG(1,"                   end_sector = 0x%02x", part->end_sector);
		DEBUG(1,"                   end_cylinder = 0x%03x", cyl(part->end_cylinder_hi, part->end_cylinder_lo));
		DEBUG(1,"                   first_block = %d / %08x", part->first_block, part->first_block);
		DEBUG(1,"                   total_blocks = %d / %08x", part->total_blocks, part->total_blocks);
	}

	if (num_partitions == 0) {
		WARNING("No partitions found!");
		return;
	}

	uint32_t cur_block = 2;	// skip the MBR sector at lba 0
	uint32_t min_blocks_needed = 1 /* RSDK */ + num_partitions /* PART */;// + 1 /* FSHD */;

	if (mbr.partition[0].file_system == 0xee) {
		WARNING("GUID / GPT format not supported");
	}
	if (mbr.partition[0].first_block < min_blocks_needed + cur_block) {
		WARNING("Not enough space between MBR and PART");
		return;
	}

    uint32_t lba = Card_GetCapacity() / 512;
	uint32_t cylblocks = mbr.partition[0].first_block / 2;		// first partition is assumed to be at cylinder 2

    uint32_t cylinders = lba / cylblocks;

    uint32_t sectors = 0;
    for (int sec = 1; sec < 255; sec += 1)
        if ((cylblocks % sec) == 0) {
            sectors = sec;
        }

    uint32_t heads = cylblocks / sectors;

    DEBUG(1, "lba = 0x%08x", lba);
    DEBUG(1, "cylinders = %d", cylinders);
    DEBUG(1, "heads = %d", heads);
    DEBUG(1, "sectors = %d", sectors);
    DEBUG(1, "cylblocks = %d", cylblocks);

	for (int i = 0; i < num_partitions; ++i) {
		tPartition* part = &mbr.partition[i];

		if (part->first_block % cylblocks != 0)
		{
			WARNING("Partition #%d start unaligned!", i);
			return;
		}
		if (part->total_blocks % cylblocks != 0)
		{
			WARNING("Partition #%d size unaligned!", i);
			return;
		}
		DEBUG(1, "PART #%d : LoCylinder = %d ; HiCylinder = %d", i, part->first_block / cylblocks, ((part->first_block+part->total_blocks) / cylblocks) - 1);
	}


	{
		tRigidDiskBlock rdsk;
		memset(&rdsk, 0x00, sizeof(rdsk));
        WRITE_BE_32B(rdsk.rdb_ID, IDNAME_RIGIDDISK);
        WRITE_BE_32B(rdsk.rdb_SummedLongs, 0x40);
        WRITE_BE_32B(rdsk.rdb_HostID, 0x7);
        WRITE_BE_32B(rdsk.rdb_BlockBytes, BLK_SIZE);
        WRITE_BE_32B(rdsk.rdb_Flags, RDBFF_DISKID | RDBFF_LASTLUN);
        WRITE_BE_32B(rdsk.rdb_BadBlockList, 0xffffffff);
        WRITE_BE_32B(rdsk.rdb_PartitionList, cur_block+1);
        WRITE_BE_32B(rdsk.rdb_FileSysHeaderList, 0xffffffff);
        WRITE_BE_32B(rdsk.rdb_DriveInit, 0xffffffff);

        WRITE_BE_32B(rdsk.rdb_Cylinders, cylinders);
        WRITE_BE_32B(rdsk.rdb_Sectors, sectors);
        WRITE_BE_32B(rdsk.rdb_Heads, heads);
        WRITE_BE_32B(rdsk.rdb_Interleave, 0x01);
        WRITE_BE_32B(rdsk.rdb_Park, cylinders);
        WRITE_BE_32B(rdsk.rdb_WritePreComp, cylinders);
        WRITE_BE_32B(rdsk.rdb_ReducedWrite, cylinders);
        WRITE_BE_32B(rdsk.rdb_StepRate, 0x03);
        WRITE_BE_32B(rdsk.rdb_RDBBlocksLo, 0);
        WRITE_BE_32B(rdsk.rdb_RDBBlocksHi, cylblocks * 2 - 1);
        WRITE_BE_32B(rdsk.rdb_LoCylinder, 2);
        WRITE_BE_32B(rdsk.rdb_HiCylinder, cylinders - 1);
        WRITE_BE_32B(rdsk.rdb_CylBlocks, cylblocks);
        WRITE_BE_32B(rdsk.rdb_HighRDSKBlock, cur_block + min_blocks_needed);

        strcpy(rdsk.rdb_DiskVendor, "Replay");
        strcpy(rdsk.rdb_DiskProduct, "MBR/RDB");
        strcpy(rdsk.rdb_DiskRevision, "1.0");

        uint32_t chksum = CalcBEsum(&rdsk, READ_BE_32B(rdsk.rdb_SummedLongs));
        WRITE_BE_32B(rdsk.rdb_ChkSum, (~chksum) + 1);

		DumpBuffer((uint8_t*)&rdsk, sizeof(rdsk));

        Card_WriteM((uint8_t*)&rdsk, cur_block++, 1, NULL);
    }

    for (int i = 0; i < num_partitions; ++i) {
		tPartitionBlock part;

		tPartition* p = &mbr.partition[i];
		uint32_t locyl = p->first_block / cylblocks;
		uint32_t hicyl = locyl + p->total_blocks / cylblocks - 1;

		memset(&part, 0x00, sizeof(part));
        WRITE_BE_32B(part.pb_ID, IDNAME_PARTITION);
        WRITE_BE_32B(part.pb_SummedLongs, 0x40);
        WRITE_BE_32B(part.pb_HostID, 0x07);
        WRITE_BE_32B(part.pb_Next, i == num_partitions - 1 ? 0xffffffff : cur_block+1);
        WRITE_BE_32B(part.pb_Flags, PBFF_BOOTABLE);

        // drive name as BSTR
        const char driveName[] = "MMC0";

        part.pb_DriveName[0] = strlen(driveName);
        memcpy(&part.pb_DriveName[1], driveName, sizeof(driveName));
        part.pb_DriveName[part.pb_DriveName[0]] += i;

        tAmigaPartitionEnvironment* ape = (tAmigaPartitionEnvironment*)&part.pb_Environment[0];

        WRITE_BE_32B(ape->ape_Entries, 0x10);
        WRITE_BE_32B(ape->ape_BlockSize, BLK_SIZE / 4);
        WRITE_BE_32B(ape->ape_Heads, heads);
        WRITE_BE_32B(ape->ape_SectorsPerBlock, 0x01);
        WRITE_BE_32B(ape->ape_BlocksPerTrack, sectors);
        WRITE_BE_32B(ape->ape_Reserved, 0x02);
        WRITE_BE_32B(ape->ape_LoCylinder, locyl);
        WRITE_BE_32B(ape->ape_HiCylinder, hicyl);
        WRITE_BE_32B(ape->ape_NumBuffers, 0x1e);
        WRITE_BE_32B(ape->ape_MaxTransferRate, 0x1fe00);
        WRITE_BE_32B(ape->ape_MaxTransferMask, 0x7ffffffe);
        WRITE_BE_32B(ape->ape_BootPriority, 0);
        WRITE_BE_32B(ape->ape_DosType, 0x444f5301);

        uint32_t chksum = CalcBEsum(&part, READ_BE_32B(part.pb_SummedLongs));
        WRITE_BE_32B(part.pb_ChkSum, (~chksum) + 1);

		DumpBuffer((uint8_t*)&part, sizeof(part));

        Card_WriteM((uint8_t*)&part, cur_block++, 1, NULL);
    }
    INFO("RDB written");
}
