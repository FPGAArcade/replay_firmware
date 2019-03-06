#include "fullfat.h"
#include "ff.h"
#include "diskio.h"

#include <stdio.h>
#undef sprintf
#include "messaging.h"

static FF_ERROR mapError(FRESULT err)
{
	switch (err)
	{
	case FR_OK:							/* (0) Succeeded */
				return FF_ERR_NONE;
	case FR_DISK_ERR:					/* (1) A hard error occurred in the low level disk I/O layer */
				return FF_ERR_DEVICE_DRIVER_FAILED;
	case FR_INT_ERR:					/* (2) Assertion failed */
				return FF_ERR_IOMAN_DRIVER_FATAL_ERROR;
	case FR_NOT_READY:					/* (3) The physical drive cannot work */
				return FF_ERR_IOMAN_NO_MOUNTABLE_PARTITION;
	case FR_NO_FILE:					/* (4) Could not find the file */
				return FF_ERR_FILE_NOT_FOUND;
	case FR_NO_PATH:					/* (5) Could not find the path */
				return FF_ERR_FILE_NOT_FOUND;
	case FR_INVALID_NAME:				/* (6) The path name format is invalid */
				return FF_ERR_FILE_INVALID_PATH;
	case FR_DENIED:						/* (7) Access denied due to prohibited access or directory full */
				return FF_ERR_FILE_IS_READ_ONLY;
	case FR_EXIST:						/* (8) Access denied due to prohibited access */
				return FF_ERR_FILE_DESTINATION_EXISTS;
	case FR_INVALID_OBJECT:				/* (9) The file/directory object is invalid */
				return FF_ERR_FILE_BAD_HANDLE;
	case FR_WRITE_PROTECTED:			/* (10) The physical drive is write protected */
				return FF_ERR_FILE_IS_READ_ONLY;
	case FR_INVALID_DRIVE:				/* (11) The logical drive number is invalid */
				return FF_ERR_IOMAN_INVALID_PARTITION_NUM;
	case FR_NOT_ENABLED:				/* (12) The volume has no work area */
				return FF_ERR_IOMAN_NOT_ENOUGH_FREE_SPACE;
	case FR_NO_FILESYSTEM:				/* (13) There is no valid FAT volume */
				return FF_ERR_IOMAN_NOT_FAT_FORMATTED;
	case FR_MKFS_ABORTED:				/* (14) The f_mkfs() aborted due to any problem */
				return FF_ERR_IOMAN_INVALID_FORMAT;
	case FR_TIMEOUT:					/* (15) Could not get a grant to access the volume within defined period */
				return FF_ERR_IOMAN_DRIVER_BUSY;
	case FR_LOCKED:						/* (16) The operation is rejected according to the file sharing policy */
				return FF_ERR_FILE_ALREADY_OPEN;
	case FR_NOT_ENOUGH_CORE:			/* (17) LFN working buffer could not be allocated */
				return FF_ERR_NOT_ENOUGH_MEMORY;
	case FR_TOO_MANY_OPEN_FILES:		/* (18) Number of open files > FF_FS_LOCK */
				return FF_ERR_FILE_COULD_NOT_CREATE_DIRENT;
	case FR_INVALID_PARAMETER:			/* (19) Given parameter is invalid */
				return FF_ERR_NULL_POINTER;
	};
	return 1; // FF_ERR_GENERIC
}

