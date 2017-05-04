#pragma once

#include "mtp_supported.h"
//#include <string.h>
#include "filesys/fullfat.h"

typedef struct _MtpStorage
{
    MtpStorageId    storage_id;
    char            description[12];
    uint16_t        storage_type;
    uint16_t        filesystem_type;
    uint32_t        max_capacity_mb;
    uint32_t        free_space_mb;
    uint16_t        read_write;
} MtpStorage;

typedef struct _MtpObjectInfo {
    MtpObjectHandle     handle;
    MtpStorageId        storage_id;
    MtpObjectFormat     format;
    uint16_t            protection_status;
    uint32_t            compressed_size;
    MtpObjectFormat     thumb_format;
    uint32_t            thumb_compressed_size;
    uint32_t            thumb_pixWidth;
    uint32_t            thumb_pixHeight;
    uint32_t            image_pixWidth;
    uint32_t            image_pixHeight;
    uint32_t            image_pixDepth;
    MtpObjectHandle     mParent;
    uint16_t            mAssociationType;
    uint32_t            mAssociationDesc;
    uint32_t            mSequenceNumber;
    char*               mName;
    time_t              mDateCreated;
    time_t              mDateModified;
    char*               mKeywords;
} MtpObjectInfo;

void mtp_open_session();
void mtp_close_session();

uint32_t            mtp_get_num_storages();
MtpStorageId        mtp_get_storage_id(uint32_t index);
const MtpStorage*   mtp_get_storage_info(MtpStorageId id);

static __attribute__((unused)) int32_t mtp_get_num_objects(MtpStorageId id, MtpObjectFormat format, MtpObjectHandle parent) { return -1; }
static __attribute__((unused)) int32_t mtp_get_object_handles(MtpStorageId id, MtpObjectFormat format, MtpObjectHandle parent, MtpObjectHandle* handles_out, uint32_t max_handles) { return -1; }

static __attribute__((unused)) void            mtp_get_object_info(MtpObjectHandle handle, MtpObjectInfo* info) { memset(info, 0x00, sizeof(MtpObjectInfo)); }
static __attribute__((unused)) MtpObjectHandle mtp_new_object_info(MtpObjectHandle handle, const MtpObjectInfo* info) { return 0; }

static __attribute__((unused)) const char* mtp_get_object_filepath(MtpObjectHandle handle) { return "?"; }

void mtp_delete_object(MtpObjectHandle handle);

void mtp_start_object_transaction(MtpObjectHandle handle, uint8_t write);   // open file
void mtp_load_object_bytes(uint32_t offset, void* buffer, uint32_t length);
void mtp_store_object_bytes(uint32_t offset, void* buffer, uint32_t length);
void mtp_stop_object_transaction();

void mtp_set_object_property(); // only PTP_OPC_ObjectFileName

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// MTP DATABASE

extern FF_IOMAN *pIoman;

uint16_t GetClusterIndex(uint32_t cluster);
void AddCluster(uint32_t cluster, uint32_t handle);
uint32_t FindClusterByHandle(uint32_t parent);
void DumpClusters();
uint32_t TrackRenames(uint32_t handle);

typedef FF_ERROR (*DirentCallback)(FF_DIRENT* pDirent, void* context);
FF_ERROR FetchDirents(FF_DIRENT* pDirent, DirentCallback callback, void* context);
FF_ERROR GetDirentInfo(uint32_t cluster_index, uint16_t item_index, FF_DIRENT* pDirent);
FF_ERROR DeleteDirent(uint32_t handle);
FF_ERROR GetDirentInfoFromName(uint32_t parent, const char* filename, FF_DIRENT* pDirent);
const char* GetFullPathFromHandle(uint32_t handle, const char* optional_filename, FF_DIRENT* optional_dirent);

extern uint16_t num_clusters;
extern int32_t dirclusters[256];
extern uint32_t assoclink[256];
extern uint16_t num_moved;
extern uint32_t moved[256];
