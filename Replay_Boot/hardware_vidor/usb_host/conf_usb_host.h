// ADD REPLAY LICENSE!

#ifndef _CONF_USB_HOST_H_
#define _CONF_USB_HOST_H_

#ifdef	__cplusplus
extern "C" {
#endif

#define USB_HOST_UHI                                UHI_HUB,UHI_HID_MOUSE,UHI_HID_KEYBOARD
#define USB_HOST_POWER_MAX                          500
#define USB_HOST_HUB_SUPPORT

#define UHC_CONNECTION_EVENT(dev,b_present)         USB_Connection_Event(dev,b_present)
#define UHC_ENUM_EVENT(dev,b_status)                USB_Enum_Event(dev,b_status)
#define UHI_HID_MOUSE_CHANGE(dev,b_plug)            USB_Mouse_Change(dev,b_plug)
#define UHI_HID_MOUSE_EVENT_BTN_LEFT(b_state)       USB_Mouse_Button_Left(b_state)
#define UHI_HID_MOUSE_EVENT_BTN_RIGHT(b_state)      USB_Mouse_Button_Right(b_state)
#define UHI_HID_MOUSE_EVENT_BTN_MIDDLE(b_state)     USB_Mouse_Button_Middle(b_state)
#define UHI_HID_MOUSE_EVENT_MOVE(x,y,scroll)        USB_Mouse_Move(x,y,scroll)
#define UHI_HID_KEYBOARD_CHANGE(dev,b_plug)         USB_Keyboard_Change(dev,b_plug)
#define UHI_HID_KEYBOARD_EVENT_META(pre,post)       USB_Keyboard_Meta(pre,post)
#define UHI_HID_KEYBOARD_EVENT_KEY_DOWN(meta,key)   USB_Keyboard_Key_Down(meta,key)
#define UHI_HID_KEYBOARD_EVENT_KEY_UP(meta,key)     USB_Keyboard_Key_Up(meta,key)
#define UHI_HUB_CHANGE(dev,b_plug)                  USB_Hub_Change(dev,b_plug)

#include "usb_debug.h"
#include "uhi_hub.h"
#include "uhi_hid_mouse.h"
#include "uhi_hid_keyboard.h"
#include "uhc.h"

void USB_Connection_Event(uhc_device_t *dev, bool b_present);
void USB_Enum_Event(uhc_device_t *dev, uhc_enum_status_t status);
void USB_Mouse_Change(uhc_device_t *dev, bool b_plug);
void USB_Mouse_Button_Left(bool b_state);
void USB_Mouse_Button_Right(bool b_state);
void USB_Mouse_Button_Middle(bool b_state);
void USB_Mouse_Move(int8_t x, int8_t y, int8_t scroll);
void USB_Keyboard_Change(uhc_device_t *dev, bool b_plug);
void USB_Keyboard_Meta(uint8_t before, uint8_t after);
void USB_Keyboard_Key_Down(uint8_t meta, uint8_t key);
void USB_Keyboard_Key_Up(uint8_t meta, uint8_t key);
void USB_Hub_Change(uhc_device_t *dev, bool b_plug);

#ifdef	__cplusplus
}
#endif

#endif // _CONF_USB_HOST_H_
