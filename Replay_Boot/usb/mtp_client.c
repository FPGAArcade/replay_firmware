#include "mtp_client.h"
#include "mtp_supported.h"
#include "ptp_readwrite.h"
#include "ptp_usb.h"
#include "messaging.h"
#include "mtp_database.h"
#include "usb_hardware.h"
#include "hardware.h"

#include "mtp_supported.inl"

#define min(a,b) \
    ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })


static void UsbSendPacket(uint8_t *packet, int len)
{
    if(debuglevel > 2) {
      DumpBuffer(packet, len);
    }
    ptp_send(packet, len);
}


//    mtp_packet_send(ptp_out, PTP_USB_CONTAINER_DATA, ptp_in->code, ptp_in->trans_id, offset);
void mtp_packet_send(PTPUSBBulkContainer* ptp, MtpContainerType type, MtpOperationCode code, MtpTransactionId trans_id, uint32_t data_length)
{
    ptp->length = PTP_USB_BULK_HDR_LEN + data_length;
    ptp->type = type;
    ptp->code = code;
    ptp->trans_id = trans_id;

//    DEBUG(1,"sending packet (size : %08x)", ptp_out->length);
//    DumpBuffer((uint8_t*)ptp_out, ptp_out->length);
    UsbSendPacket((uint8_t*)ptp, ptp->length);
}

static void DumpParams(PTPUSBBulkContainer* ptp)
{
    size_t num_params = (ptp->length - PTP_USB_BULK_HDR_LEN) / sizeof(uint32_t);
    uint32_t* params = &ptp->payload.params.param1;
    for (size_t i = 0; i < min(num_params,5); ++i)
        DEBUG(3,"params%02x : %08x", i, params[i]);
}



typedef struct _MTP_ClientState {
    MtpSessionId    session_id;
} MTP_ClientState;

static MTP_ClientState global_state =
{
    .session_id = MTP_INVALID_ID
};
static MTP_ClientState* state = &global_state;

static PTPUSBBulkContainer temp_ptp;
static PTPUSBBulkContainer* ptp_out = &temp_ptp;

static void SendResponse(PTPUSBBulkContainer* ptp_in, uint16_t response);

typedef uint32_t (*Streamer)(uint8_t* buffer, uint32_t length, void* context);
static Streamer stream_func = 0;
static void* stream_context = 0;



void mtp_packet_recv(uint8_t *packet, int len)
{
    PTPUSBBulkContainer* ptp = (PTPUSBBulkContainer*)(void*)packet;

    static uint32_t remaining = 0;
;;//   DEBUG(1,"mtp_packet_recv ; %08x bytes, %08x remaining from last operation", len, remaining);

    if (remaining) {
        if (stream_func) {
//            DEBUG(1,"\t\t Streaming %08x bytes...", len);
            remaining -= stream_func(packet, len, stream_context);
            if (!stream_func && remaining)
            {
            DEBUG(1,"\t\tstream output closed, but still expecting %d bytes -> zeroing", remaining);
            remaining = 0;
            }

        } else {
            DEBUG(1,"\t\t~NO HANDLER FOR PTP PACKETS > 512 bytes!~ discarding %08x bytes, while remaining was %08x...", len, remaining);
            remaining = 0;
        }
        if (!remaining)
            DEBUG(1,"\t\t Streaming done...");
        return;
    }

    static const char* ContainerTypes[] = {
        "Undefined",
        "Command",
        "Data",
        "Response",
        "Event",
    };

    static const char* SubTypes[] = {
        "PTP", "PTP",
        "Vendor",
        "MTP",
    };

    uint8_t subtype = ((((ptp->code&0xf00) >> 11) | ((ptp->code&0xf000) >> 14)) & 0x3);
    uint8_t has_subtype = ptp->type != PTP_USB_CONTAINER_UNDEFINED && ptp->type != PTP_USB_CONTAINER_DATA;

    DEBUG(1,"PTP_PACKET #%08x : %08x bytes; %s %s; Code : %04x", ptp->trans_id, ptp->length, has_subtype ? SubTypes[subtype] : "", ContainerTypes[ptp->type], ptp->code);

    remaining = ptp->length - len;

    if (remaining)
    {
        DEBUG(1,"\texpecting another %08x bytes..", remaining);
        if (ptp->length < 512)
        {
            DEBUG(1,"\tERROR!");
            DEBUG(1,"\tERROR!");
            DEBUG(1,"\tERROR!");
            DEBUG(1,"\tERROR!");
            DEBUG(1,"\tERROR!");
        }
    }    
/*
    uint8_t hasDataPhase = (operation == PTP_OC_SendObjectInfo ||
                            operation == PTP_OC_SetDevicePropValue ||
                            operation == PTP_OC_MTP_SetObjectReferences ||
                            operation == PTP_OC_MTP_SetObjectPropValue);
*/
    switch(ptp->type)
    {
        case PTP_USB_CONTAINER_DATA:
        case PTP_USB_CONTAINER_COMMAND: {
            const uint32_t num_operations = sizeof(SupportedOperatons) / sizeof(SupportedOperatons[0]);
            for (uint32_t i = 0; i < num_operations; ++i) {
                if (SupportedOperatons[i].code == ptp->code) {
                    DEBUG(1,"\t-> %s", SupportedOperatons[i].name);
                    uint16_t rc = SupportedOperatons[i].func(ptp);
                    if (rc != PTP_RC_Undefined) {
                        SendResponse(ptp, rc);
//                        if (ptp->code == PTP_OC_GetObjectInfo)
//	                        SendMovedFiles();
                    }
                    return;
                }
            }
            SendResponse(ptp, PTP_RC_Undefined);
            break;
        }
        default:
            DEBUG(1,"\t\t~UNKNOWN TYPE ERROR!~");
            break;
    }
}

static void SendResponse(PTPUSBBulkContainer* ptp_in, uint16_t response)
{
    ptp_out->length = PTP_USB_BULK_HDR_LEN;
    ptp_out->type = PTP_USB_CONTAINER_RESPONSE;
    ptp_out->code = response;
    ptp_out->trans_id = ptp_in->trans_id;

    if (response != PTP_RC_OK)
        DEBUG(1,"FAILED! %04x", response);

    UsbSendPacket((uint8_t*)ptp_out, ptp_out->length);
}


static uint16_t GetDeviceInfo(PTPUSBBulkContainer* ptp_in)
{
    /*
        Operation Code          0x1001
        Operation Parameter 1   None
        Operation Parameter 2   None
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    DeviceInfo dataset
        Data Direction          R->I
        ResponseCode Options    OK, Parameter_Not_Supported
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None

        Dataset field       Field order Size (bytes)Datatype
        ----------------------------------------------------
        Standard Version            1   2           UINT16
        MTP Vendor Extension ID     2   4           UINT32
        MTP Version                 3   2           UINT16
        MTP Extensions              4   Variable    String
        Functional Mode             5   2           UINT16
        Operations Supported        6   Variable    Operation Code Array
        Events Supported            7   Variable    Event Code Array
        Device Properties Supported 8   Variable    Device Property Code Array
        Capture Formats             9   Variable    Object Format Code Array
        Playback Formats            10  Variable    Object Format Code Array
        Manufacturer                11  Variable    String
        Model                       12  Variable    String
        Device Version              13  Variable    String
        Serial Number               14  Variable    String    
    */
    size_t offset = 0;
    const uint16_t mtp_version = 100; // 'For MTP devices implemented under this specification, this shall contain the value 100 (representing 1.00).'
    const char* mtp_extensions = "microsoft.com: 1.0; android.com: 1.0;";
    const char* manufacturer = "FPGAArcade";
    const char* model = "Replay";
    const char* device_version = "1.0";
    const char* serial_number = "0123456789abcdef";
    offset = Write16bits(ptp_out, offset, mtp_version);                 // Standard Version
    offset = Write32bits(ptp_out, offset, PTP_VENDOR_MICROSOFT);        // MTP Vendor Extension ID (or '0' for PTP mode)
    offset = Write16bits(ptp_out, offset, mtp_version);                 // Standard Version
    offset = WriteString(ptp_out, offset, mtp_extensions);              // MTP Extensions (empty string for PTP mode)
    offset = Write16bits(ptp_out, offset, 0);                           // Functional Mode

    const uint32_t num_operations = sizeof(SupportedOperatons) / sizeof(SupportedOperatons[0]);
    offset = Write32bits(ptp_out, offset, num_operations);              // Operations Supported
    for (uint32_t i = 0; i < num_operations; ++i)
        offset = Write16bits(ptp_out, offset, SupportedOperatons[i].code);

    const uint32_t num_events = sizeof(SupportedEvents) / sizeof(SupportedEvents[0]);
    offset = Write32bits(ptp_out, offset, num_events);                  // Events Supported
    for (uint32_t i = 0; i < num_events; ++i)
        offset = Write16bits(ptp_out, offset, SupportedEvents[i]);

    const uint32_t num_props = sizeof(SupportedDeviceProps) / sizeof(SupportedDeviceProps[0]);
    offset = Write32bits(ptp_out, offset, num_props);                   // Device Properties Supported
    for (uint32_t i = 0; i < num_props; ++i)
        offset = Write16bits(ptp_out, offset, SupportedDeviceProps[i]);

    offset = Write32bits(ptp_out, offset, 0);                           // Capture Formats

    const uint32_t num_playback = sizeof(SupportedPlaybacks) / sizeof(SupportedPlaybacks[0]);
    offset = Write32bits(ptp_out, offset, num_playback);                // Playback Formats
    for (uint32_t i = 0; i < num_playback; ++i)
        offset = Write16bits(ptp_out, offset, SupportedPlaybacks[i]);

    offset = WriteString(ptp_out, offset, manufacturer);                // Manufacturer
    offset = WriteString(ptp_out, offset, model);                       // Model
    offset = WriteString(ptp_out, offset, device_version);              // Device Version
    offset = WriteString(ptp_out, offset, serial_number);               // Serial Number

    ptp_out->length = PTP_USB_BULK_HDR_LEN + offset;
    ptp_out->type = PTP_USB_CONTAINER_DATA;
    ptp_out->code = ptp_in->code;
    ptp_out->trans_id = ptp_in->trans_id;

    UsbSendPacket((uint8_t*)ptp_out, ptp_out->length);
    return PTP_RC_OK;
}

