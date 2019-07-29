#include "usb_host/conf_usb_host.h"

#define MAX_SCANCODES (1<<4)
#define MAX_SCANCODE_LENGTH (15+1)

class ReplayKeyboard
{
    enum Meta {
        Meta_None		= 0,
        Meta_LeftCtrl	= 1 << 0,
        Meta_LeftShift	= 1 << 1,
        Meta_LeftAlt	= 1 << 2,
        Meta_LeftCmd	= 1 << 3,
        Meta_RightCtrl	= 1 << 4,
        Meta_RightShift	= 1 << 5,
        Meta_RightAlt	= 1 << 6,		// AltGr
        Meta_RightCmd	= 1 << 7
    };

  public:
    ReplayKeyboard() :
        m_Put(0), m_Get(0)
    {
    };

    const char* PopKey()
    {
        if (((m_Put - m_Get) & (MAX_SCANCODES - 1)) == 0) {
            return 0;
        }

        return m_ScanCodes[m_Get++ & (MAX_SCANCODES - 1)];
    }

    void OnControlKeysChanged(uint8_t before, uint8_t after)
    {
        char buffer[8];
        uint8_t updated = before ^ after;

        const int numMetas = 8;

        for (int meta = 0; meta < numMetas; ++meta) {
            if ((updated & (1 << meta)) == 0) {
                continue;
            }

            bool keyDown = (after & (1 << meta));
            uint8_t ps2 = GetPS2Meta(meta);

            if (!ps2) {
                continue;
            }

            ExpandPS2(ps2, keyDown, buffer);
            PushKey(buffer);
            DEBUG(2, "[%s] META [%02x => %02x => %02x] %02x => %02x %02x %02x", __FUNCTION__, before, after, updated, (1 << meta), buffer[0], buffer[1], buffer[2]);
        }
    }
    void OnKeyDown(uint8_t mod, uint8_t key)
    {
        char buffer[8];

        if (key == 0x48) {	// pause/break
            const char ps2_break[] = { 0xE0, 0x7E, 0xE0, 0xF0, 0x7E, 0x00 };
            const char ps2_pause[] = { 0xE1, 0x14, 0x77, 0xE1, 0xF0, 0x14, 0xF0, 0x77, 0x00 };
            const bool ctrl = mod & (Meta_LeftCtrl | Meta_RightCtrl);
            PushKey(ctrl ? ps2_break : ps2_pause);
            return;
        }

        uint8_t ps2 = GetPS2(key);

        if (!ps2) {
            DEBUG(2, "[%s] OEM %02x Unknown", __FUNCTION__, key);
            return;
        }

        ExpandPS2(ps2, true, buffer);
        PushKey(buffer);
        DEBUG(2, "[%s] OEM %02x => %02x %02x %02x", __FUNCTION__, key, buffer[0], buffer[1], buffer[2]);
    }
    void OnKeyUp(uint8_t mod, uint8_t key)
    {
        (void)mod;
        char buffer[8];
        uint8_t ps2 = GetPS2(key);

        if (!ps2) {
            DEBUG(2, "[%s] OEM %02x Unknown", __FUNCTION__, key);
            return;
        }

        ExpandPS2(ps2, false, buffer);
        PushKey(buffer);
        DEBUG(2, "[%s] OEM %02x => %02x %02x %02x", __FUNCTION__, key, buffer[0], buffer[1], buffer[2]);
    }

  protected:
    void PushKey(const char* scancode)
    {
        if (((m_Put - m_Get) & (MAX_SCANCODES - 1)) == 15) {
            return;    // key dropped
        }

        char* p = m_ScanCodes[m_Put++ & (MAX_SCANCODES - 1)];
        strncpy(p, scancode, MAX_SCANCODE_LENGTH);
        p[MAX_SCANCODE_LENGTH - 1] = '\0';
    }

