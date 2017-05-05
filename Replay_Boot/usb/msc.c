#include "msc.h"
#include "msc_descriptor.h"
#include "usb_hardware.h"
#include "messaging.h"

void UsbPacketReceived(uint8_t *data, int len);

// The buffer used to store packets received over USB while we are in the
// process of receiving them.
static uint8_t UsbBuffer[512] __attribute__((aligned(8)));

// The number of characters received in the USB buffer so far.
static int  UsbSoFarCount;

static uint8_t CurrentConfiguration;

static void HandleRxdSetupData(UsbSetupPacket* data);
static void HandleRxdData(uint8_t ep, uint8_t* packet, uint32_t length);

static void msc_packet_recv(uint8_t* packet, uint32_t length);


void msc_recv(uint8_t ep, uint8_t* packet, uint32_t length)
{
;;//    DEBUG(1,"ptp_recv (ep = %d; packet = %08x, length = %d", ep, packet, length);
	if (ep == 0) {
		if (length != sizeof(UsbSetupPacket)) {
	        DEBUG(1,"Setup packet length error!");
		}
		UsbSetupPacket* data = (UsbSetupPacket*)packet;
		HandleRxdSetupData(data);
	} else if (ep == 2) {
		HandleRxdData(ep, packet, length);
    } else {
        ERROR("unknown endpoint!");
    }
}

void msc_send(uint8_t* packet, uint32_t length)
{
	usb_send(1, 64, packet, length, 0);
}

void MSC_Start(void)
{
	usb_connect(msc_recv);
}
void MSC_Stop(void)
{
	usb_disconnect();
}
uint8_t MSC_Poll(void)
{
	return usb_poll();
}