static uint16_t OpenSession(PTPUSBBulkContainer* ptp_in)
{
    /*
        Operation Code          0x1002
        Operation Parameter 1   SessionID
        Operation Parameter 2   None
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    None
        Data Direction          N/A
        ResponseCode Options    OK, Parameter_Not_Supported, Invalid_Parameter, Session_Already_Open, Device_Busy
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None
    */

    if (state->session_id != MTP_INVALID_ID) {
        ptp_out->payload.params.param1 = state->session_id;
        return PTP_RC_SessionAlreadyOpened;
    }
    state->session_id = ptp_in->payload.params.param1;

    mtp_open_session();

    return PTP_RC_OK;
}

static uint16_t CloseSession(PTPUSBBulkContainer* ptp_in)
{
    /*
        Operation Code          0x1003
        Operation Parameter 1   None
        Operation Parameter 2   None
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    None
        Data Direction          N/A
        ResponseCode Options    OK, Session_Not_Open, Invalid_TransactionID, Parameter_Not_Supported
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None
    */
    if (state->session_id == MTP_INVALID_ID)
        return PTP_RC_SessionNotOpen;

    state->session_id = MTP_INVALID_ID;

    mtp_close_session();

    return PTP_RC_OK;
}

static uint16_t GetStorageIDs(PTPUSBBulkContainer* ptp_in)
{
    /*
        Operation Code          0x1004
        Operation Parameter 1   None
        Operation Parameter 2   None
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    StorageID array
        Data Direction          R->I
        ResponseCode Options    OK, Operation_Not_Supported, Session_Not_Open, Invalid_TransactionID, Parameter_Not_Supported
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None
    */

    if (state->session_id == MTP_INVALID_ID)
        return PTP_RC_SessionNotOpen;

    size_t offset = 0;

    uint32_t count = mtp_get_num_storages();
    offset = Write32bits(ptp_out, offset, count);                           // number of storage IDs
    for (int i = 0; i < count; i++)
        offset = Write32bits(ptp_out, offset, mtp_get_storage_id(i));       // Storage IDs

    ptp_out->length = PTP_USB_BULK_HDR_LEN + offset;
    ptp_out->type = PTP_USB_CONTAINER_DATA;
    ptp_out->code = ptp_in->code;
    ptp_out->trans_id = ptp_in->trans_id;

//    DEBUG(1,"sending packet (size : %08x)", ptp_out->length);
//    DumpBuffer((uint8_t*)ptp_out, ptp_out->length);
    UsbSendPacket((uint8_t*)ptp_out, ptp_out->length);
    return PTP_RC_OK;
}

static uint16_t GetStorageInfo(PTPUSBBulkContainer* ptp_in)
{
    /*
        Operation Code          0x1005
        Operation Parameter 1   StorageID
        Operation Parameter 2   None
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    StorageInfo dataset
        Data Direction          R->I
        ResponseCode Options    OK, Session_Not_Open, Invalid_TransactionID, Access_Denied, Invalid_StorageID, Store_Not_Available, Parameter_Not_Supported
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None

        Dataset field       Field order Size (bytes)Datatype
        ----------------------------------------------------
        Storage Type                1   2           UINT16
        Filesystem Type             2   2           UINT16
        Access Capability           3   2           UINT16
        Max Capacity                4   8           UINT64
        Free Space In Bytes         5   8           UINT64
        *Free Space In Objects      6   4           UINT32
        Storage Description         7   Variable    String
        Volume Identifier           8   Variable    String
    */

    if (state->session_id == MTP_INVALID_ID)
        return PTP_RC_SessionNotOpen;

    DumpParams(ptp_in);

    MtpStorageId id = ptp_in->payload.params.param1;
    const MtpStorage* storage = mtp_get_storage_info(id);
    if (!storage && 0)
        return PTP_RC_InvalidStorageId;

    DEBUG(1,"VolumeID : '%s'", storage->description);

    const uint32_t capacity = storage->max_capacity_mb;
    const uint32_t freesize = storage->free_space_mb;

    size_t offset = 0;
    offset = Write16bits(ptp_out, offset, storage->storage_type);
    offset = Write16bits(ptp_out, offset, storage->filesystem_type);
    offset = Write16bits(ptp_out, offset, storage->read_write);
    offset = Write64bits(ptp_out, offset, capacity >> (32-20), capacity << 20);     // max capacity converted to bytes
    offset = Write64bits(ptp_out, offset, freesize >> (32-20), freesize << 20);     // free space (bytes)
    offset = Write32bits(ptp_out, offset, 1024*1024*1024);                          // free space (objects)
    offset = WriteString(ptp_out, offset, storage->description);
    offset = WriteString(ptp_out, offset, "");

    ptp_out->length = PTP_USB_BULK_HDR_LEN + offset;
    ptp_out->type = PTP_USB_CONTAINER_DATA;
    ptp_out->code = ptp_in->code;
    ptp_out->trans_id = ptp_in->trans_id;

//    DEBUG(1,"sending packet (size : %08x)", ptp_out->length);
//    DumpBuffer((uint8_t*)ptp_out, ptp_out->length);
    UsbSendPacket((uint8_t*)ptp_out, ptp_out->length);
    return PTP_RC_OK;
}

static uint16_t GetNumObjects(PTPUSBBulkContainer* ptp_in)
{
    /*
        Operation Code          0x1006
        Operation Parameter 1   StorageID
        Operation Parameter 2   [ObjectFormatCode]
        Operation Parameter 3   [ObjectHandle of Association for which number of children is needed]
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    None
        Data Direction          N/A
        ResponseCode Options    OK, Operation_Not_Supported, Session_Not_Open, Invalid_TransactionID, Invalid_StorageID, Store_Not_Available, Specification_By_Format_Unsupported, Invalid_Code_Format, Parameter_Not_Supported, Invalid_ParentObject, Invalid_ObjectHandle, Invalid_Parameter
        Response Parameter 1    NumObjects
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None
    */

    if (state->session_id == MTP_INVALID_ID)
        return PTP_RC_SessionNotOpen;

    MtpStorageId storage_id = ptp_in->payload.params.param1;      // 0xFFFFFFFF for all storage
    MtpObjectFormat format = ptp_in->payload.params.param2;      // 0 for all formats
    MtpObjectHandle parent = ptp_in->payload.params.param3;      // 0xFFFFFFFF for objects with no parent
                                                                  // 0x00000000 for all objects

    if (!mtp_get_storage_info(storage_id))
        return PTP_RC_InvalidStorageId;

    int count = mtp_get_num_objects(storage_id, format, parent);

    if (count >= 0) {
        ptp_out->payload.params.param1 = count;
        return PTP_RC_OK;
    } else {
        ptp_out->payload.params.param1 = 0;
        return PTP_RC_InvalidObjectHandle;
    }

}


typedef struct _GetObjectHandles_Context {
    uint16_t payload_offset;
    uint32_t num_handles;
} GetObjectHandles_Context;

static FF_ERROR GetObjectHandles_Callback(FF_DIRENT* pDirent, void* context)
{
    GetObjectHandles_Context* ctx = (GetObjectHandles_Context*)context;

    if (!strcmp(pDirent->FileName, ".") || !strcmp(pDirent->FileName, ".."))
        return FF_ERR_NONE;

    // cluster index is +1 since it would otherwise cause the handle to result in 0x00000000 which is illegal (== root parent)
//    if (pDirent->Attrib & FF_FAT_ATTR_DIR)
//      return FF_ERR_NONE;
    uint32_t handle = ((GetClusterIndex(pDirent->DirCluster) + 1) << 16) | pDirent->CurrentItem;
    DEBUG(1,"[%08x] %08x / %04x : %s", handle, pDirent->DirCluster, pDirent->CurrentItem, pDirent->FileName);
    ctx->payload_offset = Write32bits(ptp_out, ctx->payload_offset, handle);
    ++ctx->num_handles;

    if (pDirent->Attrib & FF_FAT_ATTR_DIR)
        AddCluster(pDirent->ObjectCluster, handle);

    return FF_ERR_NONE;
}
static uint16_t GetObjectHandles(PTPUSBBulkContainer* ptp_in)
{
    /*
        Operation Code          0x1007
        Operation Parameter 1   StorageID
        Operation Parameter 2   [ObjectFormatCode]
        Operation Parameter 3   [ObjectHandle of Association or hierarchical folder for which a list of children is needed]
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    ObjectHandle array
        Data Direction          R->I
        ResponseCode Options    OK, Operation_Not_Supported, Session_Not_Open, Invalid_TransactionID, Invalid_StorageID, Store_Not_Available, Invalid_ObjectFormatCode, Specification_By_Format_Unsupported, Invalid_Code_Format, Invalid_ObjectHandle, Invalid_Parameter, Parameter_Not_Supported, Invalid_ParentObject, Invalid_ObjectHandle
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None
    */

    GetObjectHandles_Context ctx;
    DumpParams(ptp_in);

    uint32_t storage = ptp_in->payload.params.param1;
    uint32_t format = ptp_in->payload.params.param2;
    uint32_t parent = ptp_in->payload.params.param3;

    MtpObjectHandle* handles = (MtpObjectHandle*)(void*)ptp_out->payload.data;
    uint32_t max_handles = sizeof(ptp_out->payload.data) / sizeof(MtpObjectHandle);
    int32_t num_handles = mtp_get_object_handles(storage, format, parent, handles, max_handles);
    (void)num_handles;

    DEBUG(1,"\t\t\t\tcheck storage");

    if (storage != 0xffffffff && storage != 0x00010001)
        return PTP_RC_InvalidStorageId;

    DEBUG(1,"\t\t\t\tcheck num_clusters");

    FF_DIRENT dirent;
    memset(&dirent, 0, sizeof(FF_DIRENT));

    dirent.DirCluster = FindClusterByHandle(parent);
    if (!dirent.DirCluster)
        return PTP_RC_InvalidParentObject;

    ctx.payload_offset = 0;
    ctx.num_handles = 0;
    ctx.payload_offset = Write32bits(ptp_out, ctx.payload_offset, ctx.num_handles);        // number of object handles

    DEBUG(1,"\t\t\t\tinit entry fetch");
    if (FF_isERR(FF_InitEntryFetch(pIoman, dirent.DirCluster, &dirent.FetchContext)))
        return PTP_RC_GeneralError;

    DEBUG(1,"\t\t\t\tfetch dirents");
    FF_DIRENT* pDirent = &dirent;
    FetchDirents(pDirent, GetObjectHandles_Callback, &ctx);

    DumpClusters();
//    DEBUG(1,"\t\t\t\tnum_clusters : %d", num_clusters);
//    for (int i = 0; i < num_clusters; ++i)
//    {
//        DEBUG(1,"\t\t\t\t%04x ; dir = %08x ; parent = %08x", i, dirclusters[i], assoclink[i]);
//    }

    Write32bits(ptp_out, 0, ctx.num_handles);        // number of object handles

    ptp_out->length = PTP_USB_BULK_HDR_LEN + ctx.payload_offset;
    ptp_out->type = PTP_USB_CONTAINER_DATA;
    ptp_out->code = ptp_in->code;
    ptp_out->trans_id = ptp_in->trans_id;

    UsbSendPacket((uint8_t*)ptp_out, ptp_out->length);

    return PTP_RC_OK;
}

