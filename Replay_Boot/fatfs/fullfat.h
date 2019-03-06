#pragma once

#include <stdint.h>
#include <stdlib.h>

#include "filesys/ff_types.h"
#include "filesys/ff_error.h"

#include "ff.h"

#define FF_VERSION              "0.0.1 (" __DATE__ " : "__TIME__")"             // The official version number for this release.
#define FF_REVISION             "FullFAT-FatFS Wrapper   "                                                     // The official version control commit code for this release.

#define FF_T_FAT12				0x0A
#define FF_T_FAT16				0x0B
#define FF_T_FAT32				0x0C

#define FF_MODE_READ			0x01		///< Buffer / FILE Mode for Read Access.
#define	FF_MODE_WRITE			0x02		///< Buffer / FILE Mode for Write Access.
#define FF_MODE_APPEND			0x04		///< FILE Mode Append Access.
#define	FF_MODE_CREATE			0x08		///< FILE Mode Create file if not existing.
#define FF_MODE_TRUNCATE		0x10		///< FILE Mode Truncate an Existing file.
#define FF_MODE_DIR				0x80		///< Special Mode to open a Dir. (Internal use ONLY!)

#ifdef FF_USE_NATIVE_STDIO
#include <stdio.h>
#define FF_SEEK_SET     SEEK_SET
#define FF_SEEK_CUR     SEEK_CUR
#define FF_SEEK_END     SEEK_END
#else
#define FF_SEEK_SET     1
#define FF_SEEK_CUR     2
#define FF_SEEK_END     3
#endif


#define FF_FAT_ATTR_READONLY		0x01
#define FF_FAT_ATTR_HIDDEN			0x02
#define FF_FAT_ATTR_SYSTEM			0x04
#define FF_FAT_ATTR_VOLID			0x08
#define FF_FAT_ATTR_DIR				0x10
#define FF_FAT_ATTR_ARCHIVE			0x20
#define FF_FAT_ATTR_LFN				0x0F

typedef struct {
	 FF_T_UINT8			Type;
	 FF_T_UINT16		BlkSize;
	 FF_T_UINT8      	BlkFactor;
	 FF_T_UINT8			SectorsPerCluster;
	 FF_T_UINT32		NumClusters;
	 FF_T_UINT32		FreeClusterCount;
} FF_PARTITION;

typedef struct _FF_IOMAN
{
	FF_PARTITION	*pPartition;
} FF_IOMAN;
typedef struct _FF_FILE FF_FILE;
typedef struct _FF_DIRENT
{
	FF_T_UINT8	Attrib;
	FF_T_INT8	FileName[FF_MAX_FILENAME];
	DIR			dir;
} FF_DIRENT;

typedef FF_T_SINT32 (*FF_WRITE_BLOCKS)	(FF_T_UINT8 *pBuffer, FF_T_UINT32 SectorAddress, FF_T_UINT32 Count, void *pParam);
typedef FF_T_SINT32 (*FF_READ_BLOCKS)	(FF_T_UINT8 *pBuffer, FF_T_UINT32 SectorAddress, FF_T_UINT32 Count, void *pParam);

typedef enum _FF_SizeType {
	eSizeIsQuota,
	eSizeIsPercent,
	eSizeIsSectors,
} eSizeType_t;

typedef struct _FF_PartitionParameters
{
	uint32_t ulSectorCount;
	uint32_t ulHiddenSectors;
	uint32_t ulInterSpace;
	int xSizes[ 4 ];
    int xPrimaryCount;
	eSizeType_t eSizeType;
} FF_PartitionParameters_t;



const FF_T_INT8 *FF_GetErrDescription(FF_ERROR iErrorCode, char *apBuf, int aMaxlen);


FF_IOMAN	*FF_CreateIOMAN			(FF_T_UINT8 *pCacheMem, FF_T_UINT32 Size, FF_T_UINT16 BlkSize, FF_ERROR *pError);
FF_ERROR	FF_DestroyIOMAN			(FF_IOMAN *pIoman);
FF_ERROR	FF_RegisterBlkDevice	(FF_IOMAN *pIoman, FF_T_UINT16 BlkSize, FF_WRITE_BLOCKS fnWriteBlocks, FF_READ_BLOCKS fnReadBlocks, void *pParam);
FF_ERROR	FF_UnregisterBlkDevice	(FF_IOMAN *pIoman);
FF_ERROR	FF_MountPartition		(FF_IOMAN *pIoman, FF_T_UINT8 PartitionNumber);
FF_ERROR	FF_UnmountPartition		(FF_IOMAN *pIoman);
FF_T_UINT32 FF_GetVolumeSize(FF_IOMAN *pIoman);
FF_ERROR	FF_FlushCache			(FF_IOMAN *pIoman);
FF_T_BOOL	FF_Mounted		(FF_IOMAN *pIoman);
FF_ERROR	FF_IncreaseFreeClusters	(FF_IOMAN *pIoman, FF_T_UINT32 Count);
FF_T_SINT32 FF_GetPartitionBlockSize(FF_IOMAN *pIoman);
FF_T_UINT32 FF_GetFreeSize(FF_IOMAN *pIoman, FF_ERROR *pError);

