#include "USB/SAMD21_USBDevice.h"

#define USB_NUM_ENDPOINTS (2)
#define USB_NUM_INTERFACE (1)
#define USB_EP_BULK_IN   (this->pluggedEndpoint)
#define USB_EP_BULK_OUT    (this->pluggedEndpoint+1)

extern "C" {
#include "usb/usb_hardware.h"
}

// These endpoints are fixed in 'msc.c'..
#define FIXED_EP_CTRL     (0)
#define FIXED_EP_BULK_IN  (1)
#define FIXED_EP_BULK_OUT (2)

extern USBDevice_SAMD21G18x usbd;
static uint8_t buffer[EPX_SIZE] __attribute__((aligned(16)));     // 64 bytes == max endpoint size with USB 2.0 full-speed

class MSCHardware : public PluggableUSBModule
{
  public:
    MSCHardware(USBDeviceClass& _usb)
        : PluggableUSBModule(USB_NUM_ENDPOINTS, USB_NUM_INTERFACE, epType), usb(_usb), recv_func(NULL)
    {
        epType[0] = USB_ENDPOINT_TYPE_BULK | USB_ENDPOINT_IN(0);
        epType[1] = USB_ENDPOINT_TYPE_BULK | USB_ENDPOINT_OUT(0);
        bool ret = PluggableUSB().plug(this);
    }

    void begin(usb_func _recv)
    {
        USBDevice.detach();
        Timer_Wait(500);
        recv_func = _recv;
        USBDevice.attach();
        Timer_Wait(100);
    }

    void end(void)
    {
        if (!recv_func) {
            return;
        }

        begin(NULL);
    }

    uint8_t poll()
    {
        if ( !USBDevice.configured() || !recv_func ) {
            return 0;
        }

        uint32_t ep0size = USBDevice.available(EP0);

        if (ep0size) {
            USBDevice.recv(EP0, buffer, ep0size);
            recv_func(FIXED_EP_CTRL, buffer, ep0size);
        }

        uint32_t bytecnt = USBDevice.available(USB_EP_BULK_OUT);

        if (bytecnt == 0) {
            return 0;
        }

        if (bytecnt > sizeof(buffer)) {
            ERROR("USB:EP too big!");
            return 0;
        }

        USBDevice.recv(USB_EP_BULK_OUT, buffer, bytecnt);
        recv_func(FIXED_EP_BULK_OUT, buffer, bytecnt);

        return 1;
    }

    void send(uint8_t ep, const uint8_t* packet, uint32_t packet_length, uint32_t wLength)
    {
        if (!recv_func) {
            return;
        }

        if (ep == FIXED_EP_CTRL) {
            int len = wLength ? min(packet_length, wLength) : packet_length;
            USBDevice.sendControl(packet, len);

        } else if (ep == FIXED_EP_BULK_IN) {

            // if packet is not aligned, or resides in ROM, we copy it
            if ((size_t)packet & 0xf || ((size_t)packet & 0xff000000) == 0 ) {
                Assert(packet_length <= sizeof(buffer));
                memcpy(buffer, packet, min(packet_length, sizeof(buffer)));
                packet = buffer;
            }

            ep_t ep = USB_EP_BULK_IN;

            usbd.epBank1SetAddress(ep, (void*)packet);

            usbd.epBank1SetMultiPacketSize(ep, 0);
            usbd.epBank1SetByteCount(ep, packet_length);

            usbd.epBank1AckTransferComplete(ep);
            usbd.epBank1SetReady(ep);

            HARDWARE_TICK timeout = Timer_Get(100);

            while (usbd.epBank1IsReady(ep)) {
                if (Timer_Check(timeout)) {
                    WARNING("USB:TX Timeout!");

                    break;
                }
            }

        } else {
            ERROR("USB: wrong EP for TX!");
        }
    }

