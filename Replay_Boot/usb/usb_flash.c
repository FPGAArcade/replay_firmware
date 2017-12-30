//-----------------------------------------------------------------------------
// This is a driver for the UDP (USB Device Periphal) on the AT91SAM7{S,X}xxx
// chips. It appears as a generic HID device; this means that it will work
// without a kernel-mode driver under most operating systems.
//
// This is not very close to USB-compliant, but it is tested and working
// under Windows XP.
//
// To use this driver: from your code, call UsbStart() once at startup.
// After doing this, you can call UsbSendPacket() to transmit a packet
// from the device (this processor) to the host. Call UsbPoll()
// periodically to poll for received packets; when a packet is received
// it will be delivered to you by calling UsbPacketReceived(), a function
// that you provide. UsbPoll() also takes care of the signaling involved
// in enumeration.
//
// I assume that an appropriate clock has already been supplied to the UDP
// using the PMC. The exact register settings to do this will depend on
// your crystal frequency.
//
// Jonathan Westhues, split Aug 14 2005, public release May 26 2006
//-----------------------------------------------------------------------------
#if !defined(SAME70Q21)

#include "board.h"
#include "messaging.h"

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void UsbStart(void);
uint8_t UsbPoll(void);
void UsbSendPacket(uint8_t *packet, int len);
void UsbPacketReceived(uint8_t *data, int len);

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#define UDP_ENDPOINT_CSR(x)                 (AT91C_BASE_UDP->UDP_CSR[x])
#define UDP_ENDPOINT_FIFO(x)                (AT91C_BASE_UDP->UDP_FDR[x])
#define UDP_GLOBAL_STATE                    (AT91C_BASE_UDP->UDP_GLBSTATE)
#define UDP_FUNCTION_ADDR                   (AT91C_BASE_UDP->UDP_FADDR)
#define UDP_INTERRUPT_STATUS                (AT91C_BASE_UDP->UDP_ISR)
#define UDP_INTERRUPT_CLEAR                 (AT91C_BASE_UDP->UDP_ICR)
#define UDP_RESET_ENDPOINT                  (AT91C_BASE_UDP->UDP_RSTEP)

#define UDP_GLOBAL_STATE_ADDRESSED          (AT91C_UDP_FADDEN)
#define UDP_GLOBAL_STATE_CONFIGURED         (AT91C_UDP_CONFG)

#define UDP_FUNCTION_ADDR_ENABLED           (AT91C_UDP_FEN)

#define UDP_CSR_ENABLE_EP                   (AT91C_UDP_EPEDS)

#define UDP_INTERRUPT_ENDPOINT(x)           (1<<(x)) // AT91C_UDP_EPINTx

#define UDP_CSR_EPTYPE_CONTROL              (AT91C_UDP_EPTYPE_CTRL)
#define UDP_CSR_EPTYPE_INTERRUPT_OUT        (AT91C_UDP_EPTYPE_INT_OUT)
#define UDP_CSR_EPTYPE_INTERRUPT_IN         (AT91C_UDP_EPTYPE_INT_IN)

#define UDP_CSR_TX_PACKET_ACKED             (AT91C_UDP_TXCOMP) // 0
#define UDP_CSR_RX_HAVE_READ_SETUP_DATA     (AT91C_UDP_RXSETUP)  // 2
#define UDP_CSR_TX_PACKET                   (AT91C_UDP_TXPKTRDY) // 4
#define UDP_CSR_CONTROL_DATA_DIR            (AT91C_UDP_DIR) // 7

#define UDP_CSR_RX_PACKET_RECEIVED_BANK_0   (AT91C_UDP_RX_DATA_BK0)
#define UDP_CSR_RX_PACKET_RECEIVED_BANK_1   (AT91C_UDP_RX_DATA_BK1)

#define UDP_INTERRUPT_END_OF_BUS_RESET      (AT91C_UDP_ENDBUSRES)

#define UDP_CSR_BYTES_RECEIVED(x)           (((x) >> 16) & 0x7ff) // AT91C_UDP_RXBYTECNT = 0x7FF << 16


