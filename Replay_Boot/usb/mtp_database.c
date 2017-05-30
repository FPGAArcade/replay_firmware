#include "mtp_database.h"
#include "messaging.h"
#include <string.h>
#include "hardware/io.h"

/*static*/ uint16_t num_clusters = 0;
/*static*/ int32_t dirclusters[256] = { 0 };
/*static*/ uint32_t assoclink[256] = { 0 };
/*static*/ uint16_t num_moved = 0;
/*static*/ uint32_t moved[256] = { 0 };

static MtpStorage storage;

void mtp_open_session()
{
    // invalidate clusters
    num_clusters = 0;
    memset(dirclusters, 0x00, sizeof(dirclusters)); // dircluster == 0 is invalid
    memset(assoclink, 0x00, sizeof(assoclink)); // dircluster == 0 is invalid
    num_moved = 0;
    memset(moved, 0x00, sizeof(moved)); //
}

void mtp_close_session()
{

}

uint32_t mtp_get_num_storages()
{
	return FF_Mounted(pIoman) ? 1 : 0;
}

MtpStorageId mtp_get_storage_id(uint32_t index)
{
	return 0x00010001;	// hardcoded
}

const MtpStorage* mtp_get_storage_info(MtpStorageId id)
{
	if (mtp_get_num_storages() == 0 || id != mtp_get_storage_id(0))
		return 0;

	memset(&storage, 0x00, sizeof(storage));

	storage.storage_id = id;

    strncpy(storage.description, pIoman->pPartition->VolLabel, sizeof(storage.description));
    storage.description[sizeof(storage.description)-1] = '\0';
    for (int i = sizeof(storage.description)-1; i; --i)
    	if (isspace(storage.description[i]) || storage.description[i] == '\0')
    		storage.description[i] = 0;
    	else
    		break;

	storage.storage_type = PTP_ST_RemovableRAM;
	storage.filesystem_type = PTP_FST_GenericHierarchical;
	storage.max_capacity_mb = FF_GetVolumeSize(pIoman);
	storage.free_space_mb = FF_GetFreeSize(pIoman, NULL);

	storage.read_write = IO_Input_H(PIN_WRITE_PROTECT) ? PTP_AC_ReadOnly : PTP_AC_ReadWrite;

	return &storage;
}



// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// MTP DATABASE


/*
    'dirclusters' is an index-mapping from 16bit to the full 32bit FF_DIRENT.DirCluster.
    The 16bit index is the top 16bit of the MTP ObjectHandle minus 1. The lower 16bit is the FAT ItemIndex.
    The cluster index is +1 since it could otherwise cause the handle to result in 0x00000000 which is illegal (== root parent)

    'assoclink' is an index-mapping to the full 32bit ObjectHandle of the directory entry in the parent dir.

    The relationship is defined as

 .--------------------.
 |                    V
 |  DIR (cluster = 0x000001f6, dir_index = 1)
 |  |
 |  +-> ENTRY_A (file, item = 0) ; handle = 0x00010000
 |  |
 |  +-> ENTRY_B  (dir, item = 3) ; handle = 0x00010003  <----------------.
 |      |                                                                |
 |      V                                                                |
 |      DIR (ENTRY_B, cluster = 0x000320f8, dir_index = 2)  <------------+--.
 |      |                                                                |  |
 |      +-> ENTRY_C (file, item = 0) ; handle = 0x00020000               |  |
 |  ___________________________________________________________________  |  |
 '- dirclusters[0x0000] = 0x000001f6  |  assoclink[0x0000] = 0x00000000  |  |
    dirclusters[0x0001] = 0x000320f8  |  assoclink[0x0001] = 0x00010003 -'  |
                                |                                           |
                                '-------------------------------------------'                             

    This means that given an MTP ObjectHandle its parent handle is 
        uint32_t parent = assoclink[(handle >> 16) - 1];

        i.e for ENTRY_C this would be 
            assoclink[[0x00020000 >> 16) - 1] => 0x00010003

    In order to find all entries in a directory, given the ObjectHandle of the directory, we first need to establish the directory cluster.
    This value is written in the assoclink table at a given index. The dirclusters table stores the correct cluster at the same index.

    i.e for ENTRY_B this would be 
        assoclink{0x00010003} => 0x0001
        dirclusters[0x0001] => 0x000320f8

    The whole reason we need this is because MTP describe the parent/child structure by having the children point to the parent, while
    FAT is structured the other way (a directory lists its children)
*/

