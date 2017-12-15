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
 **/


#ifndef _FF_FORMAT_H_
#define _FF_FORMAT_H_

#ifdef	__cplusplus
extern "C" {
#endif


#include "ff_config.h"
#include "ff_types.h"
#include "ff_ioman.h"
#include "ff_dir.h"
#include "ff_format.h"

typedef struct _FF_PARTITION_ENTRY {
	FF_T_UINT32 ulStartLBA;
	FF_T_UINT32 ulLength;
	FF_T_UINT8  ucStatus;
	FF_T_UINT8  ucType;
} FF_PARTITION_ENTRY;

typedef struct _FF_PARTITION_TABLE {
	FF_PARTITION_ENTRY arrPEntries[4];
} FF_PARTITION_TABLE;
//---------- PROTOTYPES
// PUBLIC (Interfaces):

//FF_ERROR FF_Format (FF_IOMAN *pIoman, FF_T_UINT32 SectorCount, FF_T_BOOL TryFat16);
//FF_ERROR FF_Format(FF_IOMAN *pIoman, FF_T_UINT32 ulStartLBA, FF_T_UINT32 ulEndLBA, FF_T_UINT32 ulClusterSize);
FF_ERROR FF_FormatPartition(FF_IOMAN *pIoman, FF_T_UINT32 ulPartitionNumber, FF_T_UINT32 ulClusterSize);

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

typedef enum _FF_SizeType {
	eSizeIsQuota,    /* Assign a quotum (sum of xSizes is free, all disk space will be allocated) */
	eSizeIsPercent,  /* Assign a percentage of the available space (sum of xSizes must be <= 100) */
	eSizeIsSectors,  /* Assign fixed number of sectors (sum of xSizes must be < ulSectorCount) */
} eSizeType_t;

typedef struct _FF_PartitionParameters {
	uint32_t ulSectorCount;     /* Total number of sectors on the disk, including hidden/reserved */
								/* Must be obtained from the block driver */
	uint32_t ulHiddenSectors;   /* Keep at least these initial sectors free  */
	uint32_t ulInterSpace;      /* Number of sectors to keep free between partitions (when 0 -> 2048) */
	int xSizes[ 4 ];  /* E.g. 80, 20, 0, 0 (see eSizeType) */
    int xPrimaryCount;    /* The number of partitions that must be "primary" */
	eSizeType_t eSizeType;
} FF_PartitionParameters_t;

FF_ERROR FF_Partition( FF_IOMAN *pIoman, FF_PartitionParameters_t *pParams );

FF_ERROR FF_Format( FF_IOMAN *pIoman, int xPartitionNumber, int xPreferFAT16, int xSmallClusters );

// Private :

#ifdef	__cplusplus
} // extern "C"
#endif

#endif
