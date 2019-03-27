#if defined(AT91SAM7S256)

#include "ptp_usb.h"
#include "ptp_usb_descriptor.h"
#include "mtp_database.h"
#include "mtp_client.h"
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

void ptp_recv(uint8_t ep, uint8_t* packet, uint32_t length)
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

void ptp_send(uint8_t* packet, uint32_t length)
{
	usb_send(1, 64, packet, length, 0);
}

void PTP_USB_Start(void)
{
    // HACK! we need this value later and it takes too long to index on the fly..
    DEBUG(1, "Indexing SDCARD...");
    FF_GetFreeSize(pIoman, 0);

	usb_connect(ptp_recv);
}
void PTP_USB_Stop(void)
{
	usb_disconnect();
}
uint8_t PTP_USB_Poll(void)
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
/*

DBG: [usb/ptp_usb.c:58] --- RXSETUP:
DBG: [usb/ptp_usb.c:59] usd.bmRequestType = A1
DBG: [usb/ptp_usb.c:60] usd.bRequest      = 67
DBG: [usb/ptp_usb.c:61] usd.wValue        = 0000
DBG: [usb/ptp_usb.c:62] usd.wIndex        = 0000
DBG: [usb/ptp_usb.c:63] usd.wLength       = 0024
DBG: [usb/ptp_usb.c:58] --- RXSETUP:

*/
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
            uint32_t eps[3] = { EPTYPE_BULK_IN, EPTYPE_BULK_OUT, EPTYPE_INT_IN };
            if(CurrentConfiguration) {
                usb_setup_endpoints(eps, 3);
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
        default:
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
        mtp_packet_recv(UsbBuffer, UsbSoFarCount);
        UsbSoFarCount = 0;
    }
}

#endif // defined(AT91SAM7S256)