uint32_t TrackRenames(uint32_t handle)
{
    for (size_t i = 0; i < num_moved; i++)
        if (moved[i*2+0] == handle) {
            DEBUG(1,"\t\t\t\t remapping %08x -> %08x", moved[i*2+0], moved[i*2+1]);
            return moved[i*2+1];
        }
    return handle;
}
/*
void SendMovedFiles()
{
    for (size_t i = 0; i < num_moved; i++) {
        uint32_t handle = moved[i*2+0];

        size_t offset = 0;
        offset = Write32bits(ptp_out, offset, handle);

        ptp_out->length = PTP_USB_BULK_HDR_LEN + offset;
        ptp_out->type = PTP_USB_CONTAINER_EVENT;
        ptp_out->code = PTP_EC_ObjectRemoved;
        
        DEBUG(1,"\t\t\t\t removing %08x", handle);
        UsbSendPacket((uint8_t*)ptp_out, ptp_out->length);
    }

    for (size_t i = 0; i < num_moved; i++) {
        uint32_t handle = moved[i*2+1];

        size_t offset = 0;
        offset = Write32bits(ptp_out, offset, handle);

        ptp_out->length = PTP_USB_BULK_HDR_LEN + offset;
        ptp_out->type = PTP_USB_CONTAINER_EVENT;
        ptp_out->code = PTP_EC_ObjectAdded;
        
        DEBUG(1,"\t\t\t\t adding %08x", handle);
        UsbSendPacket((uint8_t*)ptp_out, ptp_out->length);
    }
    num_moved = 0;
}
*/

void DumpClusters() {
    DEBUG(1,"\t\t\t\tnum_clusters : %d", num_clusters);
    for (int i = 0; i < num_clusters; ++i)
    {
        DEBUG(1,"\t\t\t\t%04x ; dir = %08x ; parent = %08x", i, dirclusters[i], assoclink[i]);
    }
}

void AddCluster(uint32_t cluster, uint32_t handle) {
    for (uint16_t i = 0; i < num_clusters; ++i) {
        if (assoclink[i] == handle) {
            if (dirclusters[i] != cluster)
                DEBUG(1,"ERROR! mismatching cluster / assoclink!");
            return;
        }
    }

    dirclusters[num_clusters] = cluster;
    assoclink[num_clusters] = handle;
    num_clusters++;
}

static void DeleteCluster(uint32_t handle) {
    uint16_t cluster_index = 0xffff;
    for (uint16_t i = 0; i < num_clusters; ++i)
        if (assoclink[i] == handle) {
            if (!dirclusters[i])
                DEBUG(1,"ERROR! bad cluster value!");
            cluster_index = i;
            break;
        }

    // we can't remove the cluster here, but we mark it as 'bad'
    dirclusters[cluster_index] = 0;
}

uint16_t GetClusterIndex(uint32_t cluster) {
    for (uint16_t i = 0; i < num_clusters; ++i) {
        if (dirclusters[i] == cluster) {
            return i;
        }
    }

    DEBUG(1,"ERROR! could not find cluster index = %08x!", cluster);
    DumpClusters();
    return 0;
}

static uint16_t GetParentCluster(uint32_t handle) {
    // given an object handle, we scan through the assoclink list trying to find the index of the link
    for (uint16_t i = 0; i < num_clusters; ++i)
        if (assoclink[i] == handle) {
            return i;
        }
    return 0xffff;
}

/*
static uint32_t CreateObjectHandle(uint32_t dircluster, uint16_t itemindex) {
    uint32_t handle = ((cGetClusterIndex(dircluster) + 1) << 16) | pDirent->CurrentItem;
    return handle;
}
static uint32_t CreateDirent(uint32_t dircluster, uint16_t itemindex) {
    uint32_t handle = ((cGetClusterIndex(dircluster) + 1) << 16) | pDirent->CurrentItem;
    return handle;
}
*/
uint32_t FindClusterByHandle(uint32_t parent) {
    if (parent == 0xffffffff)
        parent = 0x00000000;

    // if we have no cluster indices on record we need to boostrap
    if (num_clusters == 0)
    {
        DEBUG(1,"\t\t\t\tcheck parent");
        // if the HOST is asking for something else than the root handles at this point it must be an error
        if (parent != 0x00000000)
            return 0;

        AddCluster(pIoman->pPartition->RootDirCluster, 0x00000000);
    }

    uint16_t cluster_index = GetParentCluster(parent);
    DEBUG(1,"\t\t\t\tcheck cluster index : %04x", cluster_index);
    if (cluster_index == 0xffff || cluster_index >= num_clusters || dirclusters[cluster_index] == 0 ) {
        DEBUG(1,"\t\t\t\tDIR CLUSTER NOT FOUND!");
        return 0;
    }

    DEBUG(1,"\t\t\t\tdircluster : %08x", dirclusters[cluster_index]);

    return dirclusters[cluster_index];
}


