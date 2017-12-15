/*****************************************************************************
 *     FullFAT - High Performance, Thread-Safe Embedded FAT File-System      *
 *                                                                           *
 *        Copyright(C) 2009  James Walmsley  <james@fullfat-fs.co.uk>        *
 *        Copyright(C) 2011  Hein Tibosch    <hein_tibosch@yahoo.es>         *
 *                                                                           *
 *    See RESTRICTIONS.TXT for extra restrictions on the use of FullFAT.     *
 *                                                                           *
 *    WARNING : COMMERCIAL PROJECTS MUST COMPLY WITH THE GNU GPL LICENSE.    *
 *                                                                           *
 *  Projects that cannot comply with the GNU GPL terms are legally obliged   *
 *    to seek alternative licensing. Contact James Walmsley for details.     *
 *                                                                           *
 *****************************************************************************
 *           See http://www.fullfat-fs.co.uk/ for more information.          *
 *****************************************************************************
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 *  The Copyright of Hein Tibosch on this project recognises his efforts in  *
 *  contributing to this project. The right to license the project under     *
 *  any other terms (other than the GNU GPL license) remains with the        *
 *  original copyright holder (James Walmsley) only.                         *
 *                                                                           *
 *****************************************************************************
 *  Modification/Extensions/Bugfixes/Improvements to FullFAT must be sent to *
 *  James Walmsley for integration into the main development branch.         *
 *****************************************************************************/

/**
 *	@file		ff_format.c
 *	@author		James Walmsley
 *	@ingroup	FORMAT
 *
 *	@defgroup	FORMAT Independent FAT Formatter
 *	@brief		Provides an interface to format a partition with FAT.
 *
 *
 *
 **/


#include "ff_format.h"
#include "ff_types.h"
#include "ff_ioman.h"
#include "ff_fatdef.h"

static FF_T_SINT8 FF_PartitionCount (FF_T_UINT8 *pBuffer)
{
	FF_T_SINT8 count = 0;
	FF_T_SINT8 part;
	// Check PBR or MBR signature
	if (FF_getChar(pBuffer, FF_FAT_MBR_SIGNATURE) != 0x55 &&
		FF_getChar(pBuffer, FF_FAT_MBR_SIGNATURE) != 0xAA ) {
		// No MBR, but is it a PBR ?
		if (FF_getChar(pBuffer, 0) == 0xEB &&          // PBR Byte 0
		    FF_getChar(pBuffer, 2) == 0x90 &&          // PBR Byte 2
		    (FF_getChar(pBuffer, 21) & 0xF0) == 0xF0) {// PBR Byte 21 : Media byte
			return 1;	// No MBR but PBR exist then only one partition
		}
		return 0;   // No MBR and no PBR then no partition found
	}
	for (part = 0; part < 4; part++)  {
		FF_T_UINT8 active = FF_getChar(pBuffer, FF_FAT_PTBL + FF_FAT_PTBL_ACTIVE + (16 * part));
		FF_T_UINT8 part_id = FF_getChar(pBuffer, FF_FAT_PTBL + FF_FAT_PTBL_ID + (16 * part));
		// The first sector must be a MBR, then check the partition entry in the MBR
		if (active != 0x80 && (active != 0 || part_id == 0)) {
			break;
		}
		count++;
	}
	return count;
}


FF_ERROR FF_CreatePartitionTable(FF_IOMAN *pIoman, FF_T_UINT32 ulTotalDeviceBlocks, FF_PARTITION_TABLE *pPTable) {
	FF_BUFFER *pBuffer = FF_GetBuffer(pIoman, 0, FF_MODE_WRITE);
	{
		if(!pBuffer) {
			return FF_ERR_DEVICE_DRIVER_FAILED;
		}


	}
	FF_ReleaseBuffer(pIoman, pBuffer);

	return FF_ERR_NONE;
}