static FF_ERROR GetObjectInfo_Callback(FF_DIRENT* pDirent, void* context)
{
    FF_DIRENT* dirent_out = (FF_DIRENT*) context;
    DEBUG(1,"           %08x / %04x : %s", pDirent->DirCluster, pDirent->CurrentItem, pDirent->FileName);
    memcpy(dirent_out, pDirent, sizeof(FF_DIRENT));
    return FF_ERR_DIR_END_OF_DIR | FF_GETENTRY;
}
static uint16_t GetObjectInfo(PTPUSBBulkContainer* ptp_in)
{
    /*
        Operation Code          0x1008
        Operation Parameter 1   ObjectHandle
        Operation Parameter 2   None
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    ObjectInfo dataset
        Data Direction          R->I
        ResponseCode Options    OK, Operation_Not_Supported, Session_Not_Open, Invalid_TransactionID, Invalid_ObjectHandle, Store_Not_Available, Parameter_Not_Supported
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None

        Dataset field       Field order Size (bytes)Datatype         Datatype Required for SendObjectInfo
        -------------------------------------------------------------------------------------------------
        StorageID               1   4               StorageID           No
        Object Format           2   2               ObjectFormatCode    Yes
        Protection Status       3   2               UINT16              No
        Object Compressed Size  4   4               UINT32              Yes
        *Thumb Format           5   2               ObjectFormatCode    No
        *Thumb Compressed Size  6   4               UINT32              No
        *Thumb Pix Width        7   4               UINT32              No
        *Thumb Pix Height       8   4               UINT32              No
        Image Pix Width         9   4               UINT32              No
        Image Pix Height        10  4               UINT32              No
        Image Bit Depth         11  4               UINT32              No
        Parent Object           12  4               ObjectHandle        No
        Association Type        13  2               Association Code    Yes
        Association Description 14  4               AssociationDesc     Yes
        *Sequence Number        15  4               UINT32              No
        Filename                16  Variable        String              Yes
        Date Created            17  Variable        DateTime String     No
        Date Modified           18  Variable        DateTime String     No
        Keywords                19  Variable        String              No
    */

    DumpParams(ptp_in);

    uint32_t handle = ptp_in->payload.params.param1;

    MtpObjectInfo info;
    mtp_get_object_info(handle, &info);

    handle = TrackRenames(handle);

    uint16_t cluster_index = (handle >> 16) - 1;
    uint16_t item_index = handle & 0xffff;

    DEBUG(2,"\t\t\t\tcheck cluster index");
    if (cluster_index >= num_clusters)
        return PTP_RC_InvalidObjectHandle;
    DEBUG(1,"\t\t\t\tcheck dir_cluster");
    uint32_t dir_cluster = dirclusters[cluster_index];
    if (dir_cluster == 0x00000000)
        return PTP_RC_InvalidParameter;

    FF_DIRENT dirent;
    memset(&dirent, 0, sizeof(dirent));


    DEBUG(2,"\t\t\t\tcluster %08x index %04x", dir_cluster, item_index);

    dirent.DirCluster = dir_cluster;
    dirent.CurrentItem = item_index;

  FF_ERROR  err;
//  FF_T_UINT8  numLFNs;


    FF_DIRENT* pDirent = &dirent;

    DEBUG(2,"\t\t\t\tinit entry fetch");
    if(FF_isERR(FF_InitEntryFetch(pIoman, pDirent->DirCluster, &pDirent->FetchContext)))
        return PTP_RC_GeneralError;

    DEBUG(3,"\t\t\t\tfetch dirent");

    FF_DIRENT dirent_out;
    err = FetchDirents(pDirent, GetObjectInfo_Callback, &dirent_out);

    DEBUG(3,"\t\t\t\tcheck return value");

    if (FF_isERR(err) && FF_GETERROR (err) != FF_ERR_DIR_END_OF_DIR)
        return PTP_RC_GeneralError;

    DEBUG(2,"\t\t\t\tvalidate cluster/index");

    if (dirent_out.DirCluster != dir_cluster || dirent_out.CurrentItem != item_index)
        return PTP_RC_InvalidObjectHandle;


    DEBUG(1,"\t\t\t\ta-ok - writing info");

    uint8_t isdir = dirent_out.Attrib & FF_FAT_ATTR_DIR;

    uint32_t storage = 0x00010001;
    uint16_t object_format = isdir ? PTP_OFC_Association : PTP_OFC_Undefined;
    uint16_t protection = PTP_PS_NoProtection;
    uint32_t filesize = dirent_out.Filesize;
    uint32_t parent = assoclink[cluster_index];
    uint16_t assoc_code = isdir ? 0x0001 : 0x0000;
    uint32_t assoc_desc = 0x00000001;   // if 0x00000001 supports "object references"

    const char* filename = dirent_out.FileName;
    const char* created = "19700101T010101.5";
    const char* modified = "19700101T010101.5";

    DEBUG(2,"\t storage       = %08x", storage);
    DEBUG(2,"\t object_format = %04x", object_format);
    DEBUG(2,"\t protection    = %04x", protection);
    DEBUG(2,"\t filesize      = %08x", filesize);
    DEBUG(2,"\t parent        = %08x", parent);
    DEBUG(2,"\t assoc_code    = %04x", assoc_code);
    DEBUG(2,"\t assoc_desc    = %08x", assoc_desc);
    DEBUG(2,"\t filename      = %s", filename);
    DEBUG(2,"\t created       = %s", created);
    DEBUG(2,"\t modified      = %s", modified);

    size_t offset = 0;
    offset = Write32bits(ptp_out, offset, storage);
    offset = Write16bits(ptp_out, offset, object_format);    
    offset = Write16bits(ptp_out, offset, protection);    
    offset = Write32bits(ptp_out, offset, filesize);

    offset = Write16bits(ptp_out, offset, 0);   // thumb
    offset = Write32bits(ptp_out, offset, 0);   // thumb
    offset = Write32bits(ptp_out, offset, 0);   // thumb
    offset = Write32bits(ptp_out, offset, 0);   // thumb

    offset = Write32bits(ptp_out, offset, 0);   // image
    offset = Write32bits(ptp_out, offset, 0);   // image
    offset = Write32bits(ptp_out, offset, 0);   // image

    offset = Write32bits(ptp_out, offset, parent);
    offset = Write16bits(ptp_out, offset, assoc_code);
    offset = Write32bits(ptp_out, offset, assoc_desc);

    offset = Write32bits(ptp_out, offset, 0);   // sequence

    offset = WriteString(ptp_out, offset, filename);
    offset = WriteString(ptp_out, offset, created);    // "YYYYMMDDThhmmss.s"
    offset = WriteString(ptp_out, offset, modified);    // "YYYYMMDDThhmmss.s"
    offset = WriteString(ptp_out, offset, "");      // keywords


    ptp_out->length = PTP_USB_BULK_HDR_LEN + offset;
    ptp_out->type = PTP_USB_CONTAINER_DATA;
    ptp_out->code = ptp_in->code;
    ptp_out->trans_id = ptp_in->trans_id;

    UsbSendPacket((uint8_t*)ptp_out, ptp_out->length);

    return PTP_RC_OK;
}

