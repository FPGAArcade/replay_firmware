static uint16_t GetDeviceInfo(PTPUSBBulkContainer*);
static uint16_t OpenSession(PTPUSBBulkContainer*);
static uint16_t CloseSession(PTPUSBBulkContainer*);
static uint16_t GetStorageIDs(PTPUSBBulkContainer*);
static uint16_t GetStorageInfo(PTPUSBBulkContainer*);
static uint16_t GetNumObjects(PTPUSBBulkContainer*);
static uint16_t GetObjectHandles(PTPUSBBulkContainer*);
static uint16_t GetObjectInfo(PTPUSBBulkContainer*);
static uint16_t GetObject(PTPUSBBulkContainer*);
static uint16_t GetThumb(PTPUSBBulkContainer*);
static uint16_t DeleteObject(PTPUSBBulkContainer*);
static uint16_t SendObjectInfo(PTPUSBBulkContainer*);
static uint16_t SendObject(PTPUSBBulkContainer*);
static uint16_t GetDevicePropDesc(PTPUSBBulkContainer*);
static uint16_t GetDevicePropValue(PTPUSBBulkContainer*);
static uint16_t SetDevicePropValue(PTPUSBBulkContainer*);
static uint16_t ResetDevicePropValue(PTPUSBBulkContainer*);
static uint16_t GetPartialObject(PTPUSBBulkContainer*);
static uint16_t GetObjectPropsSupported(PTPUSBBulkContainer*);
static uint16_t GetObjectPropDesc(PTPUSBBulkContainer*);
static uint16_t GetObjectPropValue(PTPUSBBulkContainer*);
static uint16_t SetObjectPropValue(PTPUSBBulkContainer*);
static uint16_t GetObjPropList(PTPUSBBulkContainer*);
static uint16_t GetObjectReferences(PTPUSBBulkContainer*);
static uint16_t SetObjectReferences(PTPUSBBulkContainer*);
static uint16_t GetPartialObject64(PTPUSBBulkContainer*);
static uint16_t SendPartialObject(PTPUSBBulkContainer*);
static uint16_t TruncateObject(PTPUSBBulkContainer*);
static uint16_t BeginEditObject(PTPUSBBulkContainer*);
static uint16_t EndEditObject(PTPUSBBulkContainer*);

#define OpEntry(name) { PTP_OC_##name, name,  #name }
#define OpEntry_MTP(name) { PTP_OC_MTP_##name, name,  #name }
#define OpEntry_ANDROID(name) { PTP_OC_ANDROID_##name, name,  #name }

static const Operation SupportedOperatons[] = {
    OpEntry(GetDeviceInfo),
    OpEntry(OpenSession),
    OpEntry(CloseSession),
    OpEntry(GetStorageIDs),
    OpEntry(GetStorageInfo),
    OpEntry(GetNumObjects),
    OpEntry(GetObjectHandles),
    OpEntry(GetObjectInfo),
    OpEntry(GetObject),
    OpEntry(GetThumb),
    OpEntry(DeleteObject),
    OpEntry(SendObjectInfo),
    OpEntry(SendObject),
    OpEntry(GetDevicePropDesc),
    OpEntry(GetDevicePropValue),
    OpEntry(SetDevicePropValue),
    OpEntry(ResetDevicePropValue),
    OpEntry(GetPartialObject),
    OpEntry_MTP(GetObjectPropsSupported),
    OpEntry_MTP(GetObjectPropDesc),
    OpEntry_MTP(GetObjectPropValue),
    OpEntry_MTP(SetObjectPropValue),
    OpEntry_MTP(GetObjPropList),
    OpEntry_MTP(GetObjectReferences),
    OpEntry_MTP(SetObjectReferences),
    OpEntry_ANDROID(GetPartialObject64),
    OpEntry_ANDROID(SendPartialObject),
    OpEntry_ANDROID(TruncateObject),
    OpEntry_ANDROID(BeginEditObject),
    OpEntry_ANDROID(EndEditObject),
};

#undef OpEntry
#undef OpEntry_MTP
#undef OpEntry_ANDROID

static const uint16_t SupportedEvents[] = {
    PTP_EC_ObjectAdded,
    PTP_EC_ObjectRemoved,
    PTP_EC_StoreAdded,
    PTP_EC_StoreRemoved,
    PTP_EC_DevicePropChanged,
    PTP_EC_MTP_ObjectPropChanged,
};

static const uint16_t SupportedDeviceProps[] = {
    PTP_DPC_MTP_SynchronizationPartner,
    PTP_DPC_MTP_DeviceFriendlyName,
    PTP_DPC_ImageSize,
    PTP_DPC_BatteryLevel,
};

static const uint16_t SupportedPlaybacks[] = {
    PTP_OFC_Undefined,
    PTP_OFC_Association,
/*    PTP_OFC_Text,
    PTP_OFC_HTML,
    PTP_OFC_WAV,
    PTP_OFC_MP3,
    PTP_OFC_MPEG,
    PTP_OFC_EXIF_JPEG,
    PTP_OFC_TIFF_EP,
    PTP_OFC_GIF,
    PTP_OFC_JFIF,
    PTP_OFC_PNG,
    PTP_OFC_TIFF,
    PTP_OFC_MTP_WMA,
    PTP_OFC_MTP_OGG,
    PTP_OFC_MTP_AAC,
    PTP_OFC_MTP_MP4,
    PTP_OFC_MTP_MP2,
    PTP_OFC_MTP_3GP,
    PTP_OFC_MTP_AbstractAudioVideoPlaylist,
    PTP_OFC_MTP_WPLPlaylist,
    PTP_OFC_MTP_M3UPlaylist,
    PTP_OFC_MTP_PLSPlaylist,
    PTP_OFC_MTP_XMLDocument,
    PTP_OFC_MTP_FLAC,*/
};
