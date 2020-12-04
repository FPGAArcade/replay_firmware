// ADD REPLAY LICENSE!

#include "usbblaster.h"

#if 0	// Enable USBBlaster!

#define OFFICAL_API 0

#include "Blaster.h"
#include "USB/SAMD21_USBDevice.h"

extern uint32_t EndPoints[];

class UnpluggableModule : public PluggableUSBModule
{
    friend class UnpluggableUSB;
};

class UnpluggableUSB
{
  public:
    bool plug(PluggableUSBModule* _node)
    {
        UnpluggableModule* node = (UnpluggableModule*)_node;
        UnpluggableModule* current = (UnpluggableModule*)rootNode;

        node->next = rootNode;
        rootNode = node;

        node->pluggedInterface = 0;
        node->pluggedEndpoint = 1;

        lastIf += node->numInterfaces;
        lastEp += node->numEndpoints;

        for (uint8_t i = lastEp - 1; i > node->numEndpoints; i--) {
            int prev = i - node->numEndpoints;
            uint32_t ep = prev > 0 ? EndPoints[prev] : 0;
            Serial1.print("UnpluggableBlaster : copy ");
            Serial1.print(prev);
            Serial1.print("(");
            Serial1.print(ep, HEX);
            Serial1.print(") to ");
            Serial1.println(i);
            EndPoints[i] = ep;
        }


        for (uint8_t i = 0; i < node->numEndpoints; i++) {
            EndPoints[i + node->pluggedEndpoint] = node->endpointType[i];
        }

        for (UnpluggableModule* p = (UnpluggableModule*)node->next; p; p = (UnpluggableModule*)p->next) {
            p->pluggedInterface += node->numInterfaces;
            p->pluggedEndpoint += node->numEndpoints;
        }

        return true;
    }

    bool unplug(PluggableUSBModule* _node)
    {
        UnpluggableModule* node = (UnpluggableModule*)_node;
        UnpluggableModule* current = (UnpluggableModule*)rootNode;

        Serial1.print("UnpluggableBlaster : node->pluggedInterface = ");
        Serial1.println(node->pluggedInterface);
        Serial1.print("UnpluggableBlaster : node->pluggedEndpoint = ");
        Serial1.println(node->pluggedEndpoint);

        Serial1.print("UnpluggableBlaster : node->numInterfaces = ");
        Serial1.println(node->numInterfaces);
        Serial1.print("UnpluggableBlaster : node->numEndpoints = ");
        Serial1.println(node->numEndpoints);

        if (rootNode == node) {
            rootNode = node->next;

        } else {
            while (current) {
                if (current->next == node) {
                    current->next = node->next;
                }

                break;
            }

            if (!current) {
                return false;
            }
        }

        current = (UnpluggableModule*)node->next;
        node->next = 0;

        Serial1.print("UnpluggableBlaster : current = ");
        Serial1.println((int)current, HEX);


        for (UnpluggableModule* p = current; p; p = (UnpluggableModule*)p->next) {
            p->pluggedInterface -= node->numInterfaces;
            p->pluggedEndpoint -= node->numEndpoints;
        }

        lastIf -= node->numInterfaces;
        lastEp -= node->numEndpoints;

        for (uint8_t i = node->pluggedEndpoint; i < lastEp; i++) {
            int next = i + node->numEndpoints;
            uint32_t ep = next < USB_ENDPOINTS ? EndPoints[next] : 0;
            Serial1.print("UnpluggableBlaster : copy ");
            Serial1.print(next);
            Serial1.print("(");
            Serial1.print(ep, HEX);
            Serial1.print(") to ");
            Serial1.println(i);
            EndPoints[i] = ep;
        }

        node->pluggedInterface = 0;
        node->pluggedEndpoint = 0;


        return true;
    }

    void printChain()
    {
        Serial1.print("UnpluggableBlaster : lastIf = ");
        Serial1.println(lastIf);
        Serial1.print("UnpluggableBlaster : lastEp = ");
        Serial1.println(lastEp);

        Serial1.println("UnpluggableBlaster : endpoints:");

        for (int i = 0; i < lastEp; ++i) {
            Serial1.print("UnpluggableBlaster : EndPoints[");
            Serial1.print(i);
            Serial1.print("] = ");
            Serial1.println(EndPoints[i], HEX);
        }

        Serial1.println("UnpluggableBlaster : modules:");

        for (UnpluggableModule* p = (UnpluggableModule*)rootNode; p; p = (UnpluggableModule*)p->next) {
            char buf[256];
            uint8_t len = p->getShortName(buf);
            buf[len] = 0;
            Serial1.print("UnpluggableBlaster : ");
            Serial1.print(buf);
            Serial1.print(" = ");
            Serial1.print((int)p, HEX);
            Serial1.print(", node->pluggedInterface = ");
            Serial1.print(p->pluggedInterface);
            Serial1.print(", node->pluggedEndpoint = ");
            Serial1.println(p->pluggedEndpoint);
        }

        Serial1.println("UnpluggableBlaster : :done");
    }