static uint16_t GetObject(PTPUSBBulkContainer* ptp_in) 
{
    /*
        Operation Code          0x1009
        Operation Parameter 1   ObjectHandle
        Operation Parameter 2   None
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    Object Binary Data
        Data Direction          R->I
        ResponseCode Options    OK, Operation_Not_Supported, Session_Not_Open, Invalid_TransactionID, Invalid_ObjectHandle, Invalid_Parameter, Store_Not_Available, Incomplete_Transfer, Access_Denied, Parameter_Not_Supported
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None    
    */

    DumpParams(ptp_in);

    uint32_t handle = ptp_in->payload.params.param1;

    // create full path

    char buffer[FF_MAX_PATH];
    size_t pos = sizeof(buffer) - 1;
    buffer[pos] = 0;
    uint32_t filesize = 0;

    do {
        uint16_t cluster_index = (handle >> 16) - 1;
        uint16_t item_index = handle & 0xffff;

        FF_DIRENT dirent;
        if (FF_isERR(GetDirentInfo(cluster_index, item_index, &dirent)))
            return PTP_RC_InvalidObjectHandle;

        // insert current filename infront of existing name
        size_t len = strlen(dirent.FileName);
        pos -= len;
        memcpy(&buffer[pos], dirent.FileName, len);
        buffer[--pos] = '/';

        filesize += dirent.Filesize;

        handle = assoclink[cluster_index];
    } while(handle);

    const char* path = &buffer[pos];

    DEBUG(1,"\t\t\t\tFULL PATH = %s", path);

    FF_FILE* file = FF_Open(pIoman, path, FF_MODE_READ, NULL);
    if (!file)
        return PTP_RC_GeneralError;

// ---------------------

    int32_t bytes_read = 0;
    size_t first_packet_len = 64 - PTP_USB_BULK_HDR_LEN;
    bytes_read = FF_Read(file, first_packet_len, 1, ptp_out->payload.data);

    ptp_out->length = PTP_USB_BULK_HDR_LEN + filesize;
    ptp_out->type = PTP_USB_CONTAINER_DATA;
    ptp_out->code = ptp_in->code;
    ptp_out->trans_id = ptp_in->trans_id;

    int len = PTP_USB_BULK_HDR_LEN + bytes_read;
    uint8_t *packet = (uint8_t*)ptp_out;
    DEBUG(1,"\t\t\t\tsending first packet; size = %04x", len);
    do {
        int wLength = len == 64 ? 64 : 0;   // set wLength to the wMaxPacketSize to prevent zlp except on the last one
        usb_send(2, 64, packet, len, wLength);
        bytes_read = FF_Read(file, 64, 1, (FF_T_UINT8*)buffer);
        packet = (uint8_t*)buffer;
        len = bytes_read;
        DEBUG(1,"\t\t\t\tsending next packet; size = %04x", len);
    } while(bytes_read);

#if 0
    int32_t bytes_read = 0;
    size_t first_packet_len = 64 - PTP_USB_BULK_HDR_LEN;
    bytes_read = FF_Read(file, first_packet_len, 1, ptp_out->payload.data);

/*
"NB! change to actual file length here!!!"
"    if that doesn't work - change to split header/data packets"!!

" also - sending mtp data packets that align with 512 seems to need another zlp, perhaps outside of the regular USB zlp"
" no probably not - this is just a buggy workaround in mtp.. without split headers and a absolute length of 1024 we will hit that path - not good"
*/
    ptp_out->length = PTP_USB_BULK_HDR_LEN + filesize;
    ptp_out->type = PTP_USB_CONTAINER_DATA;
    ptp_out->code = ptp_in->code;
    ptp_out->trans_id = ptp_in->trans_id;

    // in-line send routine!
        uint8_t *packet = (uint8_t*)ptp_out;
        int len = PTP_USB_BULK_HDR_LEN + bytes_read;
        DEBUG(1,"\t\t\t\tsending first packet; size = %04x", len);

        for(int i = 0; i < len; i++) {
            UDP_ENDPOINT_FIFO(2) = packet[i];
        }

        DEBUG(1,"\t\t\t\tfirst packet; size = %04x", len);
        UDP_ENDPOINT_CSR(2) |= UDP_CSR_TX_PACKET;


        if (len < 64)
            goto done;

        do {
            bytes_read = FF_Read(file, 64, 1, (FF_T_UINT8*)buffer);

//            DEBUG(1,"\t\t\t\tsending next packet; size = %04x", bytes_read);

            packet = (uint8_t*)buffer;
            len = bytes_read;

            for(int i = 0; i < len; i++) {
                UDP_ENDPOINT_FIFO(2) = packet[i];
            }

            while(!(UDP_ENDPOINT_CSR(2) & UDP_CSR_TX_PACKET_ACKED))
                ;
            UDP_ENDPOINT_CSR(2) &= ~UDP_CSR_TX_PACKET_ACKED;
            while(UDP_ENDPOINT_CSR(2) & UDP_CSR_TX_PACKET_ACKED)
                ;

            UDP_ENDPOINT_CSR(2) |= UDP_CSR_TX_PACKET;

        } while(bytes_read);
done:

        while(!(UDP_ENDPOINT_CSR(2) & UDP_CSR_TX_PACKET_ACKED))
            ;
        UDP_ENDPOINT_CSR(2) &= ~UDP_CSR_TX_PACKET_ACKED;
        while(UDP_ENDPOINT_CSR(2) & UDP_CSR_TX_PACKET_ACKED)
            ;
#endif
//---------------------------------------------------------


    DEBUG(1,"\t\t\t\twe're done");
    FF_Close(file);
    return PTP_RC_OK;
}

static uint16_t GetThumb(PTPUSBBulkContainer* ptp)
{
    /*
        Operation Code          0x100A
        Operation Parameter 1   ObjectHandle
        Operation Parameter 2   None
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    ThumbnailData
        Data Direction          R->I
        ResponseCode Options    OK, Operation_Not_Supported, Session_Not_Open, Invalid_TransactionID, Invalid_ObjectHandle, Thumbnail_Not_Present, Invalid_ObjectFormatCode, Store_Not_Available, Parameter_Not_Supported
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None
    */

    // MtpObjectHandle handle = ptp_in->payload.params.param1;
    // filename = mtp_get_object_filepath(handle);
    // jpg_file_open(filename)
    // jpg_find_section(EXIF)
    // send_object(exif_section)

    return PTP_RC_GeneralError;
}

static uint16_t DeleteObject(PTPUSBBulkContainer* ptp_in)
{
    /*
        Operation Code          0x100B
        Operation Parameter 1   ObjectHandle
        Operation Parameter 2   [ObjectFormatCode]
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    None
        Data Direction          N/A
        ResponseCode Options    OK, Operation_Not_Supported, Session_Not_Open, Invalid_TransactionID, Invalid_ObjectHandle, Object_WriteProtected, Store_Read_Only, Partial_Deletion, Store_Not_Available, Specification_By_Format_Unsupported, Invalid_Code_Format, Device_Busy, Parameter_Not_Supported, Access_Denied
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None
    */

    DumpParams(ptp_in);

    uint32_t handle = ptp_in->payload.params.param1;

    DumpClusters();
//    DEBUG(1,"\t\t\t\tnum_clusters : %d", num_clusters);
//    for (int i = 0; i < num_clusters; ++i)
//    {
//        DEBUG(1,"\t\t\t\t%04x ; dir = %08x ; parent = %08x", i, dirclusters[i], assoclink[i]);
//    }

    FF_ERROR err = DeleteDirent(handle);

    return FF_isERR(err) ? PTP_RC_StoreReadOnly : PTP_RC_OK;
}