FF_ERROR FF_FormatPartition(FF_IOMAN *pIoman, FF_T_UINT32 ulPartitionNumber, FF_T_UINT32 ulClusterSize) {

	FF_BUFFER *pBuffer;
  	FF_T_SINT8	scPartitionCount;
	FF_T_UINT32 maxClusters, f16MaxClusters, f32MaxClusters;
	FF_T_UINT32 fatSize = 32; // Default to a fat32 format.

	FF_PARTITION_ENTRY partitionGeom;

	FF_T_UINT32 ulBPRLba; ///< The LBA of the boot partition record.

	FF_T_UINT32 fat32Size, fat16Size, newFat32Size, newFat16Size, finalFatSize;
	FF_T_UINT32 sectorsPerCluster = ulClusterSize / pIoman->BlkSize;

	FF_T_UINT32 ulReservedSectors = 0, ulTotalSectors = 0;

	FF_T_UINT32 ul32DataSectors, ul16DataSectors;
	FF_T_UINT32 i;

	FF_T_UINT32 ulClusterBeginLBA;

	FF_ERROR	Error = FF_ERR_NONE;

	partitionGeom.ulStartLBA = 0;
	partitionGeom.ulLength = 0;

	// Get Partition Metrics, and pass on to FF_Format() function

	pBuffer = FF_GetBuffer(pIoman, 0, FF_MODE_READ);
	{
		if(!pBuffer) {
			return FF_ERR_DEVICE_DRIVER_FAILED | FF_FORMATPARTITION;
		}


		scPartitionCount = FF_PartitionCount(pBuffer->pBuffer);

		if(!scPartitionCount) {
			// Get Partition Geom from volume boot record.
			ulBPRLba = 0;
			partitionGeom.ulStartLBA = FF_getShort(pBuffer->pBuffer, FF_FAT_RESERVED_SECTORS); // Get offset to start of where we can actually put the FAT table.
			ulReservedSectors = partitionGeom.ulStartLBA;
			partitionGeom.ulLength = (FF_T_UINT32) FF_getShort(pBuffer->pBuffer, FF_FAT_16_TOTAL_SECTORS);

			if(partitionGeom.ulLength == 0) { // 32-bit entry was used.
				partitionGeom.ulLength = FF_getLong(pBuffer->pBuffer, FF_FAT_32_TOTAL_SECTORS);
			}

			ulTotalSectors = partitionGeom.ulLength;

			partitionGeom.ulLength -= partitionGeom.ulStartLBA; // Remove the reserved sectors from the count.

		} else {
			// Get partition Geom from the partition table entry.

		}

		// Calculate the max possiblenumber of clusters based on clustersize.
		maxClusters = partitionGeom.ulLength / sectorsPerCluster;

		// Determine the size of a FAT table required to support this.
		fat32Size = (maxClusters * 4) / pIoman->BlkSize; // Potential size in sectors of a fat32 table.
		if((maxClusters * 4) % pIoman->BlkSize) {
			fat32Size++;
		}
		fat32Size *= 2;	// Officially there are 2 copies of the FAT.

		fat16Size = (maxClusters * 2) / pIoman->BlkSize; // Potential size in bytes of a fat16 table.
		if((maxClusters * 2) % pIoman->BlkSize) {
			fat16Size++;
		}
		fat16Size *= 2;

		// A real number of sectors to be available is therefore ~~
		ul16DataSectors = partitionGeom.ulLength - fat16Size;
		ul32DataSectors = partitionGeom.ulLength - fat32Size;

		f16MaxClusters = ul16DataSectors / sectorsPerCluster;
		f32MaxClusters = ul32DataSectors / sectorsPerCluster;

		newFat16Size = (f16MaxClusters * 2) / pIoman->BlkSize;
		if((f16MaxClusters * 2) % pIoman->BlkSize) {
			newFat16Size++;
		}

		newFat32Size = (f32MaxClusters * 4) / pIoman->BlkSize;
		if((f32MaxClusters * 4) % pIoman->BlkSize) {
			newFat32Size++;
		}

		// Now determine if this should be fat16/32 format?

		if(f16MaxClusters < 65525) {
			fatSize = 16;
			finalFatSize = newFat16Size;
		} else {
			fatSize = 32;
			finalFatSize = newFat32Size;
		}

		FF_ReleaseBuffer(pIoman, pBuffer);
		for(i = 0; i < finalFatSize*2; i++) { // Ensure the FAT table is clear.
			if(i == 0) {
				pBuffer = FF_GetBuffer(pIoman, partitionGeom.ulStartLBA, FF_MODE_WR_ONLY);
				if(!pBuffer) {
					return FF_ERR_DEVICE_DRIVER_FAILED;
				}

				memset(pBuffer->pBuffer, 0, pIoman->BlkSize);
			} else {
				FF_BlockWrite(pIoman, partitionGeom.ulStartLBA+i, 1, pBuffer->pBuffer, FF_FALSE);
			}
		}

		switch(fatSize) {
		case 16: {
			FF_putShort(pBuffer->pBuffer, 0, 0xFFF8); // First FAT entry.
			FF_putShort(pBuffer->pBuffer, 2, 0xFFFF); // RESERVED alloc.
			break;
		}

		case 32: {
			FF_putLong(pBuffer->pBuffer, 0, 0x0FFFFFF8); // FAT32 FAT sig.
			FF_putLong(pBuffer->pBuffer, 4, 0xFFFFFFFF); // RESERVED alloc.
			FF_putLong(pBuffer->pBuffer, 8, 0x0FFFFFFF); // Root dir allocation.
			break;
		}

		default:
			break;
		}

		FF_ReleaseBuffer(pIoman, pBuffer);


		// Clear and initialise the root dir.
		ulClusterBeginLBA = partitionGeom.ulStartLBA + (finalFatSize*2);

		for(i = 0; i < sectorsPerCluster; i++) {
			if(i == 0) {
				pBuffer = FF_GetBuffer(pIoman, ulClusterBeginLBA, FF_MODE_WR_ONLY);
				memset(pBuffer->pBuffer, 0, pIoman->BlkSize);
			} else  {
				FF_BlockWrite(pIoman, ulClusterBeginLBA+i, 1, pBuffer->pBuffer, FF_FALSE);
			}

		}

		FF_ReleaseBuffer(pIoman, pBuffer);

		// Correctly modify the second FAT item again.
		pBuffer = FF_GetBuffer(pIoman, partitionGeom.ulStartLBA + finalFatSize, FF_MODE_WRITE);
		{
			switch(fatSize) {
			case 16: {
				FF_putShort(pBuffer->pBuffer, 0, 0xFFF8);
				FF_putShort(pBuffer->pBuffer, 2, 0xFFFF);
				break;
			}
			   
			case 32:
				FF_putLong(pBuffer->pBuffer, 0, 0x0FFFFFF8);
				FF_putLong(pBuffer->pBuffer, 4, 0xFFFFFFFF);
				FF_putLong(pBuffer->pBuffer, 8, 0x0FFFFFFF); // Root dir allocation.
			}
		}
		FF_ReleaseBuffer(pIoman, pBuffer);


		// Modify the fields in the VBR/PBR for correct mounting.
		pBuffer = FF_GetBuffer(pIoman, ulBPRLba, FF_MODE_WRITE); 		// Modify the FAT descriptions.
		{

			// -- First section Common vars to Fat12/16 and 32.
			memset(pBuffer->pBuffer, 0, pIoman->BlkSize); 				// Clear the boot record.

			FF_putChar(pBuffer->pBuffer, 0, 0xEB);						// Place the Jump to bootstrap x86 instruction.
			FF_putChar(pBuffer->pBuffer, 1, 0x3C);						// Even though we won't populate the bootstrap code.
			FF_putChar(pBuffer->pBuffer, 2, 0x90);						// Some devices look for this as a signature.

			memcpy(((FF_T_UINT8 *)pBuffer->pBuffer+3), "FULLFAT2", 8); // Place the FullFAT OEM code.

			FF_putShort(pBuffer->pBuffer, 11, pIoman->BlkSize);
			FF_putChar(pBuffer->pBuffer, 13, (FF_T_UINT8) sectorsPerCluster);

			FF_putShort(pBuffer->pBuffer, FF_FAT_RESERVED_SECTORS, (FF_T_UINT16)partitionGeom.ulStartLBA); 	// Number of reserved sectors. (1 for fat12/16, 32 for f32).
			FF_putShort(pBuffer->pBuffer, FF_FAT_NUMBER_OF_FATS, 2); 	// Always 2 copies.


			//FF_putShort(pBuffer->pBuffer, 19, 0);						// Number of sectors in partition if size < 32mb.

			FF_putChar(pBuffer->pBuffer, 21, 0xF8);             		// Media type -- HDD.
		   

			FF_putShort(pBuffer->pBuffer, 510, 0xAA55);					// MBR sig.
			FF_putLong(pBuffer->pBuffer, 32, partitionGeom.ulLength+partitionGeom.ulStartLBA); // Total sectors of this partition.

			if(fatSize == 32) {
				FF_putShort(pBuffer->pBuffer, 36, (FF_T_UINT16)finalFatSize);		// Number of sectors per fat. 
				FF_putShort(pBuffer->pBuffer, 44, 2);					// Root dir cluster (2).
				FF_putShort(pBuffer->pBuffer, 48, 1);					// FSINFO sector at LBA1.
				FF_putShort(pBuffer->pBuffer, 50, 6);					// 0 for no backup boot sector.
				FF_putChar(pBuffer->pBuffer, 66, 0x29);					// Indicate extended signature is present.
				memcpy(((FF_T_UINT8 *)pBuffer->pBuffer+71), "FullFAT2-V", 10); // Volume name.
				memcpy(((FF_T_UINT8 *)pBuffer->pBuffer+81), "FAT32   ", 8);

				// Put backup boot sector.
				FF_BlockWrite(pIoman, 6, 1, pBuffer->pBuffer, FF_FALSE);
			} else {
				FF_putChar(pBuffer->pBuffer, 38, 0x28);					// Signal this contains an extended signature.
				memcpy(((FF_T_UINT8 *)pBuffer->pBuffer+43), "FullFAT2-V", 10); // Volume name.
				memcpy(((FF_T_UINT8 *)pBuffer->pBuffer+54), "FAT16   ", 8);
				FF_putShort(pBuffer->pBuffer, FF_FAT_16_SECTORS_PER_FAT, (FF_T_UINT16) finalFatSize);
				FF_putShort(pBuffer->pBuffer, 17, 512);				 		// Number of Dir entries. (FAT32 0).
				//FF_putShort(pBuffer->pBuffer, FF_FAT_ROOT_ENTRY_COUNT, 
			}
		}
	}
	FF_ReleaseBuffer(pIoman, pBuffer);

	if(fatSize == 32) {
		// Finally if FAT32, create an FSINFO sector.
		pBuffer = FF_GetBuffer(pIoman, 1, FF_MODE_WRITE);
		{
			memset(pBuffer->pBuffer, 0, pIoman->BlkSize);
			FF_putLong(pBuffer->pBuffer, 0, 0x41615252);	// FSINFO sect magic number.
			FF_putLong(pBuffer->pBuffer, 484, 0x61417272);	// FSINFO second sig.
			// Calculate total sectors, -1 for the root dir allocation. (Free sectors count).
			FF_putLong(pBuffer->pBuffer, 488, ((ulTotalSectors - (ulReservedSectors + (2*finalFatSize))) / sectorsPerCluster)-1);
			FF_putLong(pBuffer->pBuffer, 492, 2);			// Hint for next free cluster.
			FF_putShort(pBuffer->pBuffer, 510, 0xAA55);
		}
		FF_ReleaseBuffer(pIoman, pBuffer);
	}

	FF_FlushCache(pIoman);

	return Error;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// FreeRTOS+FAT dependencies 
typedef int BaseType_t;
typedef unsigned int UBaseType_t;
#define pdFALSE			( ( BaseType_t ) 0 )
#define pdTRUE			( ( BaseType_t ) 1 )
#define	ffconfigMAX_PARTITIONS				4
//void MSG_debug(uint8_t do_osd, const char* file, unsigned int line, char* fmt, ...);
//#define FF_PRINTF(...) do { MSG_debug(0, __FILE__, __LINE__, __VA_ARGS__); } while(0)
#define FF_PRINTF(...) do { /* disabled */ } while(0)

/*
 * FreeRTOS+FAT Labs Build 160919 (C) 2016 Real Time Engineers ltd.
 * Authors include James Walmsley, Hein Tibosch and Richard Barry
 *
 *******************************************************************************
 ***** NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ***
 ***                                                                         ***
 ***                                                                         ***
 ***   FREERTOS+FAT IS STILL IN THE LAB:                                     ***
 ***                                                                         ***
 ***   This product is functional and is already being used in commercial    ***
 ***   products.  Be aware however that we are still refining its design,    ***
 ***   the source code does not yet fully conform to the strict coding and   ***
 ***   style standards mandated by Real Time Engineers ltd., and the         ***
 ***   documentation and testing is not necessarily complete.                ***
 ***                                                                         ***
 ***   PLEASE REPORT EXPERIENCES USING THE SUPPORT RESOURCES FOUND ON THE    ***
 ***   URL: http://www.FreeRTOS.org/contact  Active early adopters may, at   ***
 ***   the sole discretion of Real Time Engineers Ltd., be offered versions  ***
 ***   under a license other than that described below.                      ***
 ***                                                                         ***
 ***                                                                         ***
 ***** NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ***
 *******************************************************************************
 *
 * FreeRTOS+FAT can be used under two different free open source licenses.  The
 * license that applies is dependent on the processor on which FreeRTOS+FAT is
 * executed, as follows:
 *
 * If FreeRTOS+FAT is executed on one of the processors listed under the Special
 * License Arrangements heading of the FreeRTOS+FAT license information web
 * page, then it can be used under the terms of the FreeRTOS Open Source
 * License.  If FreeRTOS+FAT is used on any other processor, then it can be used
 * under the terms of the GNU General Public License V2.  Links to the relevant
 * licenses follow:
 *
 * The FreeRTOS+FAT License Information Page: http://www.FreeRTOS.org/fat_license
 * The FreeRTOS Open Source License: http://www.FreeRTOS.org/license
 * The GNU General Public License Version 2: http://www.FreeRTOS.org/gpl-2.0.txt
 *
 * FreeRTOS+FAT is distributed in the hope that it will be useful.  You cannot
 * use FreeRTOS+FAT unless you agree that you use the software 'as is'.
 * FreeRTOS+FAT is provided WITHOUT ANY WARRANTY; without even the implied
 * warranties of NON-INFRINGEMENT, MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. Real Time Engineers Ltd. disclaims all conditions and terms, be they
 * implied, expressed, or statutory.
 *
 * 1 tab == 4 spaces!
 *
 * http://www.FreeRTOS.org
 * http://www.FreeRTOS.org/plus
 * http://www.FreeRTOS.org/labs
 *
 */

/**
 *	@file		ff_ioman.c
 *	@ingroup	IOMAN
 *
 *	@defgroup	IOMAN	I/O Manager
 *	@brief		Handles IO buffers for FreeRTOS+FAT safely.
 *
 *	Provides a simple static interface to the rest of FreeRTOS+FAT to manage
 *	buffers. It also defines the public interfaces for Creating and
 *	Destroying a FreeRTOS+FAT IO object.
 **/

#define FF_FAT_PTBL_SECT_COUNT		0x00C
#define	FF_FAT_MEDIA_TYPE           0x015

#define FF_DOS_EXT_PART             0x05
#define FF_LINUX_EXT_PART           0x85
#define FF_WIN98_EXT_PART           0x0f

/* 'Internal' to FreeRTOS+FAT. */
typedef struct _SPart
{
	uint32_t ulStartLBA;		/* FF_FAT_PTBL_LBA */
	uint32_t ulSectorCount;		/* FF_FAT_PTBL_SECT_COUNT */
	uint32_t
			ucActive : 8,		/* FF_FAT_PTBL_ACTIVE */
			ucPartitionID : 8,	/* FF_FAT_PTBL_ID */
			bIsExtended : 1;
} FF_Part_t;

typedef struct _SPartFound
{
	int iCount;
	FF_Part_t pxPartitions[ffconfigMAX_PARTITIONS];
} FF_SPartFound_t;

/*-----------------------------------------------------------*/

/* Check if ucPartitionID introduces an extended partition. */
static BaseType_t prvIsExtendedPartition( uint8_t ucPartitionID )
{
BaseType_t xResult;

	if( ( ucPartitionID == FF_DOS_EXT_PART ) ||
		( ucPartitionID == FF_WIN98_EXT_PART ) ||
		( ucPartitionID == FF_LINUX_EXT_PART ) )
	{
		xResult = pdTRUE;
	}
	else
	{
		xResult = pdFALSE;
	}

	return xResult;
}	/* prvIsExtendedPartition() */
/*-----------------------------------------------------------*/

/* Check if the media byte in an MBR is valid */
static BaseType_t prvIsValidMedia( uint8_t media )
{
BaseType_t xResult;
	/*
	 * 0xF8 is the standard value for “fixed” (non-removable) media. For
	 * removable media, 0xF0 is frequently used. The legal values for this
	 * field are 0xF0, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, and
	 * 0xFF. The only other important point is that whatever value is put
	 * in here must also be put in the low byte of the FAT[0] entry. This
	 * dates back to the old MS-DOS 1.x media determination noted
	 * earlier and is no longer usually used for anything.
	 */
	if( ( 0xf8 <= media ) || ( media == 0xf0 ) )
	{
		xResult = pdTRUE;
	}
	else
	{
		xResult = pdFALSE;
	}

	return xResult;
}	/* prvIsValidMedia() */

/*-----------------------------------------------------------*/

/* This function will parse the 4 entries in a partition table: */
void FF_ReadParts( uint8_t *pucBuffer, FF_Part_t *pxParts )
{
BaseType_t xPartNr;
UBaseType_t uxOffset = FF_FAT_PTBL;

	/* pxParts is expected to be declared as an array of 4 elements:
		FF_Part_t pxParts[4];
		FF_ReadParts( pxBuffer->pucBuffer, pxParts );
	*/
	for( xPartNr = 0; xPartNr < 4; xPartNr++, uxOffset += 16, pxParts++ )
	{
		pxParts->ucActive = FF_getChar( pucBuffer, uxOffset + FF_FAT_PTBL_ACTIVE );
		pxParts->ucPartitionID = FF_getChar( pucBuffer, uxOffset + FF_FAT_PTBL_ID );
		pxParts->ulSectorCount = FF_getLong( pucBuffer, uxOffset + FF_FAT_PTBL_SECT_COUNT );
		pxParts->ulStartLBA = FF_getLong( pucBuffer, uxOffset + FF_FAT_PTBL_LBA );
	}
}

/*-----------------------------------------------------------*/

/*  This function will traverse through a chain of extended partitions. */

/* It is protected against rubbish data by a counter. */
static FF_ERROR FF_ParseExtended( FF_IOMAN *pxIOManager, uint32_t ulFirstSector, uint32_t ulFirstSize,
	FF_SPartFound_t *pPartsFound )
{
uint32_t ulThisSector, ulThisSize;
uint32_t ulSectorSize = pxIOManager->BlkSize / 512;
uint32_t prevTotalSectors = pxIOManager->pPartition->TotalSectors;
FF_BUFFER *pxBuffer = NULL;
BaseType_t xTryCount = 100;
BaseType_t xPartNr;
BaseType_t xExtendedPartNr;
FF_ERROR xError = FF_ERR_NONE;
FF_Part_t pxPartitions[ 4 ];

	ulThisSector = ulFirstSector;
	ulThisSize = ulFirstSize;

	/* Disable sector checking in FF_BlockRead, because the
	exact disk (partition) parameters are not yet known.
	Let user driver return an error is appropriate. */
	pxIOManager->pPartition->TotalSectors = 0;

	while( xTryCount-- )
	{
		if( ( pxBuffer == NULL ) || ( pxBuffer->Sector != ulThisSector ) )
		{
			/* Moving to a different sector. Release the
			previous one and allocate a new buffer. */
			if( pxBuffer != NULL )
			{
				xError = FF_ReleaseBuffer( pxIOManager, pxBuffer );
				pxBuffer = NULL;
				if( FF_isERR( xError ) )
				{
					break;
				}
			}
			FF_PRINTF( "FF_ParseExtended: Read sector %u\n", ( unsigned) ulThisSector );
			pxBuffer = FF_GetBuffer( pxIOManager, ulThisSector, FF_MODE_READ );
			if( pxBuffer == NULL )
			{
				xError = FF_PARSEEXTENDED | FF_ERR_DEVICE_DRIVER_FAILED; /* | FUNCTION...; */
				break;
			}
		}

		{
		uint8_t a = FF_getChar( pxBuffer->pBuffer, FF_FAT_MBR_SIGNATURE + 0 );
		uint8_t b = FF_getChar( pxBuffer->pBuffer, FF_FAT_MBR_SIGNATURE + 1 );

			if( ( a != 0x55 ) || ( b != 0xAA ) )
			{
				FF_PRINTF( "FF_ParseExtended: No signature %02X,%02X\n", a, b );
				break;
			}
		}

		/* Check for data partition(s),
		and remember if there is an extended partition */

		FF_ReadParts( pxBuffer->pBuffer, pxPartitions );

		/* Assume there is no next ext partition. */
		xExtendedPartNr = -1;

		for( xPartNr = 0; xPartNr < 4; xPartNr++ )
		{
		uint32_t ulOffset, ulSize, ulNext;
			if( pxPartitions[ xPartNr ].ulSectorCount == 0 )
			{
				/* Partition is empty */
				continue;
			}

			if( prvIsExtendedPartition( pxPartitions[ xPartNr ].ucPartitionID ) )
			{
				if( xExtendedPartNr < 0 )
				{
					xExtendedPartNr = xPartNr;
				}

				continue;	/* We'll examine this ext partition later */
			}

			/* Some sanity checks */
			ulOffset = pxPartitions[ xPartNr ].ulStartLBA * ulSectorSize;
			ulSize = pxPartitions[ xPartNr ].ulSectorCount * ulSectorSize;
			ulNext = ulThisSector + ulOffset;
			if(
				/* Is it oversized? */
				( ulOffset + ulSize > ulThisSize ) ||
				/* or going backward? */
				( ulNext < ulFirstSector ) ||
				/* Or outsize the logical partition? */
				( ulNext > ulFirstSector + ulFirstSize )
				)
			{
				FF_PRINTF( "Part %d looks insane: ulThisSector %u ulOffset %u ulNext %u\n",
					( int ) xPartNr, ( unsigned )ulThisSector, ( unsigned )ulOffset, ( unsigned )ulNext );
				continue;
			}

			{
				/* Store this partition for the caller */
				FF_Part_t *p = &pPartsFound->pxPartitions[ pPartsFound->iCount++ ];

				/* Copy the whole structure */
				memcpy( p, pxPartitions + xPartNr, sizeof( *p ) );

				/* and make LBA absolute to sector-0. */
				p->ulStartLBA += ulThisSector;
				p->bIsExtended = pdTRUE;
			}

			if( pPartsFound->iCount >= ffconfigMAX_PARTITIONS )
			{
				break;
			}

			xTryCount = 100;
		}	/* for( xPartNr = 0; xPartNr < 4; xPartNr++ ) */

		if( xExtendedPartNr < 0 )
		{
			FF_PRINTF( "No more extended partitions\n" );
			break;		/* nothing left to do */
		}

		/* Examine the ulNext extended partition */
		ulThisSector = ulFirstSector + pxPartitions[ xExtendedPartNr ].ulStartLBA * ulSectorSize;
		ulThisSize = pxPartitions[ xExtendedPartNr ].ulSectorCount * ulSectorSize;
	}

	if( pxBuffer != NULL )
	{
		FF_ERROR xTempError = FF_ReleaseBuffer( pxIOManager, pxBuffer );
		if( FF_isERR( xError ) == pdFALSE )
		{
			xError = xTempError;
		}
	}

	pxIOManager->pPartition->TotalSectors = prevTotalSectors;

	return xError;
}	/* FF_ParseExtended() */
/*-----------------------------------------------------------*/

/* FF_PartitionCount() has now been replaced by FF_PartitionSearch()
 * It will enumerate all valid partitions found
 * If sector-0 happens to be a valid MBR, 1 partition will be returned
 */
FF_ERROR FF_PartitionSearch( FF_IOMAN *pxIOManager, FF_SPartFound_t *pPartsFound )
/**
 *	@public
 *	@brief	Searches a disk for all primary and extended/logical partitions
 *	@brief	Previously called FF_PartitionCount
 *
 *	@param	pxIOManager		FF_IOManager_t object.
 *	@param	pPartsFound		Contains an array of ffconfigMAX_PARTITIONS partitions
 *
 *	@Return	>=0 Number of partitions found
 *	@Return	<0 error
 **/
//FF_Error_t FF_PartitionSearch( FF_IOManager_t *pxIOManager, FF_SPartFound_t *pPartsFound )
{
BaseType_t xPartNr;
FF_BUFFER *pxBuffer;
uint8_t *ucDataBuffer;
BaseType_t isPBR = pdFALSE;
FF_ERROR xError = FF_ERR_NONE;
uint32_t prevTotalSectors = pxIOManager->pPartition->TotalSectors;
FF_Part_t pxPartitions[ 4 ];

	memset( pPartsFound, '\0', sizeof( *pPartsFound ) );

	do
	{
		pxBuffer = FF_GetBuffer( pxIOManager, 0, FF_MODE_READ );
		if( pxBuffer == NULL )
		{
			xError = FF_ERR_DEVICE_DRIVER_FAILED | FF_PARTITIONSEARCH;
			break;
		}

		/* Disable sector checking in FF_BlockRead
		Let user driver return an error is appropriate. */
		pxIOManager->pPartition->TotalSectors = 0;
		ucDataBuffer = pxBuffer->pBuffer;

		/* Check MBR (Master Boot Record) or
		PBR (Partition Boot Record) signature. */
		if( ( FF_getChar( ucDataBuffer, FF_FAT_MBR_SIGNATURE ) != 0x55 ) &&
			( FF_getChar( ucDataBuffer, FF_FAT_MBR_SIGNATURE ) != 0xAA ) )
		{
			/* No MBR, but is it a PBR ?
			Partition Boot Record */
			if( ( FF_getChar( ucDataBuffer, 0 ) == 0xEB ) && /* PBR Byte 0 */
				( FF_getChar( ucDataBuffer, 2 ) == 0x90 ) )
			{
				/* PBR Byte 2
				No MBR but PBR exist then there is only one partition
				Handle this later. */
				isPBR = pdTRUE;
			}
			else
			{
				FF_PRINTF( "FF_PartitionSearch: [%02X,%02X] No signature (%02X %02X), no PBR neither\n",
					FF_getChar( ucDataBuffer, 0 ),
					FF_getChar( ucDataBuffer, 2 ),
					FF_getChar( ucDataBuffer, FF_FAT_MBR_SIGNATURE ),
					FF_getChar( ucDataBuffer, FF_FAT_MBR_SIGNATURE + 1 ) );

				/* No MBR and no PBR then no partition found. */
				xError = FF_ERR_IOMAN_INVALID_FORMAT | FF_PARTITIONSEARCH;
				break;
			}
		}
		/* Copy the 4 partition records into 'pxPartitions': */
		FF_ReadParts( ucDataBuffer, pxPartitions );

		for( xPartNr = 0; ( xPartNr < 4 ) && ( isPBR == pdFALSE ); xPartNr++ )
		{
			/*		FF_PRINTF ("FF_Part[%d]: id %02X act %02X Start %6lu Len %6lu (sectors)\n", */
			/*			xPartNr, pxPartitions[ xPartNr ].ucPartitionID, */
			/*			pxPartitions[ xPartNr ].ucActive, */
			/*			pxPartitions[ xPartNr ].ulStartLBA, */
			/*			pxPartitions[ xPartNr ].ulSectorCount); */
			if( prvIsExtendedPartition( pxPartitions[ xPartNr ].ucPartitionID ) != pdFALSE )
			{
				continue;		/* Do this later */
			}

			/* The first sector must be a MBR, then check the partition entry in the MBR */
			if( ( pxPartitions[ xPartNr ].ucActive != 0x80 ) &&
				( pxPartitions[ xPartNr ].ucActive != 0x00 ) )
			{
				if( ( xPartNr == 0 ) &&
					( FF_getShort( ucDataBuffer, FF_FAT_RESERVED_SECTORS ) != 0 ) &&
					( FF_getChar( ucDataBuffer, FF_FAT_NUMBER_OF_FATS ) != 0 ) )
				{
					isPBR = pdTRUE;
				}
				else
				{
					xError = FF_ERR_IOMAN_INVALID_FORMAT | FF_PARTITIONSEARCH;
					break;
				}
			}
			else if( pxPartitions[ xPartNr ].ulSectorCount )
			{
				FF_Part_t	*p = &pPartsFound->pxPartitions[ pPartsFound->iCount++ ];
				*p = pxPartitions[ xPartNr ];
				p->bIsExtended = 0;
				if( pPartsFound->iCount >= ffconfigMAX_PARTITIONS )
				{
					break;
				}
			}
		}
		if( FF_isERR( xError ) || ( pPartsFound->iCount >= ffconfigMAX_PARTITIONS ) )
		{
			break;
		}
		for( xPartNr = 0; xPartNr < 4; xPartNr++ )
		{
			if( prvIsExtendedPartition( pxPartitions[ xPartNr ].ucPartitionID ) )
			{
				xError = FF_ParseExtended( pxIOManager, pxPartitions[ xPartNr ].ulStartLBA,
					pxPartitions[ xPartNr ].ulSectorCount, pPartsFound );

				if( FF_isERR( xError ) || ( pPartsFound->iCount >= ffconfigMAX_PARTITIONS ) )
				{
					goto done;
				}
			}
		}

		if( pPartsFound->iCount == 0 )
		{
			FF_PRINTF( "FF_Part: no partitions, try as PBR\n" );
			isPBR = pdTRUE;
		}

		if( isPBR )
		{
			uint8_t media = FF_getChar( ucDataBuffer, FF_FAT_MEDIA_TYPE );
			FF_Part_t	*p;
			if( !prvIsValidMedia( media ) )
			{
				FF_PRINTF( "FF_Part: Looks like PBR but media %02X\n", media );
				xError = FF_ERR_IOMAN_NO_MOUNTABLE_PARTITION | FF_PARTITIONSEARCH;
				goto done;
			}

			/* This looks like a PBR because it has a valid media type */
			p = pPartsFound->pxPartitions;
			p->ulStartLBA = 0;	/* FF_FAT_PTBL_LBA */
			p->ulSectorCount = ( uint32_t ) FF_getShort( pxBuffer->pBuffer, FF_FAT_16_TOTAL_SECTORS );
			if( p->ulSectorCount == 0ul )
			{
				p->ulSectorCount = FF_getLong( pxBuffer->pBuffer, FF_FAT_32_TOTAL_SECTORS );
			}

			p->ucActive = 0x80;	/* FF_FAT_PTBL_ACTIVE */
			p->ucPartitionID = 0x0B;	/* FF_FAT_PTBL_ID MSDOS data partition */
			p->bIsExtended = 0;
			pPartsFound->iCount = 1;
		}
	} while( pdFALSE );
done:
	if( pxBuffer )
	{
		FF_ERROR xTempError = FF_ReleaseBuffer( pxIOManager, pxBuffer );
		if( FF_isERR( xError ) == pdFALSE )
		{
			xError = xTempError;
		}
	}

	pxIOManager->pPartition->TotalSectors = prevTotalSectors;

	return FF_isERR( xError ) ? xError : pPartsFound->iCount;
}	/* FF_PartitionSearch() */
/*-----------------------------------------------------------*/


/**
 *	@file		ff_format.c
 *	@ingroup	FORMAT
 *
 *	@defgroup	FAT Fat File-System
 *	@brief		Format a drive, given the number of sectors.
 *
 **/

/*=========================================================================================== */

#define	OFS_PART_ACTIVE_8             0x000 /* 0x01BE 0x80 if active */
#define	OFS_PART_START_HEAD_8         0x001 /* 0x01BF */
#define	OFS_PART_START_SEC_TRACK_16   0x002 /* 0x01C0 */
#define	OFS_PART_ID_NUMBER_8          0x004 /* 0x01C2 */
#define	OFS_PART_ENDING_HEAD_8        0x005 /* 0x01C3 */
#define	OFS_PART_ENDING_SEC_TRACK_16  0x006 /* 0x01C4   = SectorCount - 1 - ulHiddenSectors */
#define	OFS_PART_STARTING_LBA_32      0x008 /* 0x01C6   = ulHiddenSectors (This is important) */
#define	OFS_PART_LENGTH_32            0x00C /* 0x01CA   = SectorCount - 1 - ulHiddenSectors */

#define	OFS_PTABLE_MACH_CODE          0x000 /* 0x0000 */
#define	OFS_PTABLE_PART_0             0x1BE /* 446 */
#define	OFS_PTABLE_PART_1             0x1CE /* 462 */
#define	OFS_PTABLE_PART_2             0x1DE /* 478 */
#define	OFS_PTABLE_PART_3             0x1FE /* 494 */
#define	OFS_PTABLE_PART_LEN           16

/*=========================================================================================== */

#define	OFS_BPB_jmpBoot_24           0x000 /* uchar jmpBoot[3] "0xEB 0x00 0x90" */
#define	OFS_BPB_OEMName_64           0x003 /* uchar BS_OEMName[8] "MSWIN4.1" */

#define	OFS_BPB_BytsPerSec_16        0x00B /* Only 512, 1024, 2048 or 4096 */
#define	OFS_BPB_SecPerClus_8         0x00D /* Only 1, 2, 4, 8, 16, 32, 64, 128 */
#define	OFS_BPB_ResvdSecCnt_16       0x00E /* ulFATReservedSectors, e.g. 1 (FAT12/16) or 32 (FAT32) */

#define	OFS_BPB_NumFATs_8            0x010 /* 2 recommended */
#define	OFS_BPB_RootEntCnt_16        0x011 /* ((iFAT16RootSectors * 512) / 32) 512 (FAT12/16) or 0 (FAT32) */
#define	OFS_BPB_TotSec16_16          0x013 /* xxx (FAT12/16) or 0 (FAT32) */
#define	OFS_BPB_Media_8              0x015 /* 0xF0 (rem media) also in FAT[0] low byte */

#define	OFS_BPB_FATSz16_16           0x016
#define	OFS_BPB_SecPerTrk_16         0x018 /* n.a. CF has no tracks */
#define	OFS_BPB_NumHeads_16          0x01A /* n.a. 1 ? */
#define	OFS_BPB_HiddSec_32           0x01C /* n.a.	0 for nonparitioned volume */
#define	OFS_BPB_TotSec32_32          0x020 /* >= 0x10000 */

#define	OFS_BPB_16_DrvNum_8          0x024 /* n.a. */
#define	OFS_BPB_16_Reserved1_8       0x025 /* n.a. */
#define	OFS_BPB_16_BootSig_8         0x026 /* n.a. */
#define	OFS_BPB_16_BS_VolID_32       0x027 /* "unique" number */
#define	OFS_BPB_16_BS_VolLab_88      0x02B /* "NO NAME    " */
#define	OFS_BPB_16_FilSysType_64     0x036 /* "FAT12   " */

#define	OFS_BPB_32_FATSz32_32        0x024 /* Only when BPB_FATSz16 = 0 */
#define	OFS_BPB_32_ExtFlags_16       0x028 /* FAT32 only */
#define	OFS_BPB_32_FSVer_16          0x02A /* 0:0 */
#define	OFS_BPB_32_RootClus_32       0x02C /* See 'iFAT32RootClusters' Normally 2 */
#define	OFS_BPB_32_FSInfo_16         0x030 /* Normally 1 */
#define	OFS_BPB_32_BkBootSec_16      0x032 /* Normally 6 */
#define	OFS_BPB_32_Reserved_96       0x034 /* Zeros */
#define	OFS_BPB_32_DrvNum_8          0x040 /* n.a. */
#define	OFS_BPB_32_Reserved1_8       0x041 /* n.a. */
#define	OFS_BPB_32_BootSig_8         0x042 /* n.a. */
#define	OFS_BPB_32_VolID_32          0x043 /* "unique" number */
#define	OFS_BPB_32_VolLab_88         0x047 /* "NO NAME    " */
#define	OFS_BPB_32_FilSysType_64     0x052 /* "FAT12   " */

#define	OFS_FSI_32_LeadSig			0x000 /* With contents 0x41615252 */
#define	OFS_FSI_32_Reserved1		0x004 /* 480 times 0 */
#define	OFS_FSI_32_StrucSig			0x1E4 /* With contents 0x61417272 */
#define	OFS_FSI_32_Free_Count		0x1E8 /* last known free cluster count on the volume, ~0 for unknown */
#define	OFS_FSI_32_Nxt_Free			0x1EC /* cluster number at which the driver should start looking for free clusters */
#define	OFS_FSI_32_Reserved2		0x1F0 /* zero's */
#define	OFS_FSI_32_TrailSig			0x1FC /* 0xAA550000 (little endian) */

#define RESV_COUNT					32

#ifdef ffconfigMIN_CLUSTERS_FAT32
	#define MIN_CLUSTER_COUNT_FAT32		ffconfigMIN_CLUSTERS_FAT32
#else
	#define MIN_CLUSTER_COUNT_FAT32		( 65525 )
#endif

#ifdef ffconfigMIN_CLUSTERS_FAT16
	#define MIN_CLUSTERS_FAT16			ffconfigMIN_CLUSTERS_FAT16
#else
	#define MIN_CLUSTERS_FAT16			( 4085 + 1 )
#endif

static inline uint32_t ulMin32( uint32_t a, uint32_t b )
{
uint32_t ulReturn;

	if( a <= b )
	{
		ulReturn = a;
	}
	else
	{
		ulReturn = b;
	}
	return ulReturn;
}

/*_RB_ Candidate for splitting into multiple functions? */
FF_ERROR FF_Format( FF_IOMAN *pIoman, int xPartitionNumber, int xPreferFAT16, int xSmallClusters )
{
uint32_t ulHiddenSectors;              /* Space from MBR and partition table */
const uint32_t ulFSInfo = 1;           /* Sector number of FSINFO structure within the reserved area */
const uint32_t ulBackupBootSector = 6; /* Sector number of "copy of the boot record" within the reserved area */
const BaseType_t xFATCount = 2;        /* Number of FAT's */
uint32_t ulFATReservedSectors = 0;     /* Space between the partition table and FAT table. */
int32_t iFAT16RootSectors = 0;         /* Number of sectors reserved for root directory (FAT16 only) */
int32_t iFAT32RootClusters = 0;        /* Initial amount of clusters claimed for root directory (FAT32 only) */
uint8_t ucFATType = 0;                 /* Either 'FF_T_FAT16' or 'FF_T_FAT32' */
uint32_t ulVolumeID = 0;               /* A pseudo Volume ID */

uint32_t ulSectorsPerFAT = 0;          /* Number of sectors used by a single FAT table */
uint32_t ulClustersPerFATSector = 0;   /* # of clusters which can be described within a sector (256 or 128) */
uint32_t ulSectorsPerCluster = 0;      /* Size of a cluster (# of sectors) */
uint32_t ulUsableDataSectors = 0;      /* Usable data sectors (= SectorCount - (ulHiddenSectors + ulFATReservedSectors)) */
uint32_t ulUsableDataClusters = 0;     /* equals "ulUsableDataSectors / ulSectorsPerCluster" */
uint32_t ulNonDataSectors = 0;         /* ulFATReservedSectors + ulHiddenSectors + iFAT16RootSectors */
uint32_t ulClusterBeginLBA = 0;        /* Sector address of the first data cluster */
uint32_t ulSectorCount;
FF_SPartFound_t xPartitionsFound;
FF_Part_t *pxMyPartition = 0;
FF_IOMAN *pxIOManager = pIoman;

	FF_PartitionSearch( pxIOManager, &xPartitionsFound );
	if( xPartitionNumber >= xPartitionsFound.iCount )
	{
		return FF_ERR_IOMAN_INVALID_PARTITION_NUM | FF_MODULE_FORMAT;
	}

	pxMyPartition = xPartitionsFound.pxPartitions + xPartitionNumber;
	ulSectorCount = pxMyPartition->ulSectorCount;

	ulHiddenSectors = pxMyPartition->ulStartLBA;

	if( ( ( xPreferFAT16 == pdFALSE ) && ( ( ulSectorCount - RESV_COUNT ) >= 65536 ) ) ||
		( ( ulSectorCount - RESV_COUNT ) >= ( 64 * MIN_CLUSTER_COUNT_FAT32 ) ) )
	{
		ucFATType = FF_T_FAT32;
		iFAT32RootClusters = 2;
		ulFATReservedSectors = RESV_COUNT;
		iFAT16RootSectors = 0;
	}
	else
	{
		ucFATType = FF_T_FAT16;
		iFAT32RootClusters = 0;
		ulFATReservedSectors = 1u;
		iFAT16RootSectors = 32; /* to get 512 dir entries */
	}

	/* Set start sector and length to allow FF_BlockRead/Write */
	pxIOManager->pPartition->TotalSectors = pxMyPartition->ulSectorCount;
	pxIOManager->pPartition->BeginLBA = pxMyPartition->ulStartLBA;

	/* TODO: Find some solution here to get a unique disk ID */
	ulVolumeID = ( rand() << 16 ) | rand(); /*_RB_ rand() has proven problematic in some environments. */

	/* Sectors within partition which can not be used */
	ulNonDataSectors = ulFATReservedSectors + iFAT16RootSectors;

	/* A fs dependent constant: */
	if( ucFATType == FF_T_FAT32 )
	{
		/* In FAT32, 4 bytes are needed to store the address (LBA) of a cluster.
		Each FAT sector of 512 bytes can contain 512 / 4 = 128 entries. */
		ulClustersPerFATSector = 128u;
	}
	else
	{
		/* In FAT16, 2 bytes are needed to store the address (LBA) of a cluster.
		Each FAT sector of 512 bytes can contain 512 / 2 = 256 entries. */
		ulClustersPerFATSector = 256u;
	}

	FF_PRINTF( "FF_Format: Secs %lu Rsvd %lu Hidden %lu Root %lu Data %lu\n",
		ulSectorCount, ulFATReservedSectors, ulHiddenSectors, iFAT16RootSectors, ulSectorCount - ulNonDataSectors );

	/* Either search from small to large or v.v. */
	if( xSmallClusters != 0 )
	{
		/* The caller prefers to have small clusters.
		Less waste but it can be slower. */
		ulSectorsPerCluster = 1u;
	}
	else
	{
		if( ucFATType == FF_T_FAT32 )
		{
			ulSectorsPerCluster = 64u;
		}
		else
		{
			ulSectorsPerCluster = 32u;
		}
	}

	for( ;; )
	{
		int32_t groupSize;
		/* Usable sectors */
		ulUsableDataSectors = ulSectorCount - ulNonDataSectors;
		/* Each group consists of 'xFATCount' sectors + 'ulClustersPerFATSector' clusters */
		groupSize = xFATCount + ulClustersPerFATSector * ulSectorsPerCluster;
		/* This amount of groups will fit: */
		ulSectorsPerFAT = ( ulUsableDataSectors + groupSize - ulSectorsPerCluster - xFATCount ) / groupSize;

		ulUsableDataClusters = ulMin32(
			( uint32_t ) ( ulUsableDataSectors - xFATCount * ulSectorsPerFAT ) / ulSectorsPerCluster,
			( uint32_t ) ( ulClustersPerFATSector * ulSectorsPerFAT ) );
		ulUsableDataSectors = ulUsableDataClusters * ulSectorsPerCluster;

		if( ( ucFATType == FF_T_FAT16 ) && ( ulUsableDataClusters >= MIN_CLUSTERS_FAT16 ) && ( ulUsableDataClusters < 65536 ) )
		{
			break;
		}

		if( ( ucFATType == FF_T_FAT32 ) && ( ulUsableDataClusters >= 65536 ) && ( ulUsableDataClusters < 0x0FFFFFEF ) )
		{
			break;
		}

		/* Was this the last test? */
		if( ( ( xSmallClusters != pdFALSE ) && ( ulSectorsPerCluster == 32 ) ) ||
			( ( xSmallClusters == pdFALSE ) && ( ulSectorsPerCluster == 1) ) )
		{
			FF_PRINTF( "FF_Format: Can not make a FAT%d (tried %d) with %lu sectors\n",
				ucFATType == FF_T_FAT32 ? 32 : 16, xPreferFAT16 ? 16 : 32, ulSectorCount );
			return FF_ERR_IOMAN_BAD_MEMSIZE | FF_MODULE_FORMAT;
		}
		/* No it wasn't, try next clustersize */
		if( xSmallClusters != pdFALSE )
		{
			ulSectorsPerCluster <<= 1;
		}
		else
		{
			ulSectorsPerCluster >>= 1;
		}
	}

	if( ( ucFATType == FF_T_FAT32 ) && ( ulSectorCount >= 0x100000UL ) )	/* Larger than 0.5 GB */
	{
		int32_t lRemaining = (ulNonDataSectors + 2 * ulSectorsPerFAT) % 128;
		if( lRemaining != 0 )
		{
			/* In order to get ClusterBeginLBA well aligned (on a 128 sector boundary) */
			ulFATReservedSectors += ( 128 - lRemaining );
			ulNonDataSectors = ulFATReservedSectors + iFAT16RootSectors;
			ulUsableDataSectors = ulSectorCount - ulNonDataSectors - 2 * ulSectorsPerFAT;
			ulUsableDataClusters = ulUsableDataSectors / ulSectorsPerCluster;
		}
	}
	ulClusterBeginLBA	= ulHiddenSectors + ulFATReservedSectors + 2 * ulSectorsPerFAT;

	const uint32_t maxNumSectors = 8;
	uint8_t pucSectorBuffer[512*maxNumSectors];

/*  ======================================================================================= */

	memset( pucSectorBuffer, '\0', 512 );

	memcpy( pucSectorBuffer + OFS_BPB_jmpBoot_24, "\xEB\x76\x90" "REPLAY", 9 );   /* Includes OFS_BPB_OEMName_64 */

	FF_putShort( pucSectorBuffer, OFS_BPB_BytsPerSec_16, 512 );   /* 0x00B / Only 512, 1024, 2048 or 4096 */
	FF_putShort( pucSectorBuffer, OFS_BPB_ResvdSecCnt_16, ( uint32_t ) ulFATReservedSectors ); /*  0x00E / 1 (FAT12/16) or 32 (FAT32) */

	FF_putChar( pucSectorBuffer, OFS_BPB_NumFATs_8, 2);          /* 0x010 / 2 recommended */
	FF_putShort( pucSectorBuffer, OFS_BPB_RootEntCnt_16, ( uint32_t ) ( iFAT16RootSectors * 512 ) / 32 ); /* 0x011 / 512 (FAT12/16) or 0 (FAT32) */

	/* For FAT12 and FAT16 volumes, this field contains the count of 32- */
	/* byte directory entries in the root directory */

	FF_putChar( pucSectorBuffer, OFS_BPB_Media_8, 0xF8 );         /* 0x015 / 0xF0 (rem media) also in FAT[0] low byte */

	FF_putShort( pucSectorBuffer, OFS_BPB_SecPerTrk_16, 0x3F );	   /* 0x18 n.a. CF has no tracks */
	FF_putShort( pucSectorBuffer, OFS_BPB_NumHeads_16, 255 );         /* 0x01A / n.a. 1 ? */
	FF_putLong (pucSectorBuffer, OFS_BPB_HiddSec_32, ( uint32_t ) ulHiddenSectors ); /* 0x01C / n.a.	0 for nonparitioned volume */
	{
		int32_t fatBeginLBA;
		int32_t dirBegin;

		FF_putChar( pucSectorBuffer, OFS_BPB_SecPerClus_8, ( uint32_t ) ulSectorsPerCluster ); /*  0x00D / Only 1, 2, 4, 8, 16, 32, 64, 128 */
		FF_PRINTF("FF_Format: SecCluster %lu DatSec %lu DataClus %lu ulClusterBeginLBA %lu\n",
			ulSectorsPerCluster, ulUsableDataSectors, ulUsableDataClusters, ulClusterBeginLBA );

		/* This field is the new 32-bit total count of sectors on the volume. */
		/* This count includes the count of all sectors in all four regions of the volume */
		FF_putShort( pucSectorBuffer, OFS_BPB_TotSec16_16, 0 );        /* 0x013 / xxx (FAT12/16) or 0 (FAT32) */

		FF_putLong (pucSectorBuffer, OFS_BPB_TotSec32_32, ulSectorCount ); /* 0x020 / >= 0x10000 */

		if( ucFATType == FF_T_FAT32 )
		{
			FF_putLong( pucSectorBuffer,  OFS_BPB_32_FATSz32_32, ulSectorsPerFAT );      /* 0x24 / Only when BPB_FATSz16 = 0 */
			FF_putShort( pucSectorBuffer, OFS_BPB_32_ExtFlags_16, 0 );                               /* 0x28 / FAT32 only */
			FF_putShort( pucSectorBuffer, OFS_BPB_32_FSVer_16, 0 );                                  /* 0x2A / 0:0 */
			FF_putLong( pucSectorBuffer,  OFS_BPB_32_RootClus_32, ( uint32_t ) iFAT32RootClusters ); /* 0x2C / Normally 2 */
			FF_putShort( pucSectorBuffer, OFS_BPB_32_FSInfo_16, ulFSInfo );                          /* 0x30 / Normally 1 */
			FF_putShort( pucSectorBuffer, OFS_BPB_32_BkBootSec_16, ulBackupBootSector );             /* 0x32 / Normally 6 */
			FF_putChar( pucSectorBuffer,  OFS_BPB_32_DrvNum_8, 0 );                                  /* 0x40 / n.a. */
			FF_putChar( pucSectorBuffer,  OFS_BPB_32_BootSig_8, 0x29 );                              /* 0x42 / n.a. */
			FF_putLong( pucSectorBuffer,  OFS_BPB_32_VolID_32, ( uint32_t ) ulVolumeID );            /* 0x43 / "unique" number */
			memcpy( pucSectorBuffer + OFS_BPB_32_VolLab_88, "REPLAY     ", 11 );    /* 0x47 / "NO NAME    " */
			memcpy( pucSectorBuffer + OFS_BPB_32_FilSysType_64, "FAT32   ", 8 );    /* 0x52 / "FAT12   " */
		}
		else
		{
			FF_putChar( pucSectorBuffer, OFS_BPB_16_DrvNum_8, 0u );         /* 0x024 / n.a. */
			FF_putChar( pucSectorBuffer, OFS_BPB_16_Reserved1_8, 0 );      /* 0x025 / n.a. */
			FF_putChar( pucSectorBuffer, OFS_BPB_16_BootSig_8, 0x29 );     /* 0x026 / n.a. */
			FF_putLong (pucSectorBuffer, OFS_BPB_16_BS_VolID_32, ( uint32_t ) ulVolumeID ); /* 0x027 / "unique" number */

			FF_putShort( pucSectorBuffer, OFS_BPB_FATSz16_16, ulSectorsPerFAT );		/* 0x16 */

			memcpy( pucSectorBuffer + OFS_BPB_16_BS_VolLab_88, "REPLAY     ", 11 );            /* 0x02B / "NO NAME    " */
			memcpy( pucSectorBuffer + OFS_BPB_16_FilSysType_64, "FAT16   ", 8 );           /* 0x036 / "FAT12   " */
		}

		pucSectorBuffer[510] = 0x55;
		pucSectorBuffer[511] = 0xAA;

		FF_BlockWrite( pxIOManager, ulHiddenSectors, 1, pucSectorBuffer, 0u );
		if (ucFATType == FF_T_FAT32)
		{
			FF_BlockWrite( pxIOManager, ulHiddenSectors + ulBackupBootSector, 1, pucSectorBuffer, pdFALSE );
		}

		if( ucFATType == FF_T_FAT32 )
		{
			memset( pucSectorBuffer, '\0', 512 );

			FF_putLong( pucSectorBuffer, OFS_FSI_32_LeadSig, 0x41615252 );  /* to validate that this is in fact an FSInfo sector. */
			/* OFS_FSI_32_Reserved1		0x004 / 480 times 0 */
			FF_putLong( pucSectorBuffer, OFS_FSI_32_StrucSig, 0x61417272 ); /* Another signature that is more localized in the */
																	 /* sector to the location of the fields that are used. */
			FF_putLong( pucSectorBuffer, OFS_FSI_32_Free_Count, ulUsableDataClusters );      /* last known free cluster count on the volume, ~0 for unknown */
			FF_putLong( pucSectorBuffer, OFS_FSI_32_Nxt_Free, 2 );          /* cluster number at which the driver should start looking for free clusters */
			/* OFS_FSI_32_Reserved2		0x1F0 / zero's */
			FF_putLong( pucSectorBuffer, OFS_FSI_32_TrailSig, 0xAA550000 ); /* Will correct for endianness */

			FF_BlockWrite( pxIOManager, ulHiddenSectors + ulFSInfo, 1, pucSectorBuffer, pdFALSE );
			FF_BlockWrite( pxIOManager, ulHiddenSectors + ulFSInfo + ulBackupBootSector, 1, pucSectorBuffer, pdFALSE );
		}

		fatBeginLBA = ulHiddenSectors + ulFATReservedSectors;
		memset( pucSectorBuffer, '\0', 512 );
		switch( ucFATType )
		{
			case FF_T_FAT16:
				FF_putShort( pucSectorBuffer, 0, 0xFFF8 ); /* First FAT entry. */
				FF_putShort( pucSectorBuffer, 2, 0xFFFF ); /* RESERVED alloc. */
				break;
			case FF_T_FAT32:
				FF_putLong( pucSectorBuffer, 0, 0x0FFFFFF8 ); /* FAT32 FAT sig. */
				FF_putLong( pucSectorBuffer, 4, 0xFFFFFFFF ); /* RESERVED alloc. */
				FF_putLong( pucSectorBuffer, 8, 0x0FFFFFFF ); /* Root dir allocation. */
				break;
			default:
				break;
		}

		FF_BlockWrite( pxIOManager, ( uint32_t ) fatBeginLBA, 1, pucSectorBuffer, pdFALSE );
		FF_BlockWrite( pxIOManager, ( uint32_t ) fatBeginLBA + ulSectorsPerFAT, 1, pucSectorBuffer, pdFALSE );

		FF_PRINTF( "FF_Format: Clearing entire FAT (2 x %lu sectors):\n", ulSectorsPerFAT );
		{
		int32_t addr;

			memset( pucSectorBuffer, '\0', 512*maxNumSectors );
			for( addr = fatBeginLBA+1;
				 addr < ( fatBeginLBA + ( int32_t ) ulSectorsPerFAT );
				 addr+=maxNumSectors )
			{
				FF_T_UINT32 ulNumSectors = ( fatBeginLBA + ( int32_t ) ulSectorsPerFAT ) - addr;
				if (ulNumSectors > maxNumSectors)
					ulNumSectors = maxNumSectors;
				FF_BlockWrite( pxIOManager, ( uint32_t ) addr, ulNumSectors, pucSectorBuffer, pdFALSE );
				FF_BlockWrite( pxIOManager, ( uint32_t ) addr + ulSectorsPerFAT, ulNumSectors, pucSectorBuffer, pdFALSE );
			}
		}
		FF_PRINTF( "FF_Format: Clearing done\n" );
		dirBegin = fatBeginLBA + ( 2 * ulSectorsPerFAT );
		if (dirBegin != ulClusterBeginLBA)
		{
			return FF_ERR_IOMAN_OUT_OF_BOUNDS_WRITE | FF_MODULE_FORMAT;
		}
#if( ffconfigTIME_SUPPORT != 0 )
		{
			FF_SystemTime_t	str_t;
			int16_t myShort;

			FF_GetSystemTime( &str_t );

			myShort = ( ( str_t.Hour << 11 ) & 0xF800 ) |
								( ( str_t.Minute  <<  5 ) & 0x07E0 ) |
								( ( str_t.Second   /  2 ) & 0x001F );
			FF_putShort( pucSectorBuffer, 22, ( uint32_t ) myShort );

			myShort = ( ( ( str_t.Year- 1980 )  <<  9 ) & 0xFE00 ) |
					   ( ( str_t.Month << 5 ) & 0x01E0 ) |
					   ( str_t.Day & 0x001F );
			FF_putShort( pucSectorBuffer, 24, ( uint32_t ) myShort);
		}
#endif	/* ffconfigTIME_SUPPORT */

		memcpy (pucSectorBuffer, "REPLAY     ", 11);
		pucSectorBuffer[11] = FF_FAT_ATTR_VOLID;

		{
		int32_t lAddress;
		int32_t lLastAddress;

			if( iFAT16RootSectors != 0 )
			{
				lLastAddress = dirBegin + iFAT16RootSectors;
			}
			else
			{
				lLastAddress = dirBegin + ulSectorsPerCluster;
			}

			FF_PRINTF("FF_Format: Clearing root directory at %08lX: %lu sectors\n", dirBegin, lLastAddress - dirBegin );
			for( lAddress = dirBegin; lAddress < lLastAddress; lAddress++ )
			{
				FF_BlockWrite( pxIOManager, ( uint32_t ) lAddress, 1, pucSectorBuffer, 0u );
				if( lAddress == dirBegin )
				{
					memset( pucSectorBuffer, '\0', 512 );
				}
			}
		}
	}

	return FF_ERR_NONE;
}

FF_ERROR FF_Partition( FF_IOMAN *pIoman, FF_PartitionParameters_t *pParams )
{
	const uint32_t ulInterSpace = pParams->ulInterSpace ? pParams->ulInterSpace : 2048;  /* Hidden space between 2 extended partitions */
	BaseType_t xPartitionNumber;
	FF_Part_t pxPartitions[ ffconfigMAX_PARTITIONS ];
	uint32_t ulPartitionOffset; /* Pointer within partition table */
	FF_BUFFER *pxSectorBuffer;
	uint8_t *pucBuffer;
	uint32_t ulSummedSizes = 0;	/* Summed sizes as a percentage or as number of sectors. */
	BaseType_t xPartitionCount = 0;
	BaseType_t xNeedExtended;
	uint32_t ulReservedSpace;
	uint32_t ulAvailable;
	FF_IOMAN *pxIOManager = pIoman;


	/* Clear caching without flushing first. */
	FF_IOMAN_InitBufferDescriptors( pxIOManager );

	/* Avoid sanity checks by FF_BlockRead/Write. */
	pxIOManager->pPartition->TotalSectors = 0;

	/* Get the sum of sizes and number of actual partitions. */
	for( xPartitionNumber = 0; xPartitionNumber < ffconfigMAX_PARTITIONS; xPartitionNumber++ )
	{
		if( pParams->xSizes[ xPartitionNumber ] > 0 )
		{
			xPartitionCount++;
			ulSummedSizes += pParams->xSizes[ xPartitionNumber ];
		}
	}

	if( xPartitionCount == 0 )
	{
		xPartitionCount = 1;
		if( pParams->eSizeType == eSizeIsSectors)
		{
			pParams->xSizes[ 0 ] = pParams->ulSectorCount;
		}
		else
		{
			pParams->xSizes[ 0 ] = 100;
		}

		ulSummedSizes = pParams->xSizes[ 0 ];
	}

	/* Correct PrimaryCount if necessary. */
	if( pParams->xPrimaryCount > ( ( xPartitionCount > 4 ) ? 3 : xPartitionCount ) )
	{
		pParams->xPrimaryCount = ( xPartitionCount > 4 ) ? 3 : xPartitionCount;
	}

	/* Now see if extended is necessary. */
	xNeedExtended = ( xPartitionCount > pParams->xPrimaryCount );

	if( xNeedExtended != pdFALSE )
	{
		if( pParams->ulHiddenSectors < 4096 )
		{
			pParams->ulHiddenSectors = 4096;
		}

		ulReservedSpace = ulInterSpace * ( xPartitionCount - pParams->xPrimaryCount );
	}
	else
	{
		ulReservedSpace = 0;
	}

	ulAvailable = pParams->ulSectorCount - pParams->ulHiddenSectors - ulReservedSpace;

	/* Check validity of Sizes */
	switch( pParams->eSizeType )
	{
		case eSizeIsQuota:       /* Assign a quotum (sum of Sizes is free, all disk space will be allocated) */
			break;
		case eSizeIsPercent:  /* Assign a percentage of the available space (sum of Sizes must be <= 100) */
			if( ulSummedSizes > 100 )
			{
				return FF_FORMATPARTITION | FF_ERR_IOMAN_BAD_MEMSIZE;
			}
			ulSummedSizes = 100;
			break;
		case eSizeIsSectors:     /* Assign fixed number of sectors (512 byte each) */
			if( ulSummedSizes > ulAvailable )
			{
				return FF_FORMATPARTITION | FF_ERR_IOMAN_BAD_MEMSIZE;
			}
			break;
	}

	{
	uint32_t ulRemaining = ulAvailable;
	uint32_t ulLBA = pParams->ulHiddenSectors;

		/* Divide the available sectors among the partitions: */
		memset( pxPartitions, '\0', sizeof( pxPartitions ) );

		for( xPartitionNumber = 0; xPartitionNumber < xPartitionCount; xPartitionNumber++ )
		{
			if( pParams->xSizes[ xPartitionNumber ] > 0 )
			{
				uint32_t ulSize;
				switch( pParams->eSizeType )
				{
					case eSizeIsQuota:       /* Assign a quotum (sum of Sizes is free, all disk space will be allocated) */
					case eSizeIsPercent:  /* Assign a percentage of the available space (sum of Sizes must be <= 100) */
						ulSize = ( uint32_t ) ( ( ( uint64_t ) pParams->xSizes[ xPartitionNumber ] * ulAvailable) / ulSummedSizes );
						break;
					case eSizeIsSectors:     /* Assign fixed number of sectors (512 byte each) */
					default:                  /* Just for the compiler(s) */
						ulSize = pParams->xSizes[ xPartitionNumber ];
						break;
				}

				if( ulSize > ulRemaining )
				{
					ulSize = ulRemaining;
				}

				ulRemaining -= ulSize;
				pxPartitions[ xPartitionNumber ].ulSectorCount = ulSize;
				pxPartitions[ xPartitionNumber ].ucActive = 0x80;
				pxPartitions[ xPartitionNumber ].ulStartLBA = ulLBA; /* ulStartLBA might still change for logical partitions */
				pxPartitions[ xPartitionNumber ].ucPartitionID = 0x0B;
				ulLBA += ulSize;
			}
		}
	}

/*  ======================================================================================= */

	{
		const uint32_t maxNumSectors = 8;
		uint8_t pucSectorBuffer[512*maxNumSectors];
		if( pucSectorBuffer == NULL )
		{
			return FF_ERR_NOT_ENOUGH_MEMORY | FF_MODULE_FORMAT;
		}
		FF_PRINTF( "FF_Format: Clearing hidden sectors (%lu sectors):\n", pParams->ulHiddenSectors );
		{
		int32_t addr;

			memset( pucSectorBuffer, '\0', 512*maxNumSectors );
			for( addr = 0;
				 addr < pParams->ulHiddenSectors;
				 addr+=maxNumSectors )
			{
				FF_T_UINT32 ulNumSectors = pParams->ulHiddenSectors - addr;
				if (ulNumSectors > maxNumSectors)
					ulNumSectors = maxNumSectors;
				FF_BlockWrite( pxIOManager, ( uint32_t ) addr, ulNumSectors, pucSectorBuffer, pdFALSE );
			}
		}
		FF_PRINTF( "FF_Format: Clearing done\n" );
	}

/*  ======================================================================================= */

	if( xNeedExtended != pdFALSE )
	{
		/* Create at least 1 extended/logical partition */
		int index;
		/* Start of the big extended partition */
		unsigned extendedLBA = pParams->ulHiddenSectors;
		/* Where to write the table */
		uint32_t ulLBA = 0;
		/* Contents of the table */
		FF_Part_t writeParts[4];

		for( index = -1; index < xPartitionCount; index++ )
		{
		uint32_t ulNextLBA;

			memset (writeParts, '\0', sizeof( writeParts ) );
			if( index < 0 )
			{
			/* we're at secor 0: */
			/* write primary partitions, if any */
			/* create big extended partition */
			uint32_t ulStartLBA = pParams->ulHiddenSectors;
				for( xPartitionNumber = 0; xPartitionNumber < pParams->xPrimaryCount; xPartitionNumber++ )
				{
					writeParts[ xPartitionNumber ].ulStartLBA = ulStartLBA;
					writeParts[ xPartitionNumber ].ulSectorCount = pxPartitions[ xPartitionNumber ].ulSectorCount;
					writeParts[ xPartitionNumber ].ucActive = 0x80;
					writeParts[ xPartitionNumber ].ucPartitionID = 0x0B;
					ulStartLBA += writeParts[ xPartitionNumber ].ulSectorCount;
					index++;
				}
				extendedLBA = ulStartLBA;
				writeParts[ xPartitionNumber ].ulStartLBA = ulStartLBA;
				writeParts[ xPartitionNumber ].ulSectorCount = pParams->ulSectorCount - ulStartLBA;
				writeParts[ xPartitionNumber ].ucActive = 0x80;
				writeParts[ xPartitionNumber ].ucPartitionID = 0x05;
				ulNextLBA = ulStartLBA;
			}
			else
			{
				/* Create a logical partition with "ulSectorCount" sectors: */
				writeParts[ 0 ].ulStartLBA = ulInterSpace;
				writeParts[ 0 ].ulSectorCount = pxPartitions[index].ulSectorCount;
				writeParts[ 0 ].ucActive = 0x80;
				writeParts[ 0 ].ucPartitionID = 0x0B;
				if( index < xPartitionCount - 1 )
				{
					/* Next extended partition */
					writeParts[ 1 ].ulStartLBA = ulInterSpace + ulLBA - extendedLBA + writeParts[ 0 ].ulSectorCount;
					writeParts[ 1 ].ulSectorCount = pxPartitions[index+1].ulSectorCount + ulInterSpace;
					writeParts[ 1 ].ucActive = 0x80;
					writeParts[ 1 ].ucPartitionID = 0x05;
				}
				ulNextLBA = writeParts[ 1 ].ulStartLBA + extendedLBA;
			}
			pxSectorBuffer = FF_GetBuffer(pxIOManager, ( uint32_t ) ulLBA, ( uint8_t ) FF_MODE_WRITE );
			{
				if( pxSectorBuffer == NULL )
				{
					return FF_ERR_DEVICE_DRIVER_FAILED;
				}
			}
			pucBuffer = pxSectorBuffer->pBuffer;
			memset ( pucBuffer, 0, 512 );
			memcpy ( pucBuffer + OFS_BPB_jmpBoot_24, "\xEB\x00\x90" "REPLAY", 9 );   /* Includes OFS_BPB_OEMName_64 */

			ulPartitionOffset = OFS_PTABLE_PART_0;
			for( xPartitionNumber = 0; xPartitionNumber < ffconfigMAX_PARTITIONS; xPartitionNumber++, ulPartitionOffset += 16 )
			{
				FF_putChar( pucBuffer, ulPartitionOffset + OFS_PART_ACTIVE_8,            writeParts[ xPartitionNumber ].ucActive );		/* 0x01BE 0x80 if active */
				FF_putChar( pucBuffer, ulPartitionOffset + OFS_PART_START_HEAD_8,        1 );											/* 0x001 / 0x01BF */
				FF_putShort(pucBuffer, ulPartitionOffset + OFS_PART_START_SEC_TRACK_16,  1 );											/* 0x002 / 0x01C0 */
				FF_putChar( pucBuffer, ulPartitionOffset + OFS_PART_ID_NUMBER_8,         writeParts[ xPartitionNumber ].ucPartitionID );/* 0x004 / 0x01C2 */
				FF_putChar( pucBuffer, ulPartitionOffset + OFS_PART_ENDING_HEAD_8,       0xFE );										/* 0x005 / 0x01C3 */
				FF_putShort(pucBuffer, ulPartitionOffset + OFS_PART_ENDING_SEC_TRACK_16, writeParts[ xPartitionNumber ].ulSectorCount );/* 0x006 / 0x01C4 */
				FF_putLong (pucBuffer, ulPartitionOffset + OFS_PART_STARTING_LBA_32,     writeParts[ xPartitionNumber ].ulStartLBA );	/* 0x008 / 0x01C6 This is important */
				FF_putLong (pucBuffer, ulPartitionOffset + OFS_PART_LENGTH_32,           writeParts[ xPartitionNumber ].ulSectorCount );/* 0x00C / 0x01CA Equal to total sectors */
			}
			pucBuffer[510] = 0x55;
			pucBuffer[511] = 0xAA;

			FF_ReleaseBuffer(pxIOManager, pxSectorBuffer );
			FF_FlushCache( pxIOManager );
			ulLBA = ulNextLBA;
		}
	}
	else
	{
		pxSectorBuffer = FF_GetBuffer( pxIOManager, 0, ( uint8_t ) FF_MODE_WRITE );
		{
			if( pxSectorBuffer == NULL )
			{
				return FF_ERR_DEVICE_DRIVER_FAILED;
			}
		}

		pucBuffer = pxSectorBuffer->pBuffer;
		memset (pucBuffer, 0, 512 );
		memcpy (pucBuffer + OFS_BPB_jmpBoot_24, "\xEB\x00\x90" "REPLAY", 9 );   /* Includes OFS_BPB_OEMName_64 */
		ulPartitionOffset = OFS_PTABLE_PART_0;

		for( xPartitionNumber = 0; xPartitionNumber < ffconfigMAX_PARTITIONS; xPartitionNumber++ )
		{
			FF_putChar(  pucBuffer, ulPartitionOffset + OFS_PART_ACTIVE_8,            pxPartitions[ xPartitionNumber ].ucActive );		/* 0x01BE 0x80 if active */
			FF_putChar(  pucBuffer, ulPartitionOffset + OFS_PART_START_HEAD_8,        1 );         										/* 0x001 / 0x01BF */
			FF_putShort( pucBuffer, ulPartitionOffset + OFS_PART_START_SEC_TRACK_16,  1 );  											/* 0x002 / 0x01C0 */
			FF_putChar(  pucBuffer, ulPartitionOffset + OFS_PART_ID_NUMBER_8,         pxPartitions[ xPartitionNumber ].ucPartitionID );	/* 0x004 / 0x01C2 */
			FF_putChar(  pucBuffer, ulPartitionOffset + OFS_PART_ENDING_HEAD_8,       0xFE );     										/* 0x005 / 0x01C3 */
			FF_putShort( pucBuffer, ulPartitionOffset + OFS_PART_ENDING_SEC_TRACK_16, pxPartitions[ xPartitionNumber ].ulSectorCount );	/* 0x006 / 0x01C4 */
			FF_putLong(  pucBuffer, ulPartitionOffset + OFS_PART_STARTING_LBA_32,     pxPartitions[ xPartitionNumber ].ulStartLBA );	/* 0x008 / 0x01C6 This is important */
			FF_putLong(  pucBuffer, ulPartitionOffset + OFS_PART_LENGTH_32,           pxPartitions[ xPartitionNumber ].ulSectorCount );	/* 0x00C / 0x01CA Equal to total sectors */
			ulPartitionOffset += 16;
		}
		pucBuffer[ 510 ] = 0x55;
		pucBuffer[ 511 ] = 0xAA;

		FF_ReleaseBuffer( pxIOManager, pxSectorBuffer );
		FF_FlushCache( pxIOManager );
	}

	return FF_ERR_NONE;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