#define PIO_OUTPUT_ENABLE                   (AT91C_BASE_PIOA->PIO_OER)
#define PIO_OUTPUT_DATA_CLEAR               (AT91C_BASE_PIOA->PIO_CODR)
#define PIO_OUTPUT_DATA_SET                 (AT91C_BASE_PIOA->PIO_SODR)

#define GPIO_USB_PU                         (AT91C_PIO_PA16)

#define USB_D_PLUS_PULLUP_ON()              { \
                                                PIO_OUTPUT_DATA_CLEAR = (GPIO_USB_PU); \
                                                PIO_OUTPUT_ENABLE = (GPIO_USB_PU); \
                                            }
#define USB_D_PLUS_PULLUP_OFF()             { \
                                                PIO_OUTPUT_DATA_SET = (GPIO_USB_PU); \
                                                PIO_OUTPUT_ENABLE = (GPIO_USB_PU); \
                                            }

#define MC_FLASH_STATUS                     (*AT91C_MC_FSR)
#define MC_FLASH_COMMAND                    (*AT91C_MC_FCR)
#define FCMD_WRITE_PAGE                     (AT91C_MC_FCMD_START_PROG)

#define MC_FLASH_STATUS_READY               (AT91C_MC_FRDY)

#define MC_FLASH_COMMAND_PAGEN(x)           ((x)<<8) // AT91C_MC_PAGEN
#define MC_FLASH_COMMAND_KEY                ((0x5a)<<24) // AT91C_MC_KEY
#define FLASH_PAGE_SIZE_BYTES               256

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

typedef struct {
    uint32_t       cmd;
    uint32_t       ext1;
    uint32_t       ext2;
    uint32_t       ext3;
    union {
        uint8_t        asBytes[48];
        uint32_t       asDwords[12];
    } d;
} UsbCommand;

// For the bootloader
#define CMD_DEVICE_INFO                         0x0000
#define CMD_SETUP_WRITE                         0x0001
#define CMD_FINISH_WRITE                        0x0003
#define CMD_HARDWARE_RESET                      0x0004
#define CMD_ACK                                 0x00ff