const char* GetFullPathFromHandle(uint32_t handle, const char* optional_filename, FF_DIRENT* optional_dirent)
{
    // create full path

    FF_DIRENT dirent, *pDirent;

    static char buffer[FF_MAX_PATH];
    size_t pos = sizeof(buffer) - 1;
    buffer[pos] = 0;

    if (optional_filename) {
        size_t len = strlen(optional_filename);
        pos -= len;
        memcpy(&buffer[pos], optional_filename, len);
        buffer[--pos] = '/';
    }

    pDirent = optional_dirent ? optional_dirent : &dirent;

    while(handle) {
        uint16_t cluster_index = (handle >> 16) - 1;
        uint16_t item_index = handle & 0xffff;

        if (FF_isERR(GetDirentInfo(cluster_index, item_index, pDirent)))
            return 0;

        // insert current filename infront of existing name
        size_t len = strlen(pDirent->FileName);
        pos -= len;
        memcpy(&buffer[pos], pDirent->FileName, len);
        buffer[--pos] = '/';

        handle = assoclink[cluster_index];
    }

    const char* path = &buffer[pos];
    return path;
}


// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// FetchDirent will progress through the TOC, starting att 'CurrentItem', until it finds the end-of-dir.
// For each valid directory entry it will callback, check its return value, and (if 'true') continue to find the next directory entry
// pDirent must have 'DirCluster' and 'CurrentItem' filled out - i.e. through a previous call to FF_InitEntryFetch() with DirCluster filled out.
// callback will only be called for a valid directory entry, with _matching_ 'CurrentItem' set (i.e. not pointing to 'next')
// pDirent will be updated to point to the next (possibly invalid) directory entry before returning from FetchDirent()
FF_ERROR FetchDirents(FF_DIRENT* pDirent, DirentCallback callback, void* context)
{
    FF_ERROR err = FF_ERR_DIR_END_OF_DIR | FF_FINDNEXTINDIR;
    FF_T_UINT8  numLFNs;
    FF_T_UINT8 EntryBuffer[32];

  for(; pDirent->CurrentItem < 0xFFFF; pDirent->CurrentItem += 1) {

//    DEBUG(1,"0x%04x :", pDirent->CurrentItem);
    const FF_T_UINT16 currentEntry = pDirent->CurrentItem;

    err = FF_FetchEntryWithContext(pIoman, pDirent->CurrentItem, &pDirent->FetchContext, EntryBuffer);
    if (FF_isERR(err))
        break;

    if(EntryBuffer[0] == FF_FAT_DELETED) {
//      DEBUG(1,"       : <DELETED>");
      continue;
    }

    if(FF_isEndOfDir(EntryBuffer)){
//      DEBUG(1,"      : <ENDOFDIR>");
      err = FF_ERR_DIR_END_OF_DIR | FF_FINDNEXTINDIR;
      break;
    }

    pDirent->Attrib = FF_getChar(EntryBuffer, (FF_T_UINT16)(FF_FAT_DIRENT_ATTRIB));
    if((pDirent->Attrib & FF_FAT_ATTR_LFN) == FF_FAT_ATTR_LFN) {

      numLFNs = (FF_T_UINT8)(EntryBuffer[0] & ~0x40);
//      DEBUG(1,"       : numLFNs = %d", numLFNs);

      err = FF_FetchEntryWithContext(pIoman, (pDirent->CurrentItem + numLFNs), &pDirent->FetchContext, EntryBuffer);
      if(FF_isERR(err))
        break;

      if(EntryBuffer[0] == FF_FAT_DELETED) {
//        DEBUG(1,"       : <DELETED> (SHORT, 0x%04x)", pDirent->CurrentItem + numLFNs);
        pDirent->CurrentItem += numLFNs;    // jump to short name, for() will update to next entry
        continue;
      }

      err = FF_PopulateLongDirent(pIoman, pDirent, pDirent->CurrentItem, &pDirent->FetchContext);
      if(FF_isERR(err))
        break;

    if (callback)
    {
        const FF_T_UINT16 nextEntry = pDirent->CurrentItem;
        pDirent->CurrentItem = currentEntry;
        err = callback(pDirent, context);
        pDirent->CurrentItem = nextEntry;
        if (FF_isERR(err))
            break;
    }

      // pDirent->CurrentItem is already updated (by FF_PopulateLongDirent)
      // Adjust the value to account for our top 'for()' loop
      pDirent->CurrentItem -= 1;
//      DEBUG(1,"       :    LFN : name = %s", pDirent->FileName);

      FF_PopulateShortDirent(pIoman, pDirent, EntryBuffer);
//      DEBUG(1,"       : nonLFN : name = %s (SHORT, 0x%04x)", pDirent->FileName, pDirent->CurrentItem);

    } else if((pDirent->Attrib & FF_FAT_ATTR_VOLID) == FF_FAT_ATTR_VOLID) {
      // Do Nothing
//      DEBUG(1,"       : <VOLID>");

    } else {
      FF_PopulateShortDirent(pIoman, pDirent, EntryBuffer);

    if (callback)
    {
        const FF_T_UINT16 nextEntry = pDirent->CurrentItem;
        pDirent->CurrentItem = currentEntry;
        err = callback(pDirent, context);
        pDirent->CurrentItem = nextEntry;
        if (FF_isERR(err))
            break;
    }

//      DEBUG(1,"       : nonLFN : name = %s", pDirent->FileName);
    }
  }

// ???
  FF_CleanupEntryFetch(pIoman, &pDirent->FetchContext);

  return err;
}