static uint32_t send_object_handle = 0;
static uint32_t send_object_filesize = 0;
static uint16_t SendObjectInfo(PTPUSBBulkContainer* ptp_in) 
{
    /*
        Operation Code          0x100C
        Operation Parameter 1   [Destination StorageID on responder]
        Operation Parameter 2   [Parent ObjectHandle on responder where object shall be placed]
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    ObjectInfo dataset
        Data Direction          I->R
        ResponseCode Options    OK, Operation_Not_Supported, Session_Not_Open, Invalid_TransactionID, Access_Denied, Invalid_StorageID, Store_Read_Only, Object_Too_Large, Store_Full, Invalid_ObjectFormatCode, Store_Not_Available, Parameter_Not_Supported, Invalid_ParentObject, Invalid_Dataset, Specification_Of_Destination_Unsupported
        Response Parameter 1    Responder StorageID in which the object will be stored
        Response Parameter 2    Responder parent ObjectHandle in which the object will be stored
        Response Parameter 3    Response Parameter3: responder’s reserved ObjectHandle for the incoming object
        Response Parameter 4    None
        Response Parameter 5    None

        Dataset field       Field order Size (bytes)Datatype         Datatype Required for SendObjectInfo
        -------------------------------------------------------------------------------------------------
        StorageID               1   4               StorageID           No
        Object Format           2   2               ObjectFormatCode    Yes
        Protection Status       3   2               UINT16              No
        Object Compressed Size  4   4               UINT32              Yes
        *Thumb Format           5   2               ObjectFormatCode    No
        *Thumb Compressed Size  6   4               UINT32              No
        *Thumb Pix Width        7   4               UINT32              No
        *Thumb Pix Height       8   4               UINT32              No
        Image Pix Width         9   4               UINT32              No
        Image Pix Height        10  4               UINT32              No
        Image Bit Depth         11  4               UINT32              No
        Parent Object           12  4               ObjectHandle        No
        Association Type        13  2               Association Code    Yes
        Association Description 14  4               AssociationDesc     Yes
        *Sequence Number        15  4               UINT32              No
        Filename                16  Variable        String              Yes
        Date Created            17  Variable        DateTime String     No
        Date Modified           18  Variable        DateTime String     No
        Keywords                19  Variable        String              No
    */

    DumpParams(ptp_in);

    static uint32_t storage;
    static uint32_t parent;

    // we expect data (== the next packet)
    if (ptp_in->type == PTP_USB_CONTAINER_COMMAND)
    {
        storage = ptp_in->payload.params.param1;
        parent = ptp_in->payload.params.param2;
        return PTP_RC_Undefined;
    }

    uint32_t storage_dummy;
    uint16_t object_format = 0;
    uint16_t protection;
    uint32_t filesize;
    uint32_t parent_dummy;
    uint16_t assoc_code;
    uint32_t assoc_desc;

    char filename[FF_MAX_FILENAME];
    char created[20];
    char modified[20];

    size_t offset = 0;
    offset = Read32bits(ptp_in, offset, &storage_dummy);
    offset = Read16bits(ptp_in, offset, &object_format);    
    offset = Read16bits(ptp_in, offset, &protection);    
    offset = Read32bits(ptp_in, offset, &filesize);

    offset = Read16bits(ptp_in, offset, 0);   // thumb
    offset = Read32bits(ptp_in, offset, 0);   // thumb
    offset = Read32bits(ptp_in, offset, 0);   // thumb
    offset = Read32bits(ptp_in, offset, 0);   // thumb

    offset = Read32bits(ptp_in, offset, 0);   // image
    offset = Read32bits(ptp_in, offset, 0);   // image
    offset = Read32bits(ptp_in, offset, 0);   // image

    offset = Read32bits(ptp_in, offset, &parent_dummy);
    offset = Read16bits(ptp_in, offset, &assoc_code);
    offset = Read32bits(ptp_in, offset, &assoc_desc);

    offset = Read32bits(ptp_in, offset, 0);   // sequence

    offset = ReadString(ptp_in, offset, filename);
    offset = ReadString(ptp_in, offset, created);    // "YYYYMMDDThhmmss.s"
    offset = ReadString(ptp_in, offset, modified);    // "YYYYMMDDThhmmss.s"
    offset = ReadString(ptp_in, offset, 0);      // keywords

    DEBUG(1,"\t storage       = %08x", storage);
    DEBUG(1,"\t object_format = %04x", object_format);
    DEBUG(1,"\t protection    = %04x", protection);
    DEBUG(1,"\t filesize      = %08x", filesize);
    DEBUG(1,"\t parent        = %08x", parent);
    DEBUG(1,"\t assoc_code    = %04x", assoc_code);
    DEBUG(1,"\t assoc_desc    = %08x", assoc_desc);
    DEBUG(1,"\t filename      = %s", filename);
    DEBUG(1,"\t created       = %s", created);
    DEBUG(1,"\t modified      = %s", modified);

    DEBUG(1,"\t\t\t\tCreate full path");

    char buffer[FF_MAX_PATH];
    size_t pos = sizeof(buffer) - 1;
    buffer[pos] = 0;

    size_t len = strlen(filename);
    pos -= len;
    memcpy(&buffer[pos], filename, len);
    buffer[--pos] = '/';

    if (parent == 0xffffffff)
        parent = 0x00000000;

    {
        uint32_t handle = parent;

        while(handle) {
            uint16_t cluster_index = (handle >> 16) - 1;
            uint16_t item_index = handle & 0xffff;

            FF_DIRENT dirent;
            if (FF_isERR(GetDirentInfo(cluster_index, item_index, &dirent)))
                return PTP_RC_InvalidObjectHandle;

            // insert current filename infront of existing name
            len = strlen(dirent.FileName);
            pos -= len;
            memcpy(&buffer[pos], dirent.FileName, len);
            buffer[--pos] = '/';

            handle = assoclink[cluster_index];
        };
    }

    const char* path = &buffer[pos];

    DEBUG(1,"\t\t\t\tFULL PATH = %s", path);
    uint8_t attrib = 0;

    if (object_format == PTP_OFC_Association) {
        attrib = FF_FAT_ATTR_DIR;
        FF_ERROR err = FF_MkDir(pIoman, path);
        DEBUG(1,"\t\t\t\tFF_MkDir returned %08x", err);
    } else {
        FF_ERROR err;
        FF_FILE* file = FF_Open(pIoman, path, FF_MODE_WRITE | FF_MODE_CREATE | FF_MODE_TRUNCATE, &err);
        if (file) {
            FF_Close(file);
        }
        DEBUG(1,"\t\t\t\tFF_Open returned %08x", err);
    }

    DEBUG(1,"\t\t\t\tcreated %s with attrib = %02x", path, attrib);

    FF_DIRENT dirent;
    FF_ERROR error = GetDirentInfoFromName(parent, filename, &dirent);
    uint32_t object = dirent.ObjectCluster;

    DEBUG(1,"\t\t\t\tfind entry returned = %08x with error = %08x", object, error);
    if (FF_isERR(error))
        return PTP_RC_InvalidParameter;

    DEBUG(1,"\t\t\t\tobj cluster = %08x", object);

    uint32_t handle = ((GetClusterIndex(dirent.DirCluster) + 1) << 16) | dirent.CurrentItem;
    DEBUG(1,"[%08x] %08x / %04x : %s", handle, dirent.DirCluster, dirent.CurrentItem, dirent.FileName);
    if (attrib == FF_FAT_ATTR_DIR)
        AddCluster(dirent.ObjectCluster, handle);
    else {
        send_object_handle = handle;
        send_object_filesize = filesize;
    }

    ptp_out->length = PTP_USB_BULK_HDR_LEN + 3 * sizeof(uint32_t);
    ptp_out->type = PTP_USB_CONTAINER_RESPONSE;
    ptp_out->code = PTP_RC_OK;
    ptp_out->trans_id = ptp_in->trans_id;
    ptp_out->payload.params.param1 = 0x00010001;
    ptp_out->payload.params.param2 = parent;
    ptp_out->payload.params.param3 = handle;

    UsbSendPacket((uint8_t*)ptp_out, ptp_out->length);

    return PTP_RC_Undefined;
}

typedef struct _SendObject_Context {
    PTPUSBBulkContainer header;
    FF_FILE* file;
    int32_t remaining_bytes;
} SendObject_Context;

static SendObject_Context send_ctx;
static uint32_t stream_to_file(uint8_t* buffer, uint32_t length, void* context) {
    SendObject_Context* ctx = (SendObject_Context*)context;

//    DEBUG(1,"\t\t\t\t Streaming %08x bytes to %08X", length, ctx->file);
    ctx->remaining_bytes -= length;

    int written = 0;
    if (length)
        written = length;//FF_Write(ctx->file, 1, length, buffer);

    if (ctx->remaining_bytes > 0)
        return written;

    DEBUG(1,"\t\t\t\t Closing %08X", ctx->file);
    FF_Close(ctx->file);

    ptp_out->length = PTP_USB_BULK_HDR_LEN;
    ptp_out->type = PTP_USB_CONTAINER_RESPONSE;
    ptp_out->code = PTP_RC_OK;
    ptp_out->trans_id = ctx->header.trans_id;

    UsbSendPacket((uint8_t*)ptp_out, ptp_out->length);

    stream_func = 0;
    stream_context = 0;

    return written;
}

static uint16_t SendObject(PTPUSBBulkContainer* ptp_in)
{
    /*
        Operation Code          0x100D
        Operation Parameter 1   None
        Operation Parameter 2   None
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    Object Binary Data
        Data Direction          I->R
        ResponseCode Options    OK, Operation_Not_Supported, Session_Not_Open, Invalid_TransactionID, Access_Denied, Invalid_StorageID, Store_Read_Only, Object_Too_Large, Store_Full, Invalid_ObjectFormatCode, Store_Not_Available, Parameter_Not_Supported, Invalid_ParentObject
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None
    */

    DumpParams(ptp_in);

    if (ptp_in->type == PTP_USB_CONTAINER_DATA)
    {
        uint32_t written = stream_to_file(ptp_in->payload.data, min(ptp_in->length,512) - PTP_USB_BULK_HDR_LEN, stream_context);
        DEBUG(1,"\t\t\t\t Streaming first packet - %08x bytes", written);
        return PTP_RC_Undefined;
    }

    uint32_t handle = send_object_handle;

    // create full path

    char buffer[FF_MAX_PATH];
    size_t pos = sizeof(buffer) - 1;
    buffer[pos] = 0;

    do {
        uint16_t cluster_index = (handle >> 16) - 1;
        uint16_t item_index = handle & 0xffff;

        FF_DIRENT dirent;
        if (FF_isERR(GetDirentInfo(cluster_index, item_index, &dirent)))
            return PTP_RC_InvalidObjectHandle;

        // insert current filename infront of existing name
        size_t len = strlen(dirent.FileName);
        pos -= len;
        memcpy(&buffer[pos], dirent.FileName, len);
        buffer[--pos] = '/';

        handle = assoclink[cluster_index];
    } while(handle);

    const char* path = &buffer[pos];

    DEBUG(1,"\t\t\t\tFULL PATH = %s", path);

    memcpy(&send_ctx.header, ptp_in, sizeof(send_ctx.header));
    send_ctx.file = FF_Open(pIoman, path, FF_MODE_WRITE | FF_MODE_CREATE | FF_MODE_TRUNCATE, NULL);
    if (!send_ctx.file)
        return PTP_RC_GeneralError;
    send_ctx.remaining_bytes = send_object_filesize;

    DEBUG(1,"\t\t\t\tFILE* = %08X", send_ctx.file);

    stream_func = stream_to_file;
    stream_context = &send_ctx;

    return PTP_RC_Undefined;
}

