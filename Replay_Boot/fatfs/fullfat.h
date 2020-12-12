#pragma once

#include <stdint.h>
#include <stdlib.h>

typedef char FF_T_BOOL;    // or C99 bool?
typedef char FF_T_INT8;
typedef uint8_t FF_T_UINT8;
typedef uint16_t FF_T_UINT16;
typedef int32_t FF_T_SINT32;
typedef uint32_t FF_T_UINT32;
typedef int32_t FF_ERROR;

#define FF_MAX_FILENAME (260/3)
#define FF_MAX_PATH (2600/8)

typedef struct {
    uint32_t err: 1;
    uint32_t module: 7;
    uint32_t function: 8;
    uint32_t error: 16;
} FF_ERROR_t;

void* ff_malloc(size_t size);
void ff_free(void* ptr);

#define FF_isERR(err) (err & (1<<31))

#define FF_TRUE                                  0x00000001
#define FF_SEEK                                  0x83120000
#define FF_OPEN                                  0x83020000
#define FF_FINDNEXT                              0x82060000
#define FF_FALSE                                 0x00000000
#define FF_ERR_NULL_POINTER                      0x00000002
#define FF_ERR_NOT_ENOUGH_MEMORY                 0x00000003
#define FF_ERR_NONE                              0x00000000
#define FF_ERR_IOMAN_OUT_OF_BOUNDS_READ          0x00000017
#define FF_ERR_IOMAN_NO_MOUNTABLE_PARTITION      0x0000000e
#define FF_ERR_IOMAN_NOT_FAT_FORMATTED           0x00000011
#define FF_ERR_IOMAN_NOT_ENOUGH_FREE_SPACE       0x00000016
#define FF_ERR_IOMAN_INVALID_PARTITION_NUM       0x00000010
#define FF_ERR_IOMAN_INVALID_FORMAT              0x0000000f
#define FF_ERR_IOMAN_DRIVER_FATAL_ERROR          0x0000000b
#define FF_ERR_IOMAN_DRIVER_FATAL_ERROR          0x0000000b
#define FF_ERR_IOMAN_DRIVER_BUSY                 0x0000000a
#define FF_ERR_FILE_NOT_FOUND                    0x0000001f
#define FF_ERR_FILE_NOT_FOUND                    0x0000001f
#define FF_ERR_FILE_IS_READ_ONLY                 0x00000021
#define FF_ERR_FILE_IS_READ_ONLY                 0x00000021
#define FF_ERR_FILE_INVALID_PATH                 0x00000022
#define FF_ERR_FILE_DESTINATION_EXISTS           0x00000026
#define FF_ERR_FILE_COULD_NOT_CREATE_DIRENT      0x00000029
#define FF_ERR_FILE_BAD_HANDLE                   0x0000002a
#define FF_ERR_FILE_ALREADY_OPEN                 0x0000001e
#define FF_ERR_DRIVER_FATAL_ERROR                0x8d0d000b
#define FF_ERR_DIR_END_OF_DIR                    0x00000034
#define FF_ERR_DEVICE_DRIVER_FAILED              0x00000004

#include "ff.h"

#define FF_REVISION            "FullFAT-FatFS Wrapper   "

#define FF_T_FAT12             0x0A
#define FF_T_FAT16             0x0B
#define FF_T_FAT32             0x0C

#define FF_MODE_READ           0x01
#define FF_MODE_WRITE          0x02
#define FF_MODE_APPEND         0x04
#define FF_MODE_CREATE         0x08
#define FF_MODE_TRUNCATE       0x10
#define FF_MODE_DIR            0x80

#define FF_SEEK_SET     1
#define FF_SEEK_CUR     2
#define FF_SEEK_END     3

#define FF_FAT_ATTR_READONLY        0x01
#define FF_FAT_ATTR_HIDDEN          0x02
#define FF_FAT_ATTR_SYSTEM          0x04
#define FF_FAT_ATTR_VOLID           0x08
#define FF_FAT_ATTR_DIR             0x10
#define FF_FAT_ATTR_ARCHIVE         0x20
#define FF_FAT_ATTR_LFN             0x0F

typedef struct {
    FF_T_UINT8         Type;
    FF_T_UINT16        BlkSize;
    FF_T_UINT8         BlkFactor;
    FF_T_UINT8         SectorsPerCluster;
    FF_T_UINT32        NumClusters;
    FF_T_UINT32        FreeClusterCount;
} FF_PARTITION;

typedef struct _FF_IOMAN {
    FF_PARTITION*      pPartition;
} FF_IOMAN;
typedef struct _FF_FILE FF_FILE;
typedef struct _FF_DIRENT {
    FF_T_UINT8     Attrib;
    FF_T_INT8      FileName[FF_MAX_FILENAME];
    DIR            dir;
} FF_DIRENT;

typedef FF_T_SINT32 (*FF_WRITE_BLOCKS)    (FF_T_UINT8* pBuffer, FF_T_UINT32 SectorAddress, FF_T_UINT32 Count, void* pParam);
typedef FF_T_SINT32 (*FF_READ_BLOCKS)     (FF_T_UINT8* pBuffer, FF_T_UINT32 SectorAddress, FF_T_UINT32 Count, void* pParam);