static FF_ERROR GetDirentInfo_Callback(FF_DIRENT* pDirent, void* context)
{
    FF_DIRENT* dirent_out = (FF_DIRENT*) context;
    DEBUG(1,"           %08x / %04x : %s", pDirent->DirCluster, pDirent->CurrentItem, pDirent->FileName);
    memcpy(dirent_out, pDirent, sizeof(FF_DIRENT));
    return FF_ERR_DIR_END_OF_DIR | FF_GETENTRY;
}
FF_ERROR GetDirentInfo(uint32_t cluster_index, uint16_t item_index, FF_DIRENT* pDirent)
{
    FF_DIRENT dirent;
    memset(&dirent, 0x00, sizeof(dirent));

    if (cluster_index >= num_clusters)
        return PTP_RC_InvalidObjectHandle;
    uint32_t dir_cluster = dirclusters[cluster_index];
    if (dir_cluster == 0x00000000)
        return PTP_RC_InvalidParameter;

    dirent.DirCluster = dir_cluster;
    dirent.CurrentItem = item_index;

    DEBUG(1,"\t\t\t\tinit entry fetch");
    if(FF_isERR(FF_InitEntryFetch(pIoman, dirent.DirCluster, &dirent.FetchContext)))
        return PTP_RC_GeneralError;

    DEBUG(1,"\t\t\t\tfetch dirent");

    FF_ERROR err = FetchDirents(&dirent, GetDirentInfo_Callback, pDirent);

    DEBUG(1,"\t\t\t\tcheck return value");

    if (FF_isERR(err) && FF_GETERROR(err) != FF_ERR_DIR_END_OF_DIR)
        return PTP_RC_GeneralError;

    DEBUG(1,"\t\t\t\tvalidate cluster/index");

    if (pDirent->DirCluster != dir_cluster || pDirent->CurrentItem != item_index)
        return PTP_RC_InvalidObjectHandle;

    return FF_ERR_NONE;
}