    uint8_t GetPS2(uint8_t oem)
    {
        if (oem > sizeof(OEMtoPS2)) {
            return 0;
        }

        return OEMtoPS2[oem];
    }

    uint8_t GetPS2Meta(uint8_t meta)
    {
        if (meta > sizeof(OEMtoPS2meta)) {
            return 0;
        }

        return OEMtoPS2meta[meta];
    }

    void ExpandPS2(uint8_t scan, bool make, char* buffer)
    {
        uint32_t offset = 0;
        bool extended = scan & 0x80;
        scan &= 0x7f;

        if (extended) {
            buffer[offset++] = 0xe0;
        }

        if (!make) {
            buffer[offset++] = 0xf0;
        }

        buffer[offset++] = scan;
        buffer[offset] = '\0';
    }

  private:
    uint32_t	m_Put, m_Get;
    char		m_ScanCodes[MAX_SCANCODES][MAX_SCANCODE_LENGTH];

    static const uint8_t OEMtoPS2[0x74];
    static const uint8_t OEMtoPS2meta[8];
};

class ReplayMouse
{
public:
    ReplayMouse() :
        mouseX(0),
        mouseY(0),
        mouseZ(0),
        mouseButtons(0),
        updated(false)
    {
    }

    void SendMouseUpdate()
    {
        disableIRQ();

        uint16_t mX = mouseX;
        uint16_t mY = mouseY;
        uint16_t mZ = mouseZ;
        uint8_t mButtons = mouseButtons; 
        bool refresh = updated;
        updated = false;

        enableIRQ();

        if (!refresh) {
            return;
        }

        DEBUG(2, "%s : %d/%d/%d ; %x", __FUNCTION__, mouseX, mouseY, mouseZ, mouseButtons);
        SPI_EnableOsd();
        rSPI(OSDCMD_SENDMOUSE);
        rSPI(mX & 0xff);    // x low
        rSPI(mX >> 8);      // x high
        rSPI(mY & 0xff);    // y low
        rSPI(mY >> 8);      // y high
        rSPI(mZ & 0xff);    // z low
        rSPI(mZ >> 8);      // z high
        rSPI(mButtons);     // buttons
        Timer_Wait(1);
        SPI_DisableOsd();
    }
    void OnMouseMove(int8_t dX, int8_t dY, int8_t dZ)
    {
        mouseX += dX;
        mouseY += dY;
        mouseZ += dZ;
        updated = true;
    }
    void OnMouseButtonChanged(uint8_t button, uint8_t down)
    {
        uint8_t mask = 1 << button;
        if (down)
            mouseButtons |= mask;
        else
            mouseButtons &= ~mask;

        updated = true;
    }

private:
    volatile uint16_t    mouseX, mouseY, mouseZ;
    volatile uint8_t     mouseButtons;
    volatile bool        updated;        // $TODO make this a ring buffer instead
};

static ReplayKeyboard keyboard;
static ReplayMouse mouse;