static uint16_t GetDevicePropDesc(PTPUSBBulkContainer* ptp_in)
{
    /*
        Operation Code          0x1014
        Operation Parameter 1   DevicePropCode
        Operation Parameter 2   None
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    DevicePropDesc dataset
        Data Direction          R->I
        ResponseCode Options    OK, Operation_Not_Supported, Session_Not_Open, Invalid_TransactionID, Access_Denied, DeviceProp_Not_Supported, Device_Busy, Parameter_Not_Supported
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None

        Dataset field       Field order Size (bytes)Datatype
        ----------------------------------------------------
        Storage Type                1   2           UINT16
        Filesystem Type             2   2           UINT16
        Access Capability           3   2           UINT16
        Max Capacity                4   8           UINT64
        Free Space In Bytes         5   8           UINT64
        *Free Space In Objects      6   4           UINT32
        Storage Description         7   Variable    String
        Volume Identifier           8   Variable    String

        Device Property Code        1   2           UINT16  A specific device property code.
        Datatype                    2   2           UINT16  Identifies the data type code of the property, as defined in section 3.2 Simple Types.
        Get/Set                     3   1           UINT8   Indicates whether the property is read-only (Get), or read-write (Get/Set).
                                                            0x00 Get 
                                                            0x01 Get/Set
        Factory Default Value       4   DTS         DTS     Identifies the value of the factory default for the property.
        Current Value               5   DTS         DTS     Identifies the current value of this property.
        Form Flag                   6   1           UINT8   Indicates the format of the next field.
                                                            0x00 None. This is for properties like DateTime. In this case the FORM field is not present. 
                                                            0x01 Range-Form
                                                            0x02 Enumeration-Form
        FORM                        N/A <variable> -        This dataset depends on the Form Flag, and is absent if Form Flag = 0x00

        Range FORM field    Field order Size (bytes)Datatype
        ----------------------------------------------------
        Minimum Value               7   DTS         DTS     Minimum value supported by this property.
        Maximum Value               8   DTS         DTS     Maximum value supported by this property.
        Step Size                   9   DTS         DTS     A particular vendor's device shall support all values of a property defined by
                                                            Minimum Value + N x Step Size, which is less than or equal to Maximum Value
                                                            where N= 0 to a vendor-defined maximum.

        Enum FORM field     Field order Size (bytes)Datatype
        ----------------------------------------------------
        Number Of Values            7   2           UINT16  This field indicates the number of values of size DTS of the particular property supported by the device.
        Supported Value 1           8   DTS         DTS     A particular vendor's device shall support this value of the property.
        Supported Value 2           9   DTS         DTS     A particular vendor's device shall support this value of the property.
        Supported Value 3           10  DTS         DTS     A particular vendor's device shall support this value of the property.
        …                           …   …           …       …
        Supported Value M           M+7 DTS         DTS     A particular vendor's device shall support this value of the property
    */

    DumpParams(ptp_in);

    uint16_t prop_code = ptp_in->payload.params.param1;
    uint16_t datatype = PTP_DTC_UINT32;

    if (prop_code == PTP_DPC_BatteryLevel)
        datatype = PTP_DTC_UINT8;
    if (prop_code == PTP_DPC_MTP_DeviceFriendlyName)
        datatype = PTP_DTC_STR;

    size_t offset = 0;
    offset = Write16bits(ptp_out, offset, prop_code);
    offset = Write16bits(ptp_out, offset, datatype);
    offset = Write8bits(ptp_out, offset, 0);        // get / set
    if (datatype == PTP_DTC_UINT32) {
        offset = Write32bits(ptp_out, offset, 0);       // default value
        offset = Write32bits(ptp_out, offset, -1);      // current value
    } else if (datatype == PTP_DTC_UINT8) {
        offset = Write8bits(ptp_out, offset, 0);       // default value
        offset = Write8bits(ptp_out, offset, -1);      // current value
    } else if (datatype == PTP_DTC_STR) {
        offset = WriteString(ptp_out, offset, "REPLAY");       // default value
        offset = WriteString(ptp_out, offset, "REPLAY");      // current value
    } else
        return PTP_RC_GeneralError;
    offset = Write32bits(ptp_out, offset, 0);       // group code
    if (datatype == PTP_DTC_STR)
        offset = Write8bits(ptp_out, offset, 0);        // form flag
    else {
        offset = Write8bits(ptp_out, offset, 1);        // form flag
        if (datatype == PTP_DTC_UINT32) {
            offset = Write32bits(ptp_out, offset, 0);       // minimum value
            offset = Write32bits(ptp_out, offset, -1);      // maximum value
            offset = Write32bits(ptp_out, offset, 1);       // step size
        } else if (datatype == PTP_DTC_UINT8) {
            offset = Write8bits(ptp_out, offset, 0);       // minimum value
            offset = Write8bits(ptp_out, offset, -1);      // maximum value
            offset = Write8bits(ptp_out, offset, 1);       // step size
        } else
            return PTP_RC_GeneralError;
    }

//        0000: 1700 0000 0200 1410 1c00 0000 0150 0200   .............P..
//        0010: 0000 4301 0064 01                         ..C..d.

    return PTP_RC_OK;
}

static uint16_t GetDevicePropValue(PTPUSBBulkContainer* ptp_in)
{
    /*
        Operation Code          0x1015
        Operation Parameter 1   DevicePropCode
        Operation Parameter 2   None
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    DeviceProp Value
        Data Direction          R->I
        ResponseCode Options    OK, Operation_Not_Supported, Session_Not_Open, Invalid_TransactionID, DeviceProp_Not_Supported, Device_Busy, Parameter_Not_Supported
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None
    */

    return PTP_RC_GeneralError;
}

static uint16_t SetDevicePropValue(PTPUSBBulkContainer* ptp)
{
    /*
        Operation Code          0x1016
        Operation Parameter 1   DevicePropCode
        Operation Parameter 2   None
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    DeviceProp Value
        Data Direction          I->R
        ResponseCode Options    OK, Session_Not_Open, Invalid_TransactionID, Access_Denied, DeviceProp_Not_Supported, ObjectProp_Not_Supported, Invalid_DeviceProp_Format, Invalid_DeviceProp_Value, Device_Busy, Parameter_Not_Supported
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None
    */

    return PTP_RC_GeneralError;
}

static uint16_t ResetDevicePropValue(PTPUSBBulkContainer* ptp)
{
    /*
        Operation Code          0x1017
        Operation Parameter 1   DevicePropCode
        Operation Parameter 2   None
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    None
        Data Direction          N/A
        ResponseCode Options    OK, Operation_Not_Supported, Session_Not_Open, Invalid_TransactionID, DeviceProp_Not_Supported, Device_Busy, Parameter_Not_Supported, Access_Denied
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None
    */

    return PTP_RC_GeneralError;
}

static uint16_t GetPartialObject(PTPUSBBulkContainer* ptp)
{
    /*
        Operation Code          0x101B
        Operation Parameter 1   ObjectHandle
        Operation Parameter 2   Offset in bytes
        Operation Parameter 3   Maximum number of bytes to obtain
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    Object Binary Data
        Data Direction          R->I
        ResponseCode Options    OK, Operation_Not_Supported, Session_Not_Open, Invalid_TransactionID, Invalid_ObjectHandle, Invalid_ObjectFormatCode, Invalid_Parameter, Store_Not_Available, Device_Busy, Parameter_Not_Supported
        Response Parameter 1    Actual number of bytes sent
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None
    */
    return PTP_RC_GeneralError;
}


typedef struct _ObjectProperty {
    uint16_t opc;
    uint16_t dtc;
    size_t def_value;
} ObjectProperty;

/*
0522             MtpConstants.PROPERTY_STORAGE_ID,
0523             MtpConstants.PROPERTY_OBJECT_FORMAT,
0524             MtpConstants.PROPERTY_PROTECTION_STATUS,
0525             MtpConstants.PROPERTY_OBJECT_SIZE,
0526             MtpConstants.PROPERTY_OBJECT_FILE_NAME,
0527             MtpConstants.PROPERTY_DATE_MODIFIED,
0528             MtpConstants.PROPERTY_PARENT_OBJECT,
0529             MtpConstants.PROPERTY_PERSISTENT_UID,
0530             MtpConstants.PROPERTY_NAME,
0531             MtpConstants.PROPERTY_DATE_ADDED,
*/

static const ObjectProperty ObjectPropsSupported[] = {
    { PTP_OPC_StorageID,                        PTP_DTC_UINT32,         0x00010001 /*default storage*/ },
    { PTP_OPC_ObjectFormat,                     PTP_DTC_UINT16,         PTP_OFC_Undefined /* 'file' */ },
    { PTP_OPC_ProtectionStatus,                 PTP_DTC_UINT16,         PTP_PS_NoProtection },
    { PTP_OPC_ObjectSize,                       PTP_DTC_UINT32,         0x00000000 /* 'empty file' */ },
    { PTP_OPC_ObjectFileName,                   PTP_DTC_STR,            (size_t) "" /* 'empty string' */ },
    { PTP_OPC_ParentObject,                     PTP_DTC_UINT32,         0x00000000 /* 'root dir' */ },
    { PTP_OPC_Name,                             PTP_DTC_STR,            (size_t) "" /* 'empty string' */ },
    { PTP_OPC_DateModified,                     PTP_DTC_STR,            (size_t) "19700101T010101.5" },
    { PTP_OPC_DateAdded,                        PTP_DTC_STR,            (size_t) "19700101T010101.5" },
    { PTP_OPC_PersistantUniqueObjectIdentifier, PTP_DTC_UINT64,         (size_t) 0 },

//    PTP_OPC_DateModified,
//    PTP_OPC_PersistantUniqueObjectIdentifier,
//    PTP_OPC_SystemObject,
//    PTP_OPC_DateAdded,

};

static uint16_t GetObjectPropsSupported(PTPUSBBulkContainer* ptp_in)
{
    /*
        Operation Code          0x9801
        Operation Parameter 1   ObjectFormatCode
        Operation Parameter 2   None
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    ObjectPropCode Array
        Data Direction          R->I
        ResponseCode Options    OK, Operation_Not_Supported, Device_Busy, Invalid_TransactionID, Invalid_ObjectFormatCode
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None
    */

    DumpParams(ptp_in);

    size_t offset = 0;

    const uint32_t num_props = sizeof(ObjectPropsSupported) / sizeof(ObjectPropsSupported[0]);
    offset = Write32bits(ptp_out, offset, num_props);
    for (uint32_t i = 0; i < num_props; ++i)
        offset = Write16bits(ptp_out, offset, ObjectPropsSupported[i].opc);

    ptp_out->length = PTP_USB_BULK_HDR_LEN + offset;
    ptp_out->type = PTP_USB_CONTAINER_DATA;
    ptp_out->code = ptp_in->code;
    ptp_out->trans_id = ptp_in->trans_id;

    UsbSendPacket((uint8_t*)ptp_out, ptp_out->length);

    return PTP_RC_OK;

//        0000: 2600 0000 0200 0198 2300 0000 0b00 0000   &.......#.......
//        0010: 01dc 02dc 03dc 04dc 07dc 09dc 0bdc 41dc   ..............A.
//        0020: 44dc e0dc 4edc                            D...N.
// return list of PTP_OPC_* codes
//    return Unsupported(ptp);
}