static void HandleRxdSetupData(UsbSetupPacket* data)
{
    UsbSetupPacket usd;
    memcpy(&usd, data, sizeof(usd));

    DEBUG(1,"--- RXSETUP:");
    DEBUG(1,"usd.bmRequestType = %02X", usd.bmRequestType);
    DEBUG(1,"usd.bRequest      = %02X", usd.bRequest);
    DEBUG(1,"usd.wValue        = %04X", usd.wValue);
    DEBUG(1,"usd.wIndex        = %04X", usd.wIndex);
    DEBUG(1,"usd.wLength       = %04X", usd.wLength);

    switch(usd.bRequest) {
        case USB_REQUEST_GET_DESCRIPTOR:
            DEBUG(1,"\tUSB_REQUEST_GET_DESCRIPTOR");
            if((usd.wValue >> 8) == USB_DESCRIPTOR_TYPE_DEVICE) {
                DEBUG(1,"\t\tUSB_DESCRIPTOR_TYPE_DEVICE");
                usb_send_ep0((uint8_t *)&DeviceDescriptor,
                    sizeof(DeviceDescriptor), usd.wLength);
            } else if((usd.wValue >> 8) == USB_DESCRIPTOR_TYPE_CONFIGURATION) {
                DEBUG(1,"\t\tUSB_DESCRIPTOR_TYPE_CONFIGURATION");
                usb_send_ep0((uint8_t *)&ConfigurationDescriptor,
                    sizeof(ConfigurationDescriptor), usd.wLength);
//            } else if((usd.wValue >> 8) == USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER) {
//                DEBUG(1,"\t\tUSB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER");
//                usb_send_ep0((uint8_t *)&DeviceQualifierDescriptor,
//                    sizeof(DeviceQualifierDescriptor), usd.wLength);
            } else if((usd.wValue >> 8) == USB_DESCRIPTOR_TYPE_STRING) {
                DEBUG(1,"\t\tUSB_DESCRIPTOR_TYPE_STRING");
                const uint8_t *s = StringDescriptors[usd.wValue & 0xff];
                if ((usd.wValue & 0xff) == 0xee)
                    usb_send_ep0(StringDescriptorEE, StringDescriptorEE[0], usd.wLength);
                else if ((usd.wValue & 0xff) < sizeof(StringDescriptors) / sizeof(StringDescriptors[0]))
                    usb_send_ep0(s, s[0], usd.wLength);
                else
                    usb_send_ep0_stall();
            } else {
                DEBUG(1,"\t\tUSB_DESCRIPTOR_TYPE = UNKOWN!");
                usb_send_ep0_stall();
            }
            break;

        case USB_REQUEST_SET_ADDRESS:
            DEBUG(1,"\tUSB_REQUEST_SET_ADDRESS");
            usb_send_ep0_zlp();
            usb_setup_faddr(usd.wValue);
            break;

        case USB_REQUEST_GET_CONFIGURATION:
            DEBUG(1,"\tUSB_REQUEST_GET_CONFIGURATION");
            usb_send_ep0(&CurrentConfiguration, sizeof(CurrentConfiguration), usd.wLength);
            break;

        case USB_REQUEST_GET_STATUS: {
            DEBUG(1,"\tUSB_REQUEST_GET_STATUS");
            if(usd.bmRequestType & 0x80) {
                uint16_t w = 0x01; // self-powered
                usb_send_ep0((uint8_t *)&w, sizeof(w), usd.wLength);
            }
            break;
        }
        case USB_REQUEST_SET_CONFIGURATION:
            DEBUG(1,"\tUSB_REQUEST_SET_CONFIGURATION");
            CurrentConfiguration = usd.wValue;
            uint32_t eps[2] = { EPTYPE_BULK_IN, EPTYPE_BULK_OUT };
            if(CurrentConfiguration) {
                usb_setup_endpoints(eps, 2);
            } else {
                usb_setup_endpoints(NULL, 0);
            }
            usb_send_ep0_zlp();
            break;

        case USB_REQUEST_GET_INTERFACE: {
            DEBUG(1,"\tUSB_REQUEST_GET_INTERFACE");
            uint8_t b = 0;
            usb_send_ep0(&b, sizeof(b), usd.wLength);
            break;
        }

        case USB_REQUEST_SET_INTERFACE:
            DEBUG(1,"\tUSB_REQUEST_SET_INTERFACE");
            usb_send_ep0_zlp();
            break;
            
        case USB_REQUEST_CLEAR_FEATURE:
            if(usd.bmRequestType == 0xc0 || usd.bmRequestType == 0xc1) {
                DEBUG(1,"\tUSB_REQUEST_VENDOR_FEATURE");
                if(usd.wIndex == 4 || usd.wIndex == 5) {
                    usb_send_ep0((uint8_t *)&RecipientDevice,
                        sizeof(RecipientDevice), usd.wLength);
                } else {
                    usb_send_ep0_stall();
                }
            } else
                usb_send_ep0_stall();
            break;        
        case USB_REQUEST_SET_FEATURE:
            DEBUG(1,"\tUSB_REQUEST_SET_FEATURE");
            usb_send_ep0_stall();
            break;
        case USB_REQUEST_SET_DESCRIPTOR:
            DEBUG(1,"\tUSB_REQUEST_SET_DESCRIPTOR");
            usb_send_ep0_stall();
            break;
        case USB_REQUEST_SYNC_FRAME:
            DEBUG(1,"\tUSB_REQUEST_SYNC_FRAME");
            usb_send_ep0_stall();
            break;

        case 0x67: {
            DEBUG(1,"\tUSB_REQUEST_WEIRD!!");
            if(usd.bmRequestType & 0x80) {
                uint16_t w = 0;
                usb_send_ep0((uint8_t *)&w, sizeof(w), usd.wLength);
            }
            break;
        }
        case USB_REQUEST_GET_MAX_LUN:
            DEBUG(1,"\tUSB_REQUEST_GET_MAX_LUN");
            uint16_t w = 0x00;
            usb_send_ep0((uint8_t *)&w, sizeof(w), usd.wLength);
            break;
        default:
            WARNING("\tUSB_REQUEST_UNKNOWN!!");
            usb_send_ep0_stall();
            break;
    }
}

static void HandleRxdData(uint8_t ep, uint8_t* packet, uint32_t length)
{
    int i;
    for(i = 0; i < length; i++) {
        UsbBuffer[UsbSoFarCount] = *packet++;
        UsbSoFarCount++;
    }
    if ((UsbSoFarCount & 0x3f) != 0 || UsbSoFarCount == 0 || UsbSoFarCount == sizeof(UsbBuffer))
    {
        DEBUG(1, "msc_packet_recv(UsbBuffer, UsbSoFarCount) = %d bytes", UsbSoFarCount);
        msc_packet_recv(UsbBuffer, UsbSoFarCount);
        DumpBuffer(UsbBuffer, UsbSoFarCount);

        UsbSoFarCount = 0;
    }
}