const FF_T_INT8 *FF_GetErrDescription(FF_ERROR iErrorCode, char *apBuf, int aMaxlen)
{
	const char* err = "No error";
	switch (iErrorCode)
	{
	case FF_ERR_NONE:							err = "(0) Succeeded";
	case FF_ERR_DEVICE_DRIVER_FAILED:			err = "(1) A hard error occurred in the low level disk I/O layer";
	case FF_ERR_IOMAN_DRIVER_FATAL_ERROR:		err = "(2) Assertion failed";
	case FF_ERR_IOMAN_NO_MOUNTABLE_PARTITION:	err = "(3) The physical drive cannot work";
	case FF_ERR_FILE_NOT_FOUND:					err = "(4/5) Could not find the file/path";
	case FF_ERR_FILE_INVALID_PATH:				err = "(6) The path name format is invalid";
	case FF_ERR_FILE_IS_READ_ONLY:				err = "(7/10) Access denied due to prohibited access or directory full, or the physical drive is write protected";
	case FF_ERR_FILE_DESTINATION_EXISTS:		err = "(8) Access denied due to prohibited access";
	case FF_ERR_FILE_BAD_HANDLE:				err = "(9) The file/directory object is invalid";
	case FF_ERR_IOMAN_INVALID_PARTITION_NUM:	err = "(11) The logical drive number is invalid";
	case FF_ERR_IOMAN_NOT_ENOUGH_FREE_SPACE:	err = "(12) The volume has no work area";
	case FF_ERR_IOMAN_NOT_FAT_FORMATTED:		err = "(13) There is no valid FAT volume";
	case FF_ERR_IOMAN_INVALID_FORMAT:			err = "(14) The f_mkfs() aborted due to any problem";
	case FF_ERR_IOMAN_DRIVER_BUSY:				err = "(15) Could not get a grant to access the volume within defined period";
	case FF_ERR_FILE_ALREADY_OPEN:				err = "(16) The operation is rejected according to the file sharing policy";
	case FF_ERR_NOT_ENOUGH_MEMORY:				err = "(17) LFN working buffer could not be allocated";
	case FF_ERR_FILE_COULD_NOT_CREATE_DIRENT:	err = "(18) Number of open files > FF_FS_LOCK";
	case FF_ERR_NULL_POINTER:					err = "(19) Given parameter is invalid";
	default:									err = "(?) Generic";
	};
	snprintf (apBuf, aMaxlen, "%d : %s", (int)iErrorCode, err);
	return apBuf;
}


static FF_T_UINT8* CacheMem = 0;
static FF_T_UINT32 CacheSize = 0;
static FF_T_UINT16 PreferredBlkSize = 0;
static FF_T_UINT16 DriverBlkSize = 0;
static FF_WRITE_BLOCKS DriverWriteBlockFunction = 0;
static FF_READ_BLOCKS DriverReadBlockFunction = 0;
static void* DriverFunctionParam = 0;

DSTATUS disk_status(BYTE pdrv)
{
	return 0;//STA_NOINIT;
}
DSTATUS disk_initialize(BYTE pdrv)
{
	return 0;//STA_NOINIT;
}
DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void *buff)
{
	return RES_PARERR;
}
DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count)
{
	return FF_isERR(DriverReadBlockFunction(buff, sector, count, DriverFunctionParam)) ? RES_ERROR : RES_OK;
}
DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count)
{
	return FF_isERR(DriverWriteBlockFunction((BYTE*)buff, sector, count, DriverFunctionParam)) ? RES_ERROR : RES_OK;
}

FF_IOMAN* FF_CreateIOMAN(FF_T_UINT8 *pCacheMem, FF_T_UINT32 Size, FF_T_UINT16 BlkSize, FF_ERROR *pError)
{
	CacheMem = pCacheMem;
	CacheSize = Size;
	PreferredBlkSize = BlkSize;

	return 0;
}
FF_ERROR FF_DestroyIOMAN(FF_IOMAN *pIoman)
{
	CacheMem = 0;
	CacheSize = 0;
	PreferredBlkSize = 0;
	return 0;
}
FF_ERROR FF_RegisterBlkDevice(FF_IOMAN *pIoman, FF_T_UINT16 BlkSize, FF_WRITE_BLOCKS fnWriteBlocks, FF_READ_BLOCKS fnReadBlocks, void *pParam)
{
	DriverBlkSize = BlkSize;
	DriverWriteBlockFunction = fnWriteBlocks;
	DriverReadBlockFunction = fnReadBlocks;
	DriverFunctionParam = pParam;
	return 0;
}
FF_ERROR FF_UnregisterBlkDevice(FF_IOMAN *pIoman)
{
	DriverBlkSize = 0;
	DriverWriteBlockFunction = 0;
	DriverReadBlockFunction = 0;
	DriverFunctionParam = 0;
	return 0;
}
FF_ERROR FF_MountPartition (FF_IOMAN *pIoman, FF_T_UINT8 PartitionNumber)
{
	Assert(CacheSize >= sizeof(FATFS));
	FATFS* fs = (FATFS*)(void*)CacheMem;
	return mapError(f_mount(fs, "", 0));
}
FF_ERROR FF_UnmountPartition(FF_IOMAN *pIoman)
{
	FATFS* fs = 0;
	return mapError(f_mount(fs, "", 0));
}

FF_ERROR FF_FlushCache (FF_IOMAN *pIoman)
{
	return 0;
}
FF_T_BOOL FF_Mounted(FF_IOMAN *pIoman)
{
	return 0;
}