static FF_ERROR GetDirentInfoFromName_Callback(FF_DIRENT* pDirent, void* context)
{
    FF_DIRENT* dirent_out = (FF_DIRENT*) context;
    DEBUG(1,"           %08x / %04x : %s", pDirent->DirCluster, pDirent->CurrentItem, pDirent->FileName);
    if (FF_stricmp(dirent_out->FileName, pDirent->FileName))
        return FF_ERR_NONE;
    memcpy(dirent_out, pDirent, sizeof(FF_DIRENT));
    DEBUG(1,"\t\t\t\tfound dirent");
    return FF_ERR_DIR_END_OF_DIR | FF_GETENTRY;
}
FF_ERROR GetDirentInfoFromName(uint32_t parent, const char* filename, FF_DIRENT* pDirent)
{
    FF_DIRENT dirent;
    memset(&dirent, 0x00, sizeof(dirent));

    dirent.DirCluster = FindClusterByHandle(parent);
    dirent.CurrentItem = 0;

    DEBUG(1,"\t\t\t\tfind created entry from dir cluster %08x / %s / %02x", dirent.DirCluster, filename);

    DEBUG(1,"\t\t\t\tinit entry fetch");
    FF_ERROR err = FF_InitEntryFetch(pIoman, dirent.DirCluster, &dirent.FetchContext);
    if(FF_isERR(err))
        return err;

    DEBUG(1,"\t\t\t\tfetch dirent");
    strncpy(pDirent->FileName, filename, sizeof(pDirent->FileName));
    err = FetchDirents(&dirent, GetDirentInfoFromName_Callback, pDirent);

    DEBUG(1,"\t\t\t\tcheck return value");

    if (FF_isERR(err) && FF_GETERROR(err) != FF_ERR_DIR_END_OF_DIR)
        return err;

    if (FF_stricmp(pDirent->FileName, filename))
        return FF_ERR_DIR_END_OF_DIR | FF_GETENTRY;

    return FF_ERR_NONE;
}

static FF_ERROR DeleteDirent_Callback(FF_DIRENT* pDirent, void* context)
{
    if (!strcmp(pDirent->FileName, ".") || !strcmp(pDirent->FileName, ".."))
        return FF_ERR_NONE;

    uint32_t handle = ((GetClusterIndex(pDirent->DirCluster) + 1) << 16) | pDirent->CurrentItem;
    DEBUG(1,"[%08x] %08x / %04x : %s", handle, pDirent->DirCluster, pDirent->CurrentItem, pDirent->FileName);
    FF_ERROR err;
    if (pDirent->Attrib & FF_FAT_ATTR_DIR) {
        AddCluster(pDirent->ObjectCluster, handle);
        DEBUG(1,"\t\trecursing...");
        err = DeleteDirent(handle);
    } else {
        // cat file
        char* path = (char*)context;
        const size_t path_len = strlen(path);
        const size_t name_len = strlen(pDirent->FileName);
        path[path_len] = '/';
        memcpy(&path[path_len+1], pDirent->FileName, name_len+1);
        DEBUG(1,"                           : %s", path);
        err = FF_RmFile(pIoman, path);
        path[path_len] = 0;
    }
    return err;
}
FF_ERROR DeleteDirent(uint32_t handle)
{
    DEBUG(1,"\t\tDeleteDirent %08x", handle);

    FF_DIRENT dirent;

    uint8_t isfile = GetParentCluster(handle) == 0xffff;

    char buffer[FF_MAX_PATH];
    size_t pos = sizeof(buffer) - 1;
    buffer[pos] = 0;

    uint32_t parent = handle;
    do {
        uint16_t cluster_index = (parent >> 16) - 1;
        uint16_t item_index = parent & 0xffff;

        if (FF_isERR(GetDirentInfo(cluster_index, item_index, &dirent)))
            return PTP_RC_InvalidObjectHandle;

        // insert current filename infront of existing name
        size_t len = strlen(dirent.FileName);
        pos -= len;
        memcpy(&buffer[pos], dirent.FileName, len);
        buffer[--pos] = '/';

        parent = assoclink[cluster_index];
    } while(parent);

    memmove(buffer, &buffer[pos], strlen(&buffer[pos])+1);

    if (isfile)
    {
        DEBUG(1,"deleting (file) : %s", buffer);
        return FF_RmFile(pIoman, buffer);
    }

    DEBUG(1,"deleting (dir) : %s", buffer);

    memset(&dirent, 0x00, sizeof(dirent));
    dirent.DirCluster = FindClusterByHandle(handle);
    dirent.CurrentItem = 0;

    DEBUG(1,"\t\t\t\tinit entry fetch");
    FF_ERROR err = FF_InitEntryFetch(pIoman, dirent.DirCluster, &dirent.FetchContext);
    if(FF_isERR(err))
        return err;

    DEBUG(1,"\t\t\t\tfetch dirent");
    err = FetchDirents(&dirent, DeleteDirent_Callback, buffer);
    DEBUG(1,"\t\t\t\terr = %08x", err);
    if (FF_isERR(err) && FF_GETERROR(err) != FF_ERR_DIR_END_OF_DIR)
        return err;

    DEBUG(1,"\t\t\t\trmdir %s", buffer);
    err = FF_RmDir(pIoman, buffer);
    if(FF_isERR(err))
        return err;

    DEBUG(1,"\t\t\t\trm cluster");
    DeleteCluster(handle);

    return err;
}