    uint32_t recv(uint8_t ep, uint8_t* packet, uint32_t length)
    {
        if (!recv_func) {
            return 0;
        }

        if (ep != FIXED_EP_BULK_OUT) {
            ERROR("USB: wrong EP for RX!");
            return 0;
        }

        uint32_t read = 0;

        HARDWARE_TICK timeout = Timer_Get(100);

        while (length) {
            if (Timer_Check(timeout)) {
                WARNING("USB:RX Timeout!");

                break;
            }

            uint32_t bytecnt = USBDevice.available(USB_EP_BULK_OUT);

            if (!bytecnt) {
                continue;
            }

            if (bytecnt > length) {
                bytecnt = length;
            }

            USBDevice.recv(USB_EP_BULK_OUT, packet, bytecnt);

            packet += bytecnt;
            length -= bytecnt;
            read += bytecnt;
        }

        return read;
    }

    void stall(uint8_t ep)
    {
        if (!recv_func) {
            return;
        }

        if (ep == FIXED_EP_BULK_IN) {
            ep = USB_EP_BULK_IN;

        } else if (ep == FIXED_EP_BULK_OUT) {
            ep = USB_EP_BULK_OUT;

        } else if (ep != FIXED_EP_CTRL) {
            ERROR("USB:stalled wrong EP!");
            return;
        }

        USBDevice.stall(ep);
    }

  protected:
    bool setup(USBSetup& setup)
    {
        if (!recv_func) {
            return false;
        }

        if (pluggedInterface != setup.wIndex) {
            return false;
        }

        recv_func(FIXED_EP_CTRL, (uint8_t*)(void*)&setup, sizeof(setup));
        return true;
    }
    int getInterface(uint8_t* interfaceCount)
    {
        if (!recv_func) {
            return 0;
        }

        MSCDescriptor desc = {
            D_INTERFACE(pluggedInterface, USB_NUM_ENDPOINTS,
            USB_DEVICE_CLASS_STORAGE, MSC_SUBCLASS_SCSI, MSC_PROTOCOL_BULK_ONLY),
            D_ENDPOINT(USB_ENDPOINT_IN (USB_EP_BULK_IN),  USB_ENDPOINT_TYPE_BULK, EPX_SIZE, 0),
            D_ENDPOINT(USB_ENDPOINT_OUT(USB_EP_BULK_OUT), USB_ENDPOINT_TYPE_BULK, EPX_SIZE, 0)
        };

        *interfaceCount += USB_NUM_INTERFACE;

        return USBDevice.sendControl(&desc, sizeof(desc));
    }

    int getDescriptor(USBSetup& setup)
    {
        return 0;
    }

    uint8_t getShortName(char* name)
    {
        if (!recv_func) {
            return 0;
        }

        name[0] = 'm';
        name[1] = 's';
        name[2] = 'd';
        return 3;
    }

  private:

    USBDeviceClass& usb;
    unsigned int epType[2];
    usb_func recv_func;
};

static MSCHardware usbhw(USBDevice);

extern "C" void usb_connect(usb_func _recv)
{
    usbhw.begin(_recv);
}
extern "C" void usb_disconnect()
{
    usbhw.end();
}

extern "C" uint8_t usb_poll()
{
    return usbhw.poll();
}

extern "C" void usb_send(uint8_t ep, uint32_t wMaxPacketSize, const uint8_t* packet, uint32_t packet_length, uint32_t wLength)
{
    usbhw.send(ep, packet, packet_length, wLength);
}

extern "C" uint32_t usb_recv(uint8_t ep, uint8_t* packet, uint32_t length)
{
    return usbhw.recv(ep, packet, length);
}

extern "C" void usb_send_ep0_stall(void)
{
    usb_send_stall(FIXED_EP_CTRL);
}

extern "C" void usb_send_stall(uint8_t ep)
{
    usbhw.stall(ep);
}

// Unsupported
extern "C" void usb_send_async(uint8_t ep, usb_func _send, uint32_t length)
{
    WARNING(__FUNCTION__);
}
extern "C" void usb_setup_faddr(uint16_t wValue)
{
    WARNING(__FUNCTION__);
}
extern "C" void usb_setup_endpoints(uint32_t* ep_types, uint32_t num_eps)
{
    WARNING(__FUNCTION__);
}