typedef struct {
    uint8_t     dCBWSignature;      // 'USBC' 43425355h (little endian)
    uint32_t    dCBWTag;            // must match dCSWTag
    uint32_t    dCBWDataTransferLength;
    struct {
        uint8_t reserved:6;
        uint8_t obsolete:1;
        uint8_t direction:1;        // 0 = Data-Out from host to the device, 1 = Data-In from the device to the host
    } bmCBWFlags;
    uint8_t     bCBWLUN:4;          // 4 bits
    uint8_t     reserved0:4;        // (upper 4 bits reserved)
    uint8_t     bCBWCBLength:5;     // 5 bits
    uint8_t     reserved1:3;        // (upper 3 bits reserved)
    uint8_t     CBWCB[16];
} __attribute__ ((packed)) CommandBlockWrapper;

typedef struct {
    uint8_t     dCSWSignature;      // 'USBS' 53425355h (little endian)
    uint32_t    dCSWTag;            // must match dCBWTag
    uint32_t    dCSWDataResidue;
    uint8_t     bCSWStatus;
} __attribute__ ((packed)) CommandStatusWrapper;


// See Universal Serial Bus Mass Storage Class (Bulk-Only Transport) - USB.org
// http://www.usb.org/developers/docs/devclass_docs/usbmassbulk_10.pdf
//
// Host Expectation
// Hn Host expects no data transfers
// Hi Host expects to receive data from the device
// Ho Host expects to send data to the device
// 
// Device Intent
// Dn Device intends to transfer no data
// Di Device intends to send data to the host
// Do Device intends to receive data from the host
//
typedef enum {
    Hn_eq_Dn,   // (1)      expects no data transfers / intends to transfer no data -> result, residue = 0
    Hn_lt_Di,   // (2)      expects no data transfers / intends to send data        -> phase error, residue = 0
    Hn_lt_Do,   // (3)      expects no data transfers / intends to receive data     -> phase error, residue = 0

    Hi_gt_Dn,   // (4)      expects to receive data / intends to transfer no data   -> stall_IN, residue = hlength
    Hi_gt_Di,   // (5)      expects to receive data / intends to send data          -> stall_IN, residue = hlength - dlength
    Hi_eq_Di,   // (6)      expects to receive data / intends to send data          -> result, residue = 0 (hlength - dlength)
    Hi_lt_Di,   // (7)      expects to receive data / intends to send data          -> phase error, residue = hlength
    Hi_ne_Do,   // (8)      expects to receive data / intends to receive data       -> stall_IN + phase error, residue = 0

    Ho_gt_Dn,   // (9)      expects to send data / intends to transfer no data      -> stall_OUT, residue = hlength
    Ho_ne_Di,   // (10)     expects to send data / intends to send data             -> stall_OUT + phase error, residue = 0
    Ho_gt_Do,   // (11)     expects to send data / intends to receive data          -> stall_OUT, residue = hlength - dlength
    Ho_eq_Do,   // (12)     expects to send data / intends to receive data          -> result, residue = 0
    Ho_lt_Do,   // (13)     expects to send data / intends to receive data          -> phase error, residue = hlength
} HostDeviceDataTransferMode;

typedef enum {
    NoTransfer,
    DeviceToHost,
    HostToDevice,
} HostDeviceTransferDirection;

HostDeviceDataTransferMode get_transfer_mode(CommandBlockWrapper* cbw, CommandStatusWrapper* csw);