static uint16_t GetObjectPropDesc(PTPUSBBulkContainer* ptp_in)
{
    /*
        Operation Code          0x9802
        Operation Parameter 1   ObjectPropCode
        Operation Parameter 2   Object Format Code
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    ObjectPropDesc dataset
        Data Direction          R->I
        ResponseCode Options    OK, Operation_Not_Supported, Session_Not_Open, Invalid_TransactionID, Access_Denied, Invalid_ObjectPropCode, Invalid_ObjectFormatCode, Device_Busy
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None

        Dataset field       Field order Size (bytes)Datatype
        ----------------------------------------------------
        Property Code               1   2           UINT16
        Datatype                    2   2           UINT16
        Get/Set                     3   1           UINT8
        Default Value               4   Variable    Variable
        Group Code                  5   4           UINT32
        Form Flag                   6   1           UINT8
        FORM                        n/a Variable    Depends on Form Flag
    */

    DumpParams(ptp_in);

    uint16_t prop_code = ptp_in->payload.params.param1;
//    uint16_t format_code = ptp_in->payload.params.param2;

    const uint32_t num_props = sizeof(ObjectPropsSupported) / sizeof(ObjectPropsSupported[0]);
    uint32_t prop_index = 0;
    for (prop_index = 0; prop_index < num_props; ++prop_index)
        if (ObjectPropsSupported[prop_index].opc == prop_code)
            break;

    if (prop_index == num_props)
        return PTP_RC_GeneralError;

    uint16_t datatype = ObjectPropsSupported[prop_index].dtc;
    uint8_t get_set = (prop_code == PTP_OPC_ObjectFileName || prop_code == PTP_OPC_Name) ? 0x01 : 0x00;    // only ObjectFileName can be changed
    uint32_t def = ObjectPropsSupported[prop_index].def_value;
    uint32_t group = 0x00000000;
    uint8_t form = 0x00;

    size_t offset = 0;
    offset = Write16bits(ptp_out, offset, prop_code);
    offset = Write16bits(ptp_out, offset, datatype);
    offset = Write8bits(ptp_out, offset, get_set);        // get / set

    DEBUG(1,"Property Code : %04x", prop_code);
    DEBUG(1,"Datatype      : %04x", datatype);
    DEBUG(1,"Get/Set       : %02x", get_set);

    // write variable size default values 
    if (datatype == PTP_DTC_UINT16) {
        offset = Write16bits(ptp_out, offset, (uint16_t) def);    // default value
        DEBUG(1,"Default Value : %04x", def);
    }
    else if (datatype == PTP_DTC_UINT32) {
        offset = Write32bits(ptp_out, offset, def);       // default value
        DEBUG(1,"Default Value : %08x", def);
    }
    else if (datatype == PTP_DTC_UINT64) {
        uint64_t def_value = def == 0 ? 0x0 : *(uint64_t*) def;
        offset = Write64bits(ptp_out, offset, def_value >> 32, def_value & 0xffffffff);    // default value
        DEBUG(1,"Default Value : %08x%08x", def_value >> 32, def_value & 0xffffffff);
    }
    else if (datatype == PTP_DTC_STR) {
        const char* def_value = def == 0 ? "" : (const char*) def;
        offset = WriteString(ptp_out, offset, def_value);      // default value
        DEBUG(1,"Default Value : %s", def_value);
    }
    else
        return PTP_RC_GeneralError;

    offset = Write32bits(ptp_out, offset, group);       // group code
    offset = Write8bits(ptp_out, offset, form);        // form flag

    DEBUG(1,"Group         : %08x", group);
    DEBUG(1,"Form          : %02x", form);

    ptp_out->length = PTP_USB_BULK_HDR_LEN + offset;
    ptp_out->type = PTP_USB_CONTAINER_DATA;
    ptp_out->code = ptp_in->code;
    ptp_out->trans_id = ptp_in->trans_id;

    UsbSendPacket((uint8_t*)ptp_out, ptp_out->length);

    return PTP_RC_OK;

//        0000: 1e00 0000 0200 0298 0200 0000 04dc 0800   ................
//        0010: 00 00000000 00000000 00000000 00        ..............
//    return Unsupported(ptp_in);
}

                    // hack
                    static uint32_t PUOID[2];

                    static uint16_t GetSingleObjectPropValue(uint32_t handle, uint16_t prop_code, FF_DIRENT* cached_dirent, size_t* value_out)
                    {
                        if (prop_code == PTP_OPC_StorageID)
                            *value_out = 0x00010001;
                        else if (prop_code == PTP_OPC_ProtectionStatus)
                            *value_out = PTP_PS_NoProtection;
                        else if (prop_code == PTP_OPC_ObjectFormat) {
                            uint16_t object_format = PTP_OFC_Undefined;

                            // scan all know assoclinks for the handle. if found -> handle is an association
                            for (uint16_t i = 0; i < num_clusters; ++i) {
                                if (assoclink[i] == handle) {
                                    object_format = PTP_OFC_Association;
                                    break;
                                }
                            }
                            *value_out = object_format;
                        } else {

                            // all other object properties requires a directory fetch..

                            uint16_t cluster_index = (handle >> 16) - 1;
                            uint16_t item_index = handle & 0xffff;

                            if (cluster_index >= num_clusters)
                                return PTP_RC_InvalidObjectHandle;
                            uint32_t dir_cluster = dirclusters[cluster_index];
                            if (dir_cluster == 0x00000000)
                                return PTP_RC_InvalidParameter;

                            // if the cached dirent doesn't match what we're looking for -> re-read
                            if (cached_dirent->DirCluster != dir_cluster || cached_dirent->CurrentItem != item_index)
                            {
                                FF_DIRENT dirent;
                                memset(&dirent, 0x00, sizeof(dirent));

                                dirent.DirCluster = dir_cluster;
                                dirent.CurrentItem = item_index;

                                DEBUG(1,"\t\t\t\tinit entry fetch");
                                if(FF_isERR(FF_InitEntryFetch(pIoman, dirent.DirCluster, &dirent.FetchContext)))
                                    return PTP_RC_GeneralError;

                                DEBUG(1,"\t\t\t\tfetch dirent");

                                FF_ERROR err = FetchDirents(&dirent, GetObjectInfo_Callback, cached_dirent);

                                DEBUG(1,"\t\t\t\tcheck return value");

                                if (FF_isERR(err) && FF_GETERROR(err) != FF_ERR_DIR_END_OF_DIR)
                                    return PTP_RC_GeneralError;

                                DEBUG(1,"\t\t\t\tvalidate cluster/index");

                                if (cached_dirent->DirCluster != dir_cluster || cached_dirent->CurrentItem != item_index)
                                    return PTP_RC_InvalidObjectHandle;
                            }

                            if (prop_code == PTP_OPC_ObjectSize)
                                *value_out = cached_dirent->Filesize;
                            else if (prop_code == PTP_OPC_ObjectFileName || prop_code == PTP_OPC_Name)
                               *value_out = (size_t)cached_dirent->FileName;
                            else if (prop_code == PTP_OPC_ParentObject)
                                *value_out = assoclink[cluster_index];
                            else if (prop_code == PTP_OPC_DateModified)
                                *value_out = (size_t)"19700101T010101.5";
                            else if (prop_code == PTP_OPC_DateAdded)
                                *value_out = (size_t)"19700101T010101.5";
                            else if (prop_code == PTP_OPC_PersistantUniqueObjectIdentifier) {
                                PUOID[0] = cached_dirent->DirCluster;
                                PUOID[1] = cached_dirent->ObjectCluster;
                                *value_out = (size_t)PUOID;
                            }
                            else {
                                DEBUG(1,"Unsupported");
                                return PTP_RC_GeneralError;
                            }
                        }
                        return PTP_RC_OK;
                    }

static uint16_t GetObjectPropValue(PTPUSBBulkContainer* ptp_in)
{
    /*
        Operation Code          0x9803
        Operation Parameter 1   ObjectHandle
        Operation Parameter 2   ObjectPropCode
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    ObjectProp Value
        Data Direction          R->I
        ResponseCode Options    OK, Operation_Not_Supported, Session_Not_Open, Invalid_TransactionID, Invalid_ObjectPropCode, Device_Busy, Invalid_Object_Handle
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None
    */

    DumpParams(ptp_in);

    uint32_t handle = ptp_in->payload.params.param1;
    uint32_t prop_code = ptp_in->payload.params.param2;

    size_t offset = 0;

    const uint32_t num_props = sizeof(ObjectPropsSupported) / sizeof(ObjectPropsSupported[0]);
    uint32_t prop_index = 0;
    for (prop_index = 0; prop_index < num_props; ++prop_index)
        if (ObjectPropsSupported[prop_index].opc == prop_code)
            break;

    if (prop_index == num_props)
        return PTP_RC_GeneralError;

    uint16_t datatype = ObjectPropsSupported[prop_index].dtc;
    size_t value = 0;

    FF_DIRENT dirent;
    memset(&dirent, 0x00, sizeof(dirent));
    uint16_t rc = GetSingleObjectPropValue(handle, prop_code, &dirent, &value);
    if (rc != PTP_RC_OK) {
        DEBUG(1,"Unknown prop_code / error getting property : %04x", prop_code);    
        return rc;
    }
    offset = WriteVariable(ptp_out, offset, datatype, value);

    ptp_out->length = PTP_USB_BULK_HDR_LEN + offset;
    ptp_out->type = PTP_USB_CONTAINER_DATA;
    ptp_out->code = ptp_in->code;
    ptp_out->trans_id = ptp_in->trans_id;

    UsbSendPacket((uint8_t*)ptp_out, ptp_out->length);

    return PTP_RC_OK;

//        0000: 1400 0000 0200 0398 2400 0000 0000 0000   ........$.......
//        0010: 0000 0000                                 ....
//        0000: 1400 0000 0100 0398 5100 0000 1000 0000   ........Q.......
//        0010: 04dc 0000                                 ....
//    return Unsupported(ptp_in);
}