FF_ERROR FF_IncreaseFreeClusters(FF_IOMAN *pIoman, FF_T_UINT32 Count)
{
	return 0;
}

FF_T_SINT32 FF_GetPartitionBlockSize(FF_IOMAN *pIoman)
{
	Assert(FF_MIN_SS == FF_MAX_SS);
	return FF_MAX_SS;
}

static FRESULT get_total_and_free_mb(DWORD* fre_sect, DWORD* tot_sect)
{
	FATFS *fs = 0;
	DWORD fre_clust;

	FRESULT ret = f_getfree("0:", &fre_clust, &fs);
	Assert(!ret);
	if (!ret)
		return ret;

	Assert(FF_GetPartitionBlockSize(0) == 512);

	if (tot_sect)
		*tot_sect = (fs->n_fatent - 2) * fs->csize / (2 * 1024);
	if (fre_sect)
		*fre_sect = fre_clust * fs->csize / (2 * 1024);

	return ret;
}

FF_T_UINT32 FF_GetVolumeSize(FF_IOMAN *pIoman)
{
	DWORD tot_mb = 0;
	get_total_and_free_mb(0, &tot_mb);
	return tot_mb;
}
FF_T_UINT32 FF_GetFreeSize(FF_IOMAN *pIoman, FF_ERROR *pError)
{
	DWORD free_mb = 0;
	get_total_and_free_mb(&free_mb, 0);
	return free_mb;
}

#if FF_MULTI_PARTITION
/* Volume management table defined by user (required when FF_MULTI_PARTITION == 1) */

PARTITION VolToPart[] = {
    {0, 0},    /* "0:" ==> Physical drive 0, auto detection */
    {1, 0},    /* "1:" ==> Physical drive 1, auto detection */
    {2, 0},    /* "2:" ==> Physical drive 2, auto detection */
    {3, 0},    /* "3:" ==> Physical drive 3, auto detection */
};
#endif

FF_ERROR FF_Partition( FF_IOMAN *pIoman, FF_PartitionParameters_t *pParams )
{
	// params ignored
	DWORD plist[] = {100, 0, 0, 0};  /* Divide drive into two partitions */
	BYTE work[FF_MAX_SS];

	return mapError(f_fdisk(0, plist, work));
}
FF_ERROR FF_Format( FF_IOMAN *pIoman, int xPartitionNumber, int xPreferFAT16, int xSmallClusters )
{
	BYTE work[FF_MAX_SS];
	return mapError(f_mkfs("", FM_ANY, 0, work, sizeof(work)));
}

FF_T_BOOL FF_isDirEmpty(FF_IOMAN *pIoman, const FF_T_INT8 *Path)
{
	FF_DIRENT dirent;
	FF_ERROR ret = FF_FindFirst(pIoman, &dirent, Path);
	return FF_GETERROR(ret) == FF_ERR_DIR_END_OF_DIR;
}
FF_ERROR FF_RmFile(FF_IOMAN *pIoman, const FF_T_INT8 *path)
{
	return mapError(f_unlink(path));
}
FF_ERROR FF_RmDir(FF_IOMAN *pIoman, const FF_T_INT8 *path)
{
	return mapError(f_unlink(path));
}
FF_ERROR FF_RmDirTree(FF_IOMAN *pIoman, const FF_T_INT8 *path)
{
	return mapError(f_unlink(path));
}
FF_ERROR FF_Move(FF_IOMAN *pIoman, const FF_T_INT8 *szSourceFile, const FF_T_INT8 *szDestinationFile)
{
	return mapError(f_rename(szSourceFile, szDestinationFile));
}

FF_FILE *FF_Open(FF_IOMAN *pIoman, const FF_T_INT8 *path, FF_T_UINT8 Mode, FF_ERROR *pError)
{
	FIL* fp = ff_malloc(sizeof(FIL));
	BYTE mode = Mode;	// TODO map mode!
	FRESULT result = f_open (fp, path, mode);
	if (pError)
		*pError = FF_OPEN| mapError(result);
	return (FF_FILE*)fp;
}