void UsbPacketReceived(uint8_t *packet, int len)
{
    return; // disable flash operations

    int i;
    UsbCommand *c = (UsbCommand *)(void*)packet;
    volatile uint32_t *p;

    if(len != sizeof(*c)) {
        ERROR("size mismatch");
    }

    switch(c->cmd) {
        case CMD_DEVICE_INFO:
            break;

        case CMD_SETUP_WRITE:
            p = (volatile uint32_t *)0;
            for(i = 0; i < 12; i++) {
                p[i+c->ext1] = c->d.asDwords[i];
            }
            break;

        case CMD_FINISH_WRITE:
            p = (volatile uint32_t *)0;
            for(i = 0; i < 4; i++) {
                p[i+60] = c->d.asDwords[i];
            }

            MC_FLASH_COMMAND = MC_FLASH_COMMAND_KEY |
                MC_FLASH_COMMAND_PAGEN(c->ext1/FLASH_PAGE_SIZE_BYTES) |
                FCMD_WRITE_PAGE;
            while(!(MC_FLASH_STATUS & MC_FLASH_STATUS_READY))
                ;
            break;

        case CMD_HARDWARE_RESET:
            break;

        default:
            ERROR("unknown cmd");
            break;
    }

    c->cmd = CMD_ACK;
    UsbSendPacket(packet, len);
    
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%


#define min(a, b) (((a) > (b)) ? (b) : (a))

#define USB_REPORT_PACKET_SIZE 64

typedef struct PACKED {
    uint8_t        bmRequestType;
    uint8_t        bRequest;
    uint16_t        wValue;
    uint16_t        wIndex;
    uint16_t        wLength;
} UsbSetupData;


#define USB_REQUEST_GET_STATUS          0
#define USB_REQUEST_CLEAR_FEATURE       1
#define USB_REQUEST_SET_FEATURE         3
#define USB_REQUEST_SET_ADDRESS         5
#define USB_REQUEST_GET_DESCRIPTOR      6
#define USB_REQUEST_SET_DESCRIPTOR      7
#define USB_REQUEST_GET_CONFIGURATION   8
#define USB_REQUEST_SET_CONFIGURATION   9
#define USB_REQUEST_GET_INTERFACE       10
#define USB_REQUEST_SET_INTERFACE       11
#define USB_REQUEST_SYNC_FRAME          12

#define USB_DESCRIPTOR_TYPE_DEVICE              1
#define USB_DESCRIPTOR_TYPE_CONFIGURATION       2
#define USB_DESCRIPTOR_TYPE_STRING              3
#define USB_DESCRIPTOR_TYPE_INTERFACE           4
#define USB_DESCRIPTOR_TYPE_ENDPOINT            5
#define USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER    6
#define USB_DESCRIPTOR_TYPE_OTHER_SPEED_CONF    7
#define USB_DESCRIPTOR_TYPE_INTERFACE_POWER     8
#define USB_DESCRIPTOR_TYPE_HID                 0x21
#define USB_DESCRIPTOR_TYPE_HID_REPORT          0x22

#define USB_DEVICE_CLASS_HID                    0x03

// These descriptors are partially adapted from Jan Axelson's sample code,
// which appears to be itself adapted from Cypress's examples for their
// USB-enabled 8051s.

static const uint8_t HidReportDescriptor[] = {
    0x06, 0xA0,0xFF,            // Usage Page (vendor defined) FFA0
    0x09, 0x01,                 // Usage (vendor defined)
    0xA1, 0x01,                 // Collection (Application)
    0x09, 0x02,                 // Usage (vendor defined)
    0xA1, 0x00,                 // Collection (Physical)
    0x06, 0xA1,0xFF,            // Usage Page (vendor defined) 

    // The input report
    0x09, 0x03,                 // usage - vendor defined
    0x09, 0x04,                 // usage - vendor defined
    0x15, 0x80,                 // Logical Minimum (-128)
    0x25, 0x7F,                 // Logical Maximum (127)
    0x35, 0x00,                 // Physical Minimum (0)
    0x45, 0xFF,                 // Physical Maximum (255)
    0x75, 0x08,                 // Report Size (8)  (bits)
    0x95, 0x40,                 // Report Count (64)  (fields)
    0x81, 0x02,                 // Input (Data,Variable,Absolute)  

    // The output report
    0x09, 0x05,                 // usage - vendor defined
    0x09, 0x06,                 // usage - vendor defined
    0x15, 0x80,                 // Logical Minimum (-128)
    0x25, 0x7F,                 // Logical Maximum (127)
    0x35, 0x00,                 // Physical Minimum (0)
    0x45, 0xFF,                 // Physical Maximum (255)
    0x75, 0x08,                 // Report Size (8)  (bits)
    0x95, 0x40,                 // Report Count (64)  (fields)
    0x91, 0x02,                 // Output (Data,Variable,Absolute)

    0xC0,                       // End Collection

    0xC0,                       // End Collection
};

static const uint8_t DeviceDescriptor[] = {
    0x12,                       // Descriptor length (18 bytes)
    0x01,                       // Descriptor type (Device)
    0x10, 0x01,                 // Complies with USB Spec. Release 1.10
    0x00,                       // Class code (0)
    0x00,                       // Subclass code (0)
    0x00,                       // Protocol (No specific protocol)
    0x08,                       // Maximum packet size for Endpoint 0 (8 bytes)
    0xc5, 0x9a,                 // Vendor ID (random numbers)
    0x8f, 0x4b,                 // Product ID (random numbers)
    0x01, 0x00,                 // Device release number (0001)
    0x01,                       // Manufacturer string descriptor index
    0x02,                       // Product string descriptor index
    0x00,                       // Serial Number string descriptor index (None)
    0x01,                       // Number of possible configurations (1)
};

static const uint8_t ConfigurationDescriptor[] = {
    0x09,                       // Descriptor length (9 bytes)
    0x02,                       // Descriptor type (Configuration)
    0x29, 0x00,                 // Total data length (41 bytes)
    0x01,                       // Interface supported (1)
    0x01,                       // Configuration value (1)
    0x00,                       // Index of string descriptor (None)
    0x80,                       // Configuration (Bus powered)
    250,                        // Maximum power consumption (500mA)

    // Interface
    0x09,                       // Descriptor length (9 bytes)
    0x04,                       // Descriptor type (Interface)
    0x00,                       // Number of interface (0)
    0x00,                       // Alternate setting (0)
    0x02,                       // Number of interface endpoint (2)
    0x03,                       // Class code (HID)
    0x00,                       // Subclass code ()
    0x00,                       // Protocol code ()
    0x00,                       // Index of string()

    // Class
    0x09,                       // Descriptor length (9 bytes)
    0x21,                       // Descriptor type (HID)
    0x00, 0x01,                 // HID class release number (1.00)
    0x00,                       // Localized country code (None)
    0x01,                       // # of HID class dscrptr to follow (1)
    0x22,                       // Report descriptor type (HID)
    // Total length of report descriptor
    sizeof(HidReportDescriptor), 0x00,

    // endpoint 1
    0x07,                       // Descriptor length (7 bytes)
    0x05,                       // Descriptor type (Endpoint)
    0x01,                       // Encoded address (Respond to OUT)
    0x03,                       // Endpoint attribute (Interrupt transfer)
    0x08, 0x00,                 // Maximum packet size (8 bytes)
    0x01,                       // Polling interval (1 ms)

    // endpoint 2
    0x07,                       // Descriptor length (7 bytes)
    0x05,                       // Descriptor type (Endpoint)
    0x82,                       // Encoded address (Respond to IN)
    0x03,                       // Endpoint attribute (Interrupt transfer)
    0x08, 0x00,                 // Maximum packet size (8 bytes)
    0x01,                       // Polling interval (1 ms)
};

static const uint8_t StringDescriptor0[] = {
    0x04,                       // Length
    0x03,                       // Type is string
    0x09,                       // English
    0x04,                       //  US
};

// These string descriptors can be changed for the particular application.
// The string itself is Unicode, and the length is the total length of
// the descriptor, so it is the number of characters times two, plus two.

static const uint8_t StringDescriptor1[] = {
    26,             // Length
    0x03,           // Type is string
    'M', 0x00,
    'a', 0x00,
    'n', 0x00,
    'u', 0x00,
    'f', 0x00,
    'a', 0x00,
    'c', 0x00,
    't', 0x00,
    'u', 0x00,
    'r', 0x00,
    'e', 0x00,
    'r', 0x00,
};

static const uint8_t StringDescriptor2[] = {
    14,             // Length
    0x03,           // Type is string
    'D', 0x00,
    'e', 0x00,
    'v', 0x00,
    'i', 0x00,
    'c', 0x00,
    'e', 0x00,
};

static const uint8_t * const StringDescriptors[] = {
    StringDescriptor0,
    StringDescriptor1,
    StringDescriptor2,
};

// The buffer used to store packets received over USB while we are in the
// process of receiving them.
static uint8_t UsbBuffer[64];

// The number of characters received in the USB buffer so far.
static int  UsbSoFarCount;

static uint8_t CurrentConfiguration;

//-----------------------------------------------------------------------------
// Send a zero-length packet on EP0. This blocks until the packet has been
// transmitted and an ACK from the host has been received.
//-----------------------------------------------------------------------------
static void UsbSendZeroLength(void)
{
    UDP_ENDPOINT_CSR(0) |= UDP_CSR_TX_PACKET;

    while(!(UDP_ENDPOINT_CSR(0) & UDP_CSR_TX_PACKET_ACKED))
        ;

    UDP_ENDPOINT_CSR(0) &= ~UDP_CSR_TX_PACKET_ACKED;

    while(UDP_ENDPOINT_CSR(0) & UDP_CSR_TX_PACKET_ACKED)
        ;
}

//-----------------------------------------------------------------------------
// Send a STALL packet on EP0. This is needed when device doesn't support a
// control packet, the control request has failed, or the endpoint failed.
//-----------------------------------------------------------------------------
static void UsbSendStall(void)
{
    UDP_ENDPOINT_CSR(0) |= AT91C_UDP_FORCESTALL;
    while(!(UDP_ENDPOINT_CSR(0) & AT91C_UDP_STALLSENT))
        ;
    UDP_ENDPOINT_CSR(0) &= ~(AT91C_UDP_FORCESTALL | AT91C_UDP_STALLSENT);
    while(UDP_ENDPOINT_CSR(0) & (AT91C_UDP_FORCESTALL | AT91C_UDP_STALLSENT))
        ;
}

//-----------------------------------------------------------------------------
// Send a packet over EP0. This blocks until the packet has been transmitted
// and an ACK from the host has been received.
//-----------------------------------------------------------------------------
static void UsbSendEp0(const uint8_t *data, int size, int wLength)
{
    int thisTime, i;

    int len = min(size, wLength);
    uint8_t needzlp = ((len & 0x7) == 0x00) && len != wLength;

    do {
        thisTime = min(len, 8);
        len -= thisTime;

        for(i = 0; i < thisTime; i++) {
            UDP_ENDPOINT_FIFO(0) = *data;
            data++;
        }

        if(UDP_ENDPOINT_CSR(0) & UDP_CSR_TX_PACKET_ACKED) {
            UDP_ENDPOINT_CSR(0) &= ~UDP_CSR_TX_PACKET_ACKED;
            while(UDP_ENDPOINT_CSR(0) & UDP_CSR_TX_PACKET_ACKED)
                ;
        }

        UDP_ENDPOINT_CSR(0) |= UDP_CSR_TX_PACKET;

        do {
            if(UDP_ENDPOINT_CSR(0) & UDP_CSR_RX_PACKET_RECEIVED_BANK_0) {
                // This means that the host is trying to write to us, so
                // abandon our write to them.
                UDP_ENDPOINT_CSR(0) &= ~UDP_CSR_RX_PACKET_RECEIVED_BANK_0;
                return;
            }
        } while(!(UDP_ENDPOINT_CSR(0) & UDP_CSR_TX_PACKET_ACKED));
    } while(len > 0);

    if(UDP_ENDPOINT_CSR(0) & UDP_CSR_TX_PACKET_ACKED) {
        UDP_ENDPOINT_CSR(0) &= ~UDP_CSR_TX_PACKET_ACKED;
        while(UDP_ENDPOINT_CSR(0) & UDP_CSR_TX_PACKET_ACKED)
            ;
    }

    // The HOST can't determine the actual data length if the size is a multiple of the packet size (8).
    // This is only needed when the data size is less than what the HOST 'expects'.
    if (needzlp)
        UsbSendZeroLength();
}

//-----------------------------------------------------------------------------
// Handle a received SETUP DATA packet. These are the packets used to
// configure the link (e.g. request various descriptors, and assign our
// address).
//-----------------------------------------------------------------------------
static void HandleRxdSetupData(void)
{
    int i;
    UsbSetupData usd;

    for(i = 0; i < sizeof(usd); i++) {
        ((uint8_t *)&usd)[i] = UDP_ENDPOINT_FIFO(0);
    }

    if(usd.bmRequestType & 0x80) {
        UDP_ENDPOINT_CSR(0) |= UDP_CSR_CONTROL_DATA_DIR;
        while(!(UDP_ENDPOINT_CSR(0) & UDP_CSR_CONTROL_DATA_DIR))
            ;
    }

    UDP_ENDPOINT_CSR(0) &= ~UDP_CSR_RX_HAVE_READ_SETUP_DATA;
    while(UDP_ENDPOINT_CSR(0) & UDP_CSR_RX_HAVE_READ_SETUP_DATA)
        ;

    switch(usd.bRequest) {
        case USB_REQUEST_GET_DESCRIPTOR:
            if((usd.wValue >> 8) == USB_DESCRIPTOR_TYPE_DEVICE) {
                UsbSendEp0((uint8_t *)&DeviceDescriptor,
                    sizeof(DeviceDescriptor), usd.wLength);
            } else if((usd.wValue >> 8) == USB_DESCRIPTOR_TYPE_CONFIGURATION) {
                UsbSendEp0((uint8_t *)&ConfigurationDescriptor,
                    sizeof(ConfigurationDescriptor), usd.wLength);
            } else if((usd.wValue >> 8) == USB_DESCRIPTOR_TYPE_STRING) {
                const uint8_t *s = StringDescriptors[usd.wValue & 0xff];
                if ((usd.wValue & 0xff) < sizeof(StringDescriptors) / sizeof(StringDescriptors[0]))
                    UsbSendEp0(s, s[0], usd.wLength);
                else
                    UsbSendStall();
            } else if((usd.wValue >> 8) == USB_DESCRIPTOR_TYPE_HID_REPORT) {
                UsbSendEp0((uint8_t *)&HidReportDescriptor,
                    sizeof(HidReportDescriptor), usd.wLength);
            } else
                UsbSendStall();
            break;

        case USB_REQUEST_SET_ADDRESS:
            UsbSendZeroLength();
            UDP_FUNCTION_ADDR = UDP_FUNCTION_ADDR_ENABLED | usd.wValue ;
            if(usd.wValue != 0) {
                UDP_GLOBAL_STATE = UDP_GLOBAL_STATE_ADDRESSED;
            } else {
                UDP_GLOBAL_STATE = 0;
            }
            break;

        case USB_REQUEST_GET_CONFIGURATION:
            UsbSendEp0(&CurrentConfiguration, sizeof(CurrentConfiguration), usd.wLength);
            break;

        case USB_REQUEST_GET_STATUS: {
            if(usd.bmRequestType & 0x80) {
                uint16_t w = 0;
                UsbSendEp0((uint8_t *)&w, sizeof(w), usd.wLength);
            }
            break;
        }
        case USB_REQUEST_SET_CONFIGURATION:
            CurrentConfiguration = usd.wValue;
            if(CurrentConfiguration) {
                UDP_GLOBAL_STATE = UDP_GLOBAL_STATE_CONFIGURED;
                UDP_ENDPOINT_CSR(1) = UDP_CSR_ENABLE_EP |
                    UDP_CSR_EPTYPE_INTERRUPT_OUT;
                UDP_ENDPOINT_CSR(2) = UDP_CSR_ENABLE_EP |
                    UDP_CSR_EPTYPE_INTERRUPT_IN;
            } else {
                UDP_GLOBAL_STATE = UDP_GLOBAL_STATE_ADDRESSED;
                UDP_ENDPOINT_CSR(1) = 0;
                UDP_ENDPOINT_CSR(2) = 0;
            }
            UsbSendZeroLength();
            break;

        case USB_REQUEST_GET_INTERFACE: {
            uint8_t b = 0;
            UsbSendEp0(&b, sizeof(b), usd.wLength);
            break;
        }

        case USB_REQUEST_SET_INTERFACE:
            UsbSendZeroLength();
            break;
            
        case USB_REQUEST_CLEAR_FEATURE:
        case USB_REQUEST_SET_FEATURE:
        case USB_REQUEST_SET_DESCRIPTOR:
        case USB_REQUEST_SYNC_FRAME:
        default:
            UsbSendStall();
            break;
    }
}

//-----------------------------------------------------------------------------
// Send a data packet. This packet should be exactly USB_REPORT_PACKET_SIZE
// long. This function blocks until the packet has been transmitted, and
// an ACK has been received from the host.
//-----------------------------------------------------------------------------
void UsbSendPacket(uint8_t *packet, int len)
{
    int i, thisTime;

    while(len > 0) {
        thisTime = min(len, 8);
       
        for(i = 0; i < thisTime; i++) {
            UDP_ENDPOINT_FIFO(2) = packet[i];
        }
        UDP_ENDPOINT_CSR(2) |= UDP_CSR_TX_PACKET;

        while(!(UDP_ENDPOINT_CSR(2) & UDP_CSR_TX_PACKET_ACKED))
            ;
        UDP_ENDPOINT_CSR(2) &= ~UDP_CSR_TX_PACKET_ACKED;

        while(UDP_ENDPOINT_CSR(2) & UDP_CSR_TX_PACKET_ACKED)
            ;

        len -= thisTime;
        packet += thisTime;
    }
}

//-----------------------------------------------------------------------------
// Handle a received packet. This handles only those packets received on
// EP1 (i.e. the HID reports that we use as our data packets).
//-----------------------------------------------------------------------------
static void HandleRxdData(void)
{
    int i, len;

    if(UDP_ENDPOINT_CSR(1) & UDP_CSR_RX_PACKET_RECEIVED_BANK_0) {
        len = UDP_CSR_BYTES_RECEIVED(UDP_ENDPOINT_CSR(1));

        for(i = 0; i < len; i++) {
            UsbBuffer[UsbSoFarCount] = UDP_ENDPOINT_FIFO(1);
            UsbSoFarCount++;
        }

        UDP_ENDPOINT_CSR(1) &= ~UDP_CSR_RX_PACKET_RECEIVED_BANK_0;
        while(UDP_ENDPOINT_CSR(1) & UDP_CSR_RX_PACKET_RECEIVED_BANK_0)
            ;

        if(UsbSoFarCount >= 64) {
            UsbPacketReceived(UsbBuffer, UsbSoFarCount);
            UsbSoFarCount = 0;
        }
    }

    if(UDP_ENDPOINT_CSR(1) & UDP_CSR_RX_PACKET_RECEIVED_BANK_1) {
        len = UDP_CSR_BYTES_RECEIVED(UDP_ENDPOINT_CSR(1));

        for(i = 0; i < len; i++) {
            UsbBuffer[UsbSoFarCount] = UDP_ENDPOINT_FIFO(1);
            UsbSoFarCount++;
        }

        UDP_ENDPOINT_CSR(1) &= ~UDP_CSR_RX_PACKET_RECEIVED_BANK_1;
        while(UDP_ENDPOINT_CSR(1) & UDP_CSR_RX_PACKET_RECEIVED_BANK_1)
            ;

        if(UsbSoFarCount >= 64) {
            UsbPacketReceived(UsbBuffer, UsbSoFarCount);
            UsbSoFarCount = 0;
        }
    }
}

//-----------------------------------------------------------------------------
// Initialize the USB driver. To be thorough, disconnect the pull-up on
// D+ (to make the host think that we have been unplugged, resetting all
// of its state) before reconnecting it and resetting the AT91's USB
// peripheral.
//-----------------------------------------------------------------------------
void UsbStart(void)
{
    volatile int i;

    UsbSoFarCount = 0;

    // take care the optimizer does not remove it!
    for(i = 0; i < 1000000; i++) USB_D_PLUS_PULLUP_OFF();

    USB_D_PLUS_PULLUP_ON();

    if(UDP_INTERRUPT_STATUS & UDP_INTERRUPT_END_OF_BUS_RESET) {
        UDP_INTERRUPT_CLEAR = UDP_INTERRUPT_END_OF_BUS_RESET;
    }
}

//-----------------------------------------------------------------------------
// This must be called periodically in order for USB to work. UsbPoll may
// call UsbPacketReceived if a non-control packet has been received, and
// UsbPoll should not be called from UsbPacketReceived.
//
// Returns TRUE if we `did something' (received either a control or data
// packet, or a piece of one at least), FALSE otherwise.
//-----------------------------------------------------------------------------
uint8_t UsbPoll(void)
{
    uint8_t ret = FALSE;

    if(UDP_INTERRUPT_STATUS & UDP_INTERRUPT_END_OF_BUS_RESET) {
        UDP_INTERRUPT_CLEAR = UDP_INTERRUPT_END_OF_BUS_RESET;

        // following a reset we should be ready to receive a setup packet
        UDP_RESET_ENDPOINT = 0xf;
        UDP_RESET_ENDPOINT = 0;

        UDP_FUNCTION_ADDR = UDP_FUNCTION_ADDR_ENABLED;

        UDP_ENDPOINT_CSR(0) = UDP_CSR_EPTYPE_CONTROL | UDP_CSR_ENABLE_EP;

        CurrentConfiguration = 0;

        ret = TRUE;
    }

    if(UDP_INTERRUPT_STATUS & UDP_INTERRUPT_ENDPOINT(0)) {
        if(UDP_ENDPOINT_CSR(0) & UDP_CSR_RX_HAVE_READ_SETUP_DATA) {
            HandleRxdSetupData();
            ret = TRUE;
        }
    }

    if(UDP_INTERRUPT_STATUS & UDP_INTERRUPT_ENDPOINT(1)) {
        HandleRxdData();
        ret = TRUE;
    }

    return ret;
}

#endif // !defined(SAME70Q21)