static uint16_t SetObjectPropValue(PTPUSBBulkContainer* ptp_in)
{
    /*
        Operation Code          0x9804
        Operation Parameter 1   ObjectHandle
        Operation Parameter 2   ObjectPropCode
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    ObjectProp Value
        Data Direction          I->R
        ResponseCode Options    OK, Session_Not_Open, Invalid_TransactionID, Access_Denied, Invalid_ObjectPropCode, Invalid_ObjectHandle, Device_Busy, Invalid_ObjectProp_Format, Invalid_ObjectProp_Value
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None
    */

    DumpParams(ptp_in);

    static uint32_t handle;
    static uint32_t prop_code;

    // we expect data (== the next packet)
    if (ptp_in->type == PTP_USB_CONTAINER_COMMAND)
    {
        handle = ptp_in->payload.params.param1;
        prop_code = ptp_in->payload.params.param2;
        return PTP_RC_Undefined;
    }

    if (prop_code == PTP_OPC_ObjectFileName) {
        char new_name[FF_MAX_PATH];
        const char* old_name = GetFullPathFromHandle(handle, NULL, NULL);
        DEBUG(1,"\t\t\t\told_name = %s", old_name);

        uint8_t len = ptp_in->length - PTP_USB_BULK_HDR_LEN;
        uint8_t offset = strrchr(old_name, '/') - old_name + 1;
        DEBUG(1,"\t\t\t\tlen = %d", len);
        DEBUG(1,"\t\t\t\toffset = %d", offset);
        memcpy(new_name, old_name, offset);
        ReadString(ptp_in, 0, &new_name[offset]);
        DEBUG(1,"\t\t\t\tnew_name = %s", new_name);


        FF_Move(pIoman, old_name, new_name);


//        uint32_t new_handle = GetHandleFromFullPath(handle);

        DEBUG(1,"\t\t\t\t -- get new handle");

        uint32_t parent = 0;
        FF_ERROR error = FF_ERR_NONE;
        FF_T_UINT32 dirCluster = FF_FindDir(pIoman, new_name, offset, &error);
        DEBUG(1,"\t\t\t\t error = %08x", error);
        DEBUG(1,"\t\t\t\t dirCluster = %08x", dirCluster);
        for (size_t cluster_index = 0; cluster_index < num_clusters; ++cluster_index)
            if (dirclusters[cluster_index] == dirCluster)
                parent = assoclink[cluster_index];
        DEBUG(1,"\t\t\t\t parent = %08x", parent);
        FF_DIRENT dirent;
        error = GetDirentInfoFromName(parent, &new_name[offset], &dirent);
        DEBUG(1,"\t\t\t\t error = %08x", error);
        DEBUG(1,"\t\t\t\t dirent.DirCluster = %08x", dirent.DirCluster);
        DEBUG(1,"\t\t\t\t dirent.CurrentItem = %08x", dirent.CurrentItem);
        uint32_t new_handle = ((GetClusterIndex(dirent.DirCluster) + 1) << 16) | dirent.CurrentItem;
        DEBUG(1,"\t\t\t\t old_handle = %08x", handle);
        DEBUG(1,"\t\t\t\t new_handle = %08x", new_handle);

        moved[num_moved*2+0] = handle;
        moved[num_moved*2+1] = new_handle;
        num_moved++;
    }

    return PTP_RC_OK;
}

static uint16_t GetObjPropList(PTPUSBBulkContainer* ptp_in)
{
    /*
        Operation Code          0x9805
        Operation Parameter 1   ObjectHandle
        Operation Parameter 2   [ObjectFormatCode]
        Operation Parameter 3   ObjectPropCode
        Operation Parameter 4   [ObjectPropGroupCode]
        Operation Parameter 5   [Depth]
        Data                    ObjectPropList dataset
        Data Direction          R->I
        ResponseCode Options    OK, Operation_Not_Supported, Session_Not_Open, Invalid_TransactionID, ObjectProp_Not_Supported, Invalid_ObjectHandle, Group_Not_Supported, Device_Busy, Parameter_Not_Supported, Specification_By_Format_Unsupported, Specification_By_Group_Unsupported, Specification_By_Depth_Unsupported, Invalid_Code_Format, Invalid_ObjectPropCode, Invalid_StorageID, Store_Not_Available
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None

        Dataset field       Field order Size (bytes)Datatype
        ----------------------------------------------------
        Number of Elements          1   4           UINT32
        #1 Handle                   2   4           UINT32
        #1 Property Code            3   2           UINT16
        #1 Datatype                 4   Variable    Variable
        #1 Value                    5   4           UINT32
        #2 Handle                   6   4           UINT32
        #2 Property Code            7   2           UINT16
        #2 Datatype                 8   Variable    Variable
        #2 Value                    9   4           UINT32
        ...
        #N Handle               4*N-2   4           UINT32
    */

    DumpParams(ptp_in);

    uint32_t handle = ptp_in->payload.params.param1;

    if (ptp_in->payload.params.param2 != 0)
        return PTP_RC_SpecificationByFormatUnsupported;

    if (ptp_in->payload.params.param3 == 0)
        return PTP_RC_MTP_Specification_By_Group_Unsupported;

    if (ptp_in->payload.params.param5 != 0)
        return PTP_RC_MTP_Specification_By_Depth_Unsupported;

    size_t offset = 0;

    uint32_t prop_index = 0;
    uint32_t num_props = sizeof(ObjectPropsSupported) / sizeof(ObjectPropsSupported[0]);

    if (ptp_in->payload.params.param3 != 0xffffffff) {                        // "A value of 0xFFFFFFFF indicates that all properties are requested"
        uint16_t prop_code = ptp_in->payload.params.param3;
        for (prop_index = 0; prop_index < num_props; ++prop_index)
            if (ObjectPropsSupported[prop_index].opc == prop_code)
                break;

        if (prop_index == num_props) {
            DEBUG(1,"Unknown prop_code : %04x", prop_code);        
            num_props = 0;
        } else {
            num_props = 1;
        }
    }

    offset = Write32bits(ptp_out, offset, num_props);

    FF_DIRENT dirent;
    memset(&dirent, 0x00, sizeof(dirent));
    for (uint32_t i = prop_index; i < (prop_index + num_props); ++i) {

        uint16_t prop_code = ObjectPropsSupported[i].opc;
        uint16_t datatype = ObjectPropsSupported[i].dtc;
        size_t value = 0;

        if (GetSingleObjectPropValue(handle, prop_code, &dirent, &value) != PTP_RC_OK) {
            DEBUG(1,"Unknown prop_code / error getting property : %04x", prop_code);
            return PTP_RC_GeneralError;    
        }

        offset = Write32bits(ptp_out, offset, handle);
        offset = Write16bits(ptp_out, offset, prop_code);
        offset = Write16bits(ptp_out, offset, datatype);
        offset = WriteVariable(ptp_out, offset, datatype, value);
    }

    ptp_out->length = PTP_USB_BULK_HDR_LEN + offset;
    ptp_out->type = PTP_USB_CONTAINER_DATA;
    ptp_out->code = ptp_in->code;
    ptp_out->trans_id = ptp_in->trans_id;

    UsbSendPacket((uint8_t*)ptp_out, ptp_out->length);

    return PTP_RC_OK;
}

static uint16_t GetObjectReferences(PTPUSBBulkContainer* ptp_in)
{
    /*
        Operation Code          0x9810
        Operation Parameter 1   ObjectHandle
        Operation Parameter 2   None
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    ObjectHandle array
        Data Direction          R->I
        ResponseCode Options    OK, Operation_Not_Supported, Session_Not_Open, Invalid_TransactionID, Invalid_ObjectHandle, Store_Not_Available
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None
    */

    DumpParams(ptp_in);

    size_t offset = 0;
    offset = Write32bits(ptp_out, offset, 0);                           // number of object handles

    ptp_out->length = PTP_USB_BULK_HDR_LEN + offset;
    ptp_out->type = PTP_USB_CONTAINER_DATA;
    ptp_out->code = ptp_in->code;
    ptp_out->trans_id = ptp_in->trans_id;

    UsbSendPacket((uint8_t*)ptp_out, ptp_out->length);

    return PTP_RC_OK;
}

static uint16_t SetObjectReferences(PTPUSBBulkContainer* ptp)
{
    /*
        Operation Code          0x9811
        Operation Parameter 1   ObjectHandle
        Operation Parameter 2   None
        Operation Parameter 3   None
        Operation Parameter 4   None
        Operation Parameter 5   None
        Data                    ObjectHandle array
        Data Direction          I->R
        ResponseCode Options    OK, Operation_Not_Supported, Session_Not_Open, Invalid_TransactionID, Access_Denied, Invalid_StorageID,Store_Read_Only, Store_Full, Store_Not_Available, Invalid_ObjectHandle, Invalid_ObjectReference
        Response Parameter 1    None
        Response Parameter 2    None
        Response Parameter 3    None
        Response Parameter 4    None
        Response Parameter 5    None
    */
    return PTP_RC_GeneralError;
}

static uint16_t GetPartialObject64(PTPUSBBulkContainer* ptp)
{
    return PTP_RC_GeneralError;
}

static uint16_t SendPartialObject(PTPUSBBulkContainer* ptp)
{
    return PTP_RC_GeneralError;
}

static uint16_t TruncateObject(PTPUSBBulkContainer* ptp)
{
    return PTP_RC_GeneralError;
}

static uint16_t BeginEditObject(PTPUSBBulkContainer* ptp)
{
    return PTP_RC_GeneralError;
}

static uint16_t EndEditObject(PTPUSBBulkContainer* ptp)
{ 
    return PTP_RC_GeneralError;
}