FF_ERROR FF_Close(FF_FILE *pFile)
{
	FF_ERROR ret = mapError(f_close((FIL*)pFile));
	if (pFile)
		ff_free(pFile);
	return ret;
}
FF_T_SINT32 FF_GetC(FF_FILE *pFile)
{
	FF_T_UINT8 c;
	UINT read = 0;
	if (f_eof((FIL*)pFile))
		return -1;
	mapError(f_read((FIL*)pFile, &c, sizeof(c), &read));
	Assert(read == sizeof(c));
	return (FF_T_UINT32)c;
}
FF_T_SINT32 FF_GetLine(FF_FILE *pFile, FF_T_INT8 *szLine, FF_T_UINT32 ulLimit)
{
	if (szLine == f_gets(szLine, ulLimit, (FIL*)pFile))
		return strlen(szLine);
	return 0;
}
FF_T_SINT32 FF_Read(FF_FILE *pFile, FF_T_UINT32 ElementSize, FF_T_UINT32 Count, FF_T_UINT8 *buffer)
{
	UINT read = 0;
	mapError(f_read((FIL*)pFile, buffer, ElementSize*Count, &read));
	return read;
}
FF_T_SINT32 FF_Write(FF_FILE *pFile, FF_T_UINT32 ElementSize, FF_T_UINT32 Count, FF_T_UINT8 *buffer)
{
	UINT written = 0;
	mapError(f_write((FIL*)pFile, buffer, ElementSize*Count, &written));
	return written;
}
FF_T_BOOL FF_isEOF(FF_FILE *pFile)
{
	return f_eof((FIL*)pFile);
}
FF_T_SINT32 FF_BytesLeft(FF_FILE *pFile) ///< Returns # of bytes left to read
{
	return f_size((FIL*)pFile) - f_tell((FIL*)pFile);
}
FF_ERROR FF_Seek(FF_FILE *pFile, FF_T_SINT32 Offset, FF_T_INT8 Origin)
{
	if (Origin == FF_SEEK_CUR)
		Offset += f_tell((FIL*)pFile);
	else if (Origin == FF_SEEK_END)
		Offset += f_size((FIL*)pFile);
	return mapError(f_lseek((FIL*)pFile, Offset));
}
// FF_T_SINT32      FF_PutC                (FF_FILE *pFile, FF_T_UINT8 Value);

FF_T_UINT32 FF_Tell(FF_FILE *pFile)
{
	FF_T_UINT32 pos = f_tell((FIL*)pFile);
	return pos;
}
/*{
	return pFile ? pFile->FilePointer , 0;
}
*/
FF_T_UINT32 FF_Size(FF_FILE *pFile)
{
	FF_T_UINT32 size = f_size((FIL*)pFile);
	return size;
}


// FF_T_UINT8       FF_GetModeBits (FF_T_INT8 *Mode);
// FF_ERROR     FF_CheckValid (FF_FILE *pFile);   ///< Check if pFile is a valid FF_FILE pointer
// FF_T_SINT32      FF_Invalidate (FF_IOMAN *pIoman); ///< Invalidate all handles belonging to pIoman
FF_T_SINT32 FF_ReadDirect(FF_FILE *pFile, FF_T_UINT32 ElementSize, FF_T_UINT32 Count)
{
	Assert(0);
	return 0;
}


FF_ERROR FF_FindFirst(FF_IOMAN *pIoman, FF_DIRENT *pDirent, const FF_T_INT8 *path)
{
	FRESULT ret;
	DIR* dir = &pDirent->dir;
	memset(dir, 0x00, sizeof(DIR));
	if (!(ret = f_opendir(dir, path)))
		return mapError(ret);

	return FF_FindNext(pIoman, pDirent);
}
FF_ERROR FF_FindNext(FF_IOMAN *pIoman, FF_DIRENT *pDirent)
{
	FRESULT ret;
	FILINFO info;
	DIR* dir = &pDirent->dir;
	size_t filelen = sizeof(pDirent->FileName);

	if (!(ret = f_readdir(dir, &info)))
		return mapError(ret);

	if (info.fname[0] == 0)
	{
		f_closedir(dir);
		memset(dir, 0x00, sizeof(DIR));
		return FF_ERR_DIR_END_OF_DIR | FF_FINDNEXT;
	}

	pDirent->Attrib = info.fattrib;
	if (filelen > sizeof(info.fname))
		filelen = sizeof(info.fname);
	memcpy(pDirent->FileName, info.fname, filelen);

	return mapError(ret);
}

FF_ERROR FF_MkDirTree(FF_IOMAN *pIoman, const FF_T_INT8 *Path)
{
	return mapError(f_mkdir(Path));
}