static void msc_packet_recv(uint8_t* packet, uint32_t length)
{
    CommandBlockWrapper cbw;
    CommandStatusWrapper csw;
    memcpy(&cbw, packet, sizeof(CommandBlockWrapper));

    if (length != sizeof(CommandBlockWrapper) ||
        cbw.dCBWSignature != 0x43425355) {
    
        DEBUG(1, "Error reading CBW");

        usb_send_stall(1);
        usb_send_stall(2);
        return;
    }

    HostDeviceDataTransferMode transferMode = get_transfer_mode(&cbw, &csw);

    switch(transferMode) {
        case Hn_eq_Dn:
        case Hi_eq_Di:
        case Ho_eq_Do:
            // -> result, residue = 0
            break;

        case Hn_lt_Di:
        case Hn_lt_Do:
            // -> phase error, residue = 0
            break;

        case Ho_lt_Do:
        case Hi_lt_Di:
            // -> phase error, residue = hlength
            break;

        case Hi_gt_Dn:
        case Hi_gt_Di:
            // -> stall_IN, residue = hlength - dlength (0)
            break;

        case Ho_gt_Dn:
        case Ho_gt_Do:
            // -> stall_OUT, residue = hlength - dlength (0)
            break;

        case Hi_ne_Do:
            // -> stall_IN + phase error, residue = 0
            break;
        case Ho_ne_Di:
            // -> stall_OUT + phase error, residue = 0
            break;
    }
}



// SCSI Commands Reference Manual (Rev. A) - Seagate
// http://www.seagate.com/staticfiles/support/disc/manuals/scsi/100293068a.pdf

typedef struct {
    uint8_t     OPERATIONCODE;
    uint8_t     EVPD:1;
    uint8_t     CMDDT:1;
    uint8_t     Reserved:6;
    uint8_t     PAGECODE;
    uint8_t     ALLOCATIONLENGTH[2];
    uint8_t     CONTROL;
} __attribute__ ((packed)) INQUIRY;
#define OPERATIONCODE_INQUIRY 0x12

typedef union {
    uint8_t     OPERATIONCODE;
    INQUIRY     inquiry;
} CommandDescriptorBlock;

#define READ_BE_16B(x)  ((uint16_t) ((x[0] << 8) | x[1]))

HostDeviceDataTransferMode get_transfer_mode(CommandBlockWrapper* cbw, CommandStatusWrapper* csw)
{
    const uint32_t hostLength = cbw->dCBWDataTransferLength;
    const HostDeviceTransferDirection hostTransferType = hostLength == 0 ? NoTransfer : cbw->bmCBWFlags.direction ? DeviceToHost : HostToDevice;

    uint32_t deviceLength = 0;
    HostDeviceTransferDirection deviceTransferType = NoTransfer;

    CommandDescriptorBlock* cdb = (CommandDescriptorBlock*)cbw->CBWCB;
    switch (cdb->OPERATIONCODE) {
        case OPERATIONCODE_INQUIRY:
            deviceLength = READ_BE_16B(cdb->inquiry.ALLOCATIONLENGTH);
            deviceTransferType = DeviceToHost;
            break;
    }

    if (hostTransferType == NoTransfer) {
        if (deviceTransferType == NoTransfer) {
            return Hn_eq_Dn;                                // (1)
        } else if (deviceTransferType == DeviceToHost) {
            return Hn_lt_Di;                                // (2)
        } else { // deviceTransferType == HostToDevice
            return Hn_lt_Do;                                // (3)
        }
    } else if (hostTransferType == DeviceToHost) {
        if (deviceTransferType == NoTransfer) {
            return Hi_gt_Dn;                                // (4)
        } else if (deviceTransferType == DeviceToHost) {
            if (hostLength < deviceLength) {
                return Hi_gt_Di;                            // (5)
            } else if (hostLength == deviceLength) {
                return Hi_eq_Di;                            // (6)
            } else { // hostLength > deviceLength
                return Hi_lt_Di;                            // (7)
            }
        } else { // deviceTransferType == HostToDevice
            return Hi_ne_Do;                                // (8)
        }
    } else { // hostTransferType == HostToDevice
        if (deviceTransferType == NoTransfer) {
            return Ho_gt_Dn;                                // (9)
        } else if (deviceTransferType == DeviceToHost) {
            return Ho_ne_Di;                                // (10)
        } else { // deviceTransferType == HostToDevice
            if (hostLength > deviceLength) {
                return Ho_gt_Do;                            // (11)
            } else if (hostLength == deviceLength) {
                return Ho_eq_Do;                            // (12)
            } else { // hostLength < deviceLength
                return Ho_lt_Do;                            // (13)
            }
        }
    }
}