    uint8_t lastIf;
    uint8_t lastEp;
    PluggableUSBModule* rootNode;
};


extern USBDevice_SAMD21G18x usbd;

static bool Blaster_Enabled = true;
static bool Blaster_Inited = false;

static void killUSB()
{
#ifdef CDC_ENABLED
    SerialUSB.end();
#endif
    NVIC_DisableIRQ((IRQn_Type) USB_IRQn);
    USBDevice.detach();
    usbd.disableEndOfResetInterrupt();
    usbd.disableStartOfFrameInterrupt();
    usbd.ackEndOfResetInterrupt();
    usbd.ackStartOfFrameInterrupt();

    for (int ep = 0; ep < USB_EPT_NUM; ep++) {
        usbd.epAckPendingInterrupts(ep);
        usbd.epReleaseOutBank0(ep, 64);
    }

    usbd.disable();
    usbd.reset();
}

static void waitForUSBConfig()
{
    HARDWARE_TICK current = Timer_Get(0);
    int retry = 5;

    while (!USBDevice.configured()) {
        if (Timer_Check(current)) {
            Serial1.println("UnpluggableBlaster : waiting for usb config");
            current = Timer_Get(1000);

            if (!retry--) {
                Serial1.println("UnpluggableBlaster : giving up..");
                break;
            }
        }
    }
}

extern "C" void USBBlaster_Disable()
{
    if (!Blaster_Enabled) {
        return;
    }

#if OFFICAL_API
    USBDevice.detach();
#else

    UnpluggableUSB* unpluggable = (UnpluggableUSB*)&PluggableUSB();

    if (sizeof(UnpluggableUSB) != sizeof(PluggableUSB_)) {
        Serial1.println("UnpluggableBlaster : class size mismatch!");
    }

    if (sizeof(UnpluggableModule) != sizeof(PluggableUSBModule)) {
        Serial1.println("UnpluggableBlaster : class size mismatch!");
    }

    unpluggable->printChain();

    Serial1.println("UnpluggableBlaster : detaching");
    killUSB();


    Serial1.println("UnpluggableBlaster : unplugging");

    if (!unpluggable->unplug(&USBBlaster)) {
        Serial1.println("UnpluggableBlaster : unplugging failed");
    }

    Serial1.println("UnpluggableBlaster : attaching");
    unpluggable->printChain();

    USBDevice.init();
    USBDevice.attach();
    waitForUSBConfig();

#endif
    Blaster_Enabled = false;
    //    Blaster_Inited = false;

    Serial1.flush();
}

extern "C" void USBBlaster_EnableAndInit()
{
    if (!Blaster_Enabled) {

#if OFFICAL_API
        USBDevice.attach();
        waitForUSBConfig();
#else
        UnpluggableUSB* unpluggable = (UnpluggableUSB*)&PluggableUSB();

        Serial1.println("UnpluggableBlaster : detaching");
        killUSB();

        Serial1.println("UnpluggableBlaster : plugging");

        if (!unpluggable->plug(&USBBlaster)) {
            Serial1.println("UnpluggableBlaster : plugging failed");
        }

        Serial1.println("UnpluggableBlaster : attaching");
        unpluggable->printChain();

        USBDevice.init();
        USBDevice.attach();
        waitForUSBConfig();

#endif
        Blaster_Enabled = true;
    }

    if (!Blaster_Inited) {

        Serial1.println("usbblaster.begin()");

        USBBlaster.begin(true);
        Blaster_Inited = true;
    }

    Serial1.flush();
}

extern "C" void USBBlaster_Update()
{
    extern uint8_t pin_fpga_done;

    if (!Blaster_Enabled || !Blaster_Inited) {
        return;
    }

    if (!USBDevice.configured()) {
        return;
    }

    int ret = USBBlaster.loop();

    static HARDWARE_TICK last_action;

    if (ret) {
        last_action = Timer_Get(5000);
    }

    if (pin_fpga_done && ret != 0) {
        DEBUG(1, "USBBlaster is active!");
        pin_fpga_done = 0;

    } else if (!pin_fpga_done && ret == 0 && Timer_Check(last_action)) {
        DEBUG(1, "USBBlaster is inactive!");
        pin_fpga_done = 1;
    }

}

#else

extern "C" void USBBlaster_Disable() {}
extern "C" void USBBlaster_EnableAndInit() {}
extern "C" void USBBlaster_Update() {}

#endif