typedef enum _FF_SizeType {
    eSizeIsQuota,
    eSizeIsPercent,
    eSizeIsSectors,
} eSizeType_t;

typedef struct _FF_PartitionParameters {
    uint32_t ulSectorCount;
    uint32_t ulHiddenSectors;
    uint32_t ulInterSpace;
    int xSizes[4];
    int xPrimaryCount;
    eSizeType_t eSizeType;
} FF_PartitionParameters_t;

const FF_T_INT8* FF_GetErrDescription(FF_ERROR iErrorCode, char* apBuf, int aMaxlen);

FF_IOMAN*   FF_CreateIOMAN             (FF_T_UINT8* pCacheMem, FF_T_UINT32 Size, FF_T_UINT16 BlkSize, FF_ERROR* pError);
FF_ERROR    FF_DestroyIOMAN            (FF_IOMAN* pIoman);
FF_ERROR    FF_RegisterBlkDevice       (FF_IOMAN* pIoman, FF_T_UINT16 BlkSize, FF_WRITE_BLOCKS fnWriteBlocks, FF_READ_BLOCKS fnReadBlocks, void* pParam);
FF_ERROR    FF_UnregisterBlkDevice     (FF_IOMAN* pIoman);
FF_ERROR    FF_MountPartition          (FF_IOMAN* pIoman, FF_T_UINT8 PartitionNumber);
FF_ERROR    FF_UnmountPartition        (FF_IOMAN* pIoman);
FF_T_UINT32 FF_GetVolumeSize           (FF_IOMAN* pIoman);
FF_ERROR    FF_FlushCache              (FF_IOMAN* pIoman);
FF_T_BOOL   FF_Mounted                 (FF_IOMAN* pIoman);
FF_ERROR    FF_IncreaseFreeClusters    (FF_IOMAN* pIoman, FF_T_UINT32 Count);
FF_T_SINT32 FF_GetPartitionBlockSize   (FF_IOMAN* pIoman);
FF_T_UINT32 FF_GetFreeSize             (FF_IOMAN* pIoman, FF_ERROR* pError);

FF_ERROR    FF_Partition               (FF_IOMAN* pIoman, FF_PartitionParameters_t* pParams );
FF_ERROR    FF_Format                  (FF_IOMAN* pIoman, int xPartitionNumber, int xPreferFAT16, int xSmallClusters );

FF_T_BOOL        FF_isDirEmpty         (FF_IOMAN* pIoman, const FF_T_INT8* Path);
FF_ERROR         FF_RmFile             (FF_IOMAN* pIoman, const FF_T_INT8* path);
FF_ERROR         FF_RmDir              (FF_IOMAN* pIoman, const FF_T_INT8* path);
FF_ERROR         FF_RmDirTree          (FF_IOMAN* pIoman, const FF_T_INT8* path);
FF_ERROR         FF_Move               (FF_IOMAN* pIoman, const FF_T_INT8* szSourceFile, const FF_T_INT8* szDestinationFile);

FF_FILE*         FF_Open               (FF_IOMAN* pIoman, const FF_T_INT8* path, FF_T_UINT8 Mode, FF_ERROR* pError);
FF_ERROR         FF_Close              (FF_FILE* pFile);
FF_T_SINT32      FF_GetC               (FF_FILE* pFile);
FF_T_SINT32      FF_GetLine            (FF_FILE* pFile, FF_T_INT8* szLine, FF_T_UINT32 ulLimit);
FF_T_SINT32      FF_Read               (FF_FILE* pFile, FF_T_UINT32 ElementSize, FF_T_UINT32 Count, FF_T_UINT8* buffer);
FF_T_SINT32      FF_Write              (FF_FILE* pFile, FF_T_UINT32 ElementSize, FF_T_UINT32 Count, FF_T_UINT8* buffer);
FF_T_BOOL        FF_isEOF              (FF_FILE* pFile);
FF_T_SINT32      FF_BytesLeft          (FF_FILE* pFile); ///< Returns # of bytes left to read
FF_ERROR         FF_Seek               (FF_FILE* pFile, int64_t Offset, FF_T_INT8 Origin);

uint64_t         FF_Tell               (FF_FILE* pFile);
uint64_t         FF_Size               (FF_FILE* pFile);

FF_T_SINT32      FF_ReadDirect         (FF_FILE* pFile, FF_T_UINT32 ElementSize, FF_T_UINT32 Count);
FF_ERROR         FF_FindFirst          (FF_IOMAN* pIoman, FF_DIRENT* pDirent, const FF_T_INT8* path);
FF_ERROR         FF_FindNext           (FF_IOMAN* pIoman, FF_DIRENT* pDirent);
FF_ERROR         FF_MkDirTree          (FF_IOMAN* pIoman, const FF_T_INT8* Path);
FF_T_UINT32      FF_getClusterChainNumber(FF_IOMAN* pIoman, FF_T_UINT32 nEntry, FF_T_UINT16 nEntrySize);