FF_ERROR FF_Partition( FF_IOMAN *pIoman, FF_PartitionParameters_t *pParams );
FF_ERROR FF_Format( FF_IOMAN *pIoman, int xPartitionNumber, int xPreferFAT16, int xSmallClusters );

FF_T_BOOL        FF_isDirEmpty  (FF_IOMAN *pIoman, const FF_T_INT8 *Path);
FF_ERROR         FF_RmFile              (FF_IOMAN *pIoman, const FF_T_INT8 *path);
FF_ERROR         FF_RmDir               (FF_IOMAN *pIoman, const FF_T_INT8 *path);
FF_ERROR         FF_RmDirTree           (FF_IOMAN *pIoman, const FF_T_INT8 *path);
FF_ERROR         FF_Move                (FF_IOMAN *pIoman, const FF_T_INT8 *szSourceFile, const FF_T_INT8 *szDestinationFile);

FF_FILE *FF_Open(FF_IOMAN *pIoman, const FF_T_INT8 *path, FF_T_UINT8 Mode, FF_ERROR *pError);
// FF_T_BOOL        FF_isDirEmpty  (FF_IOMAN *pIoman, const FF_T_INT8 *Path);
// FF_ERROR         FF_RmFile              (FF_IOMAN *pIoman, const FF_T_INT8 *path);
// FF_ERROR         FF_RmDir               (FF_IOMAN *pIoman, const FF_T_INT8 *path);
// FF_ERROR         FF_RmDirTree           (FF_IOMAN *pIoman, const FF_T_INT8 *path);
// FF_ERROR         FF_Move                (FF_IOMAN *pIoman, const FF_T_INT8 *szSourceFile, const FF_T_INT8 *szDestinationFile);
// FF_ERROR FF_SetFileTime(FF_FILE *pFile, FF_SYSTEMTIME *pTime, FF_T_UINT aWhat);
// FF_ERROR FF_SetTime(FF_IOMAN *pIoman, const FF_T_INT8 *path, FF_SYSTEMTIME *pTime, FF_T_UINT aWhat);
FF_ERROR         FF_Close               (FF_FILE *pFile);
FF_T_SINT32      FF_GetC                (FF_FILE *pFile);
FF_T_SINT32  FF_GetLine         (FF_FILE *pFile, FF_T_INT8 *szLine, FF_T_UINT32 ulLimit);
FF_T_SINT32      FF_Read                (FF_FILE *pFile, FF_T_UINT32 ElementSize, FF_T_UINT32 Count, FF_T_UINT8 *buffer);
FF_T_SINT32      FF_Write               (FF_FILE *pFile, FF_T_UINT32 ElementSize, FF_T_UINT32 Count, FF_T_UINT8 *buffer);
FF_T_BOOL        FF_isEOF               (FF_FILE *pFile);
FF_T_SINT32      FF_BytesLeft   (FF_FILE *pFile); ///< Returns # of bytes left to read
FF_ERROR         FF_Seek                (FF_FILE *pFile, FF_T_SINT32 Offset, FF_T_INT8 Origin);
// FF_T_SINT32      FF_PutC                (FF_FILE *pFile, FF_T_UINT8 Value);

FF_T_UINT32    FF_Tell                (FF_FILE *pFile);
/*{
	return pFile ? pFile->FilePointer , 0;
}
*/
FF_T_UINT32    FF_Size                (FF_FILE *pFile);


// FF_T_UINT8       FF_GetModeBits (FF_T_INT8 *Mode);
// FF_ERROR     FF_CheckValid (FF_FILE *pFile);   ///< Check if pFile is a valid FF_FILE pointer
// FF_T_SINT32      FF_Invalidate (FF_IOMAN *pIoman); ///< Invalidate all handles belonging to pIoman
FF_T_SINT32      FF_ReadDirect          (FF_FILE *pFile, FF_T_UINT32 ElementSize, FF_T_UINT32 Count);
FF_ERROR	FF_FindFirst		(FF_IOMAN *pIoman, FF_DIRENT *pDirent, const FF_T_INT8 *path);
FF_ERROR	FF_FindNext			(FF_IOMAN *pIoman, FF_DIRENT *pDirent);
FF_ERROR	FF_MkDirTree		(FF_IOMAN *pIoman, const FF_T_INT8 *Path);
FF_T_UINT32 FF_getClusterChainNumber(FF_IOMAN *pIoman, FF_T_UINT32 nEntry, FF_T_UINT16 nEntrySize);