extern "C" {

void USB_Connection_Event(uhc_device_t *dev, bool b_present)
{
    (void)dev;
    (void)b_present;
    kprintf("%s\n\r", __FUNCTION__);
}

void USB_Enum_Event(uhc_device_t *dev, uhc_enum_status_t status)
{
    (void)dev;
    if (status == UHC_ENUM_FAIL) {
        if (dev->hub && dev->speed == UHD_SPEED_LOW) {
            WARNING("LOW speed via HUB detected!");
        } else {
            WARNING("USB device failed enumeration");
        }
    } else if (status == UHC_ENUM_SUCCESS) {
        INFO("USB VID/PID : %04x / %04x", dev->dev_desc.idVendor, dev->dev_desc.idProduct);
    }

}

void USB_Mouse_Change(uhc_device_t *dev, bool b_plug)
{
    (void)dev;
    INFO("HID Device MOUSE %s", b_plug ? "attached" : "detached");
}

void USB_Keyboard_Change(uhc_device_t *dev, bool b_plug)
{
    (void)dev;
    INFO("HID Device KEYBOARD %s", b_plug ? "attached" : "detached");
}

void USB_Hub_Change(uhc_device_t *dev, bool b_plug)
{
    (void)dev;
    INFO("USB Hub Device %s", b_plug ? "attached" : "detached");
}

void USB_Mouse_Button_Left(bool b_state)
{
    mouse.OnMouseButtonChanged(0, b_state);
}

void USB_Mouse_Button_Right(bool b_state)
{
    mouse.OnMouseButtonChanged(1, b_state);
}

void USB_Mouse_Button_Middle(bool b_state)
{
    mouse.OnMouseButtonChanged(2, b_state);
}

void USB_Mouse_Move(int8_t x, int8_t y, int8_t scroll)
{
    mouse.OnMouseMove(x, y, scroll);
}


void USB_Keyboard_Meta(uint8_t before, uint8_t after)
{
    keyboard.OnControlKeysChanged(before, after);
}
void USB_Keyboard_Key_Down(uint8_t meta, uint8_t key)
{
    keyboard.OnKeyDown(meta, key);
}
void USB_Keyboard_Key_Up(uint8_t meta, uint8_t key)
{
    keyboard.OnKeyUp(meta, key);
}

}


extern "C" void USBHID_Init()
{
    uhc_start();
}

extern "C" const char* USBHID_Update()
{
    mouse.SendMouseUpdate();
    disableIRQ();
    const char* key = keyboard.PopKey();
    enableIRQ();
    return key;
}

const uint8_t ReplayKeyboard::OEMtoPS2[0x74] = {
    0x00,			// 0x00 =>No Event
    0x00,			// 0x01 =>Overrun Error
    0x00,			// 0x02 =>POST Fail
    0x00,			// 0x03 =>ErrorUndefined
    0x1C,			// 0x04 => a A
    0x32,			// 0x05 => b B
    0x21,			// 0x06 => c C
    0x23,			// 0x07 => d D
    0x24,			// 0x08 => e E
    0x2B,			// 0x09 => f F
    0x34,			// 0x0A => g G
    0x33,			// 0x0B => h H
    0x43,			// 0x0C => i I
    0x3B,			// 0x0D => j J
    0x42,			// 0x0E => k K
    0x4B,			// 0x0F => l L
    0x3A,			// 0x10 => m M
    0x31,			// 0x11 => n N
    0x44,			// 0x12 => o O
    0x4D,			// 0x13 => p P
    0x15,			// 0x14 => q Q
    0x2D,			// 0x15 => r R
    0x1B,			// 0x16 => s S
    0x2C,			// 0x17 => t T
    0x3C,			// 0x18 => u U
    0x2A,			// 0x19 => v V
    0x1D,			// 0x1A => w W
    0x22,			// 0x1B => x X
    0x35,			// 0x1C => y Y
    0x1A,			// 0x1D => z Z
    0x16,			// 0x1E => 1 !
    0x1E,			// 0x1F => 2 @
    0x26,			// 0x20 => 3 #
    0x25,			// 0x21 => 4 $
    0x2E,			// 0x22 => 5 %
    0x36,			// 0x23 => 6 ^
    0x3D,			// 0x24 => 7 &
    0x3E,			// 0x25 => 8 *
    0x46,			// 0x26 => 9 (
    0x45,			// 0x27 => 0 )
    0x5A,			// 0x28 => Return
    0x76,			// 0x29 => Escape
    0x66,			// 0x2A => Backspace
    0x0D,			// 0x2B => Tab
    0x29,			// 0x2C => Space
    0x4E,			// 0x2D => - _
    0x55,			// 0x2E => = +
    0x54,			// 0x2F => [ {
    0x5B,			// 0x30 => ] }
    0x5D,			// 0x31 => \ |
    0x5D,			// 0x32 => Europe 1 (Note 2)
    0x4C,			// 0x33 => ; :
    0x52,			// 0x34 => ' "
    0x0E,			// 0x35 => ` ~
    0x41,			// 0x36 => , <
    0x49,			// 0x37 => . >
    0x4A,			// 0x38 => / ?
    0x58,			// 0x39 => Caps Lock
    0x05,			// 0x3A => F1
    0x06,			// 0x3B => F2
    0x04,			// 0x3C => F3
    0x0C,			// 0x3D => F4
    0x03,			// 0x3E => F5
    0x0B,			// 0x3F => F6
    0x83,			// 0x40 => F7
    0x0A,			// 0x41 => F8
    0x01,			// 0x42 => F9
    0x09,			// 0x43 => F10
    0x78,			// 0x44 => F11
    0x07,			// 0x45 => F12
    0x80 + 0x7C,	// 0x46 => Print Screen
    0x7E,			// 0x47 => Scroll Lock
    0x00,			// 0x48 => Pause/Break
    0x80 + 0x70,	// 0x49 => Insert (Note 1)
    0x80 + 0x6C,	// 0x4A => Home (Note 1)
    0x80 + 0x7D,	// 0x4B => Page Up (Note 1)
    0x80 + 0x71,	// 0x4C => Delete (Note 1)
    0x80 + 0x69,	// 0x4D => End (Note 1)
    0x80 + 0x7A,	// 0x4E => Page Down (Note 1)
    0x80 + 0x74,	// 0x4F => Right Arrow (Note 1)
    0x80 + 0x6B,	// 0x50 => Left Arrow (Note 1)
    0x80 + 0x72,	// 0x51 => Down Arrow (Note 1)
    0x80 + 0x75,	// 0x52 => Up Arrow (Note 1)
    0x77,			// 0x53 => Num Lock
    0x80 + 0x4A,	// 0x54 => Keypad / (Note 1)
    0x7C,			// 0x55 => Keypad *
    0x7B,			// 0x56 => Keypad -
    0x79,			// 0x57 => Keypad +
    0x80 + 0x5A,	// 0x58 => Keypad Enter
    0x69,			// 0x59 => Keypad 1 End
    0x72,			// 0x5A => Keypad 2 Down
    0x7A,			// 0x5B => Keypad 3 PageDn
    0x6B,			// 0x5C => Keypad 4 Left
    0x73,			// 0x5D => Keypad 5
    0x74,			// 0x5E => Keypad 6 Right
    0x6C,			// 0x5F => Keypad 7 Home
    0x75,			// 0x60 => Keypad 8 Up
    0x7D,			// 0x61 => Keypad 9 PageUp
    0x70,			// 0x62 => Keypad 0 Insert
    0x71,			// 0x63 => Keypad . Delete
    0x61,			// 0x64 => Europe 2 (Note 2)
    0x80 + 0x2F,	// 0x65 => App
    0x80 + 0x37,	// 0x66 => Keyboard Power
    0x0F,			// 0x67 => Keypad =
    0x08,			// 0x68 => F13
    0x10,			// 0x69 => F14
    0x18,			// 0x6A => F15
    0x20,			// 0x6B => F16
    0x28,			// 0x6C => F17
    0x30,			// 0x6D => F18
    0x38,			// 0x6E => F19
    0x40,			// 0x6F => F20
    0x48,			// 0x70 => F21
    0x50,			// 0x71 => F22
    0x57,			// 0x72 => F23
    0x5F,			// 0x73 => F24
};

const uint8_t ReplayKeyboard::OEMtoPS2meta[8] = {
    0x14,			// 0xE0 => Left Control
    0x12,			// 0xE1 => Left Shift
    0x11,			// 0xE2 => Left Alt
    0x80 + 0x1F,	// 0xE3 => Left GUI
    0x80 + 0x14,	// 0xE4 => Right Control
    0x59,			// 0xE5 => Right Shift
    0x80 + 0x11,	// 0xE6 => Right Alt
    0x80 + 0x27,	// 0xE7 => Right GUI
};
