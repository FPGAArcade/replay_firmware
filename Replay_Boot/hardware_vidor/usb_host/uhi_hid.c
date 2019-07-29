// ADD REPLAY LICENSE!

#include "conf_usb_host.h"
#include "usb_protocol_hid.h"
#include "uhi_hid.h"

uhi_hid_report_data_t uhi_hid_report_data;

uint8_t get_report_id(uint8_t* hid_report, uint16_t hid_report_size)
{
	// we do an half-assed attempt to parse the report here; we really only care if there is a report ID present or not.

	uint8_t usage = 0x00;
	uint8_t usage_page = 0x00;
	uint8_t collection_stack[16];
	uint8_t collection_index = 0;
	uint8_t report_id = 0xff;

	usb_printmem(hid_report, hid_report_size);

	char indent[16] = { 0 };

	uint8_t* report = hid_report;
	for (uint16_t remaining = hid_report_size; remaining > 0; ) {

		const usb_hid_report_item_t* item = (usb_hid_report_item_t*)report;
		report++; remaining--;

		uint32_t data = 0;
		const uint32_t data_size = (item->bSize ? (1 << (item->bSize-1)) : 0);

		Assert(data_size <= remaining);

		for (uint8_t i = 0; i < data_size; ++i) {
			data = data | (*report << 8*i);
			report++; remaining--;
		}

		int32_t data_signed = (int32_t)data;
		if (data_size == 1) {
			data_signed = (int8_t)data;
		} else if (data_size == 2) {
			data_signed = (int16_t)data;
		}
		(void)data_signed;

		const uint8_t* ptr = (uint8_t*)item;

		for (uint8_t i = 0; i < 5; ++i) {
			if (ptr < report)
				usb_printf("%02X ", *ptr++);
			else
				usb_printf("   ");
		}

		usb_printf(" | %s", indent);

		switch(item->bType) {
			case USB_HID_REPORT_ITEM_MAIN:
				// usb_printf("  USB_HID_REPORT_ITEM_MAIN");
				switch(item->bTag) {
					case USB_HID_REPORT_MAIN_INPUT:
						usb_printf("    Input ");
						usb_hid_main_item_bits* bits_ptr = (usb_hid_main_item_bits*)&data;
						usb_hid_main_item_bits bits = *bits_ptr;
						if (bits.constant) {
							usb_printf("(Constant)");
						} else {
							usb_printf("(Data");
							if (bits.variable)
								usb_printf(", Variable");
							else
								usb_printf(", Array");
							if (bits.relative)
								usb_printf(", Relative");
							else
								usb_printf(", Absolute");
							if (bits.wrap)
								usb_printf(", Wrap");
							if (bits.non_linear)
								usb_printf(", Non linear");
							if (bits.no_preferred)
								usb_printf(", No preferred state");
							if (bits.null_state)
								usb_printf(", Null state");
							if (bits.volatile_)
								usb_printf(", Volatile");
							if (bits.buffered_bytes)
								usb_printf(", Buffered bytes");
							else
								usb_printf(", Bitfield");
							usb_printf(")");
						}
						break;
					case USB_HID_REPORT_MAIN_OUTPUT:
						usb_printf("    USB_HID_REPORT_MAIN_OUTPUT");
						break;
					case USB_HID_REPORT_MAIN_FEATURE:
						usb_printf("    USB_HID_REPORT_MAIN_FEATURE");
						break;
					case USB_HID_REPORT_MAIN_COLLECTION:
						usb_printf("    Collection ");
						strcat(indent, "\t");
						switch(data) {
							case USB_HID_REPORT_COLLETION_PHYSICAL:
								usb_printf("(Physical)");
								break;
							case USB_HID_REPORT_COLLETION_APPLICATION:
								usb_printf("(Application)");
								break;
							case USB_HID_REPORT_COLLETION_LOGICAL:
								usb_printf("(Logical)");
								break;
							case USB_HID_REPORT_COLLETION_REPORT:
								usb_printf("(Report)");
								break;
							case USB_HID_REPORT_COLLETION_NAMED_ARRAY:
								usb_printf("(Named Array)");
								break;
							case USB_HID_REPORT_COLLETION_USAGE_SWITCH:
								usb_printf("(Usage Switch)");
								break;
							case USB_HID_REPORT_COLLETION_USAGE_MODIFIER:
								usb_printf("(Usage Modifier)");
								break;
							default:
								usb_printf("(Reserved)");
								break;
						}
						collection_stack[collection_index++] = data;
						break;
					case USB_HID_REPORT_MAIN_COLLECTION_END:
						usb_printf("    End Collection");
						Assert(indent[0] != 0);
						indent[strlen(indent)-1] = 0;
						Assert(collection_index > 0);
						collection_index--;
						break;
					default:
						usb_printf("    USB_HID_REPORT_MAIN_RESERVED");
						break;
				}
				break;
			case USB_HID_REPORT_ITEM_GLOBAL:
				// usb_printf("  USB_HID_REPORT_ITEM_GLOBAL");
				switch(item->bTag) {
					case USB_HID_REPORT_GLOBAL_USAGE_PAGE:
						usb_printf("    Usage Page ");
						switch(data) {
							case USB_HID_REPORT_USAGE_PAGE_GENERIC_DESKTOP:
								usb_printf("(Generic Desktop)");
								break;
							case USB_HID_REPORT_USAGE_PAGE_SIMULATION:
								usb_printf("(Simulation)");
								break;
							case USB_HID_REPORT_USAGE_PAGE_VR:
								usb_printf("(VR)");
								break;
							case USB_HID_REPORT_USAGE_PAGE_SPORT:
								usb_printf("(Sport)");
								break;
							case USB_HID_REPORT_USAGE_PAGE_GAMING:
								usb_printf("(Gaming)");
								break;
							case USB_HID_REPORT_USAGE_PAGE_GENERIC_DEVICE:
								usb_printf("(Generic Device)");
								break;
							case USB_HID_REPORT_USAGE_PAGE_KEYBOARD:
								usb_printf("(Keyboard)");
								break;
							case USB_HID_REPORT_USAGE_PAGE_LEDS:
								usb_printf("(LEDs)");
								break;
							case USB_HID_REPORT_USAGE_PAGE_BUTTONS:
								usb_printf("(Buttons)");
								break;
							case USB_HID_REPORT_USAGE_PAGE_ORDINAL:
								usb_printf("(Ordinal)");
								break;
							case USB_HID_REPORT_USAGE_PAGE_TELEPHONY:
								usb_printf("(Telephony)");
								break;
							case USB_HID_REPORT_USAGE_PAGE_CONSUMER:
								usb_printf("(Consumer)");
								break;
							default:
								usb_printf("(Unknown)");
								break;
						}
						usage_page = data;
						break;
					case USB_HID_REPORT_GLOBAL_LOGICAL_MINIMUM:
						usb_printf("    Logical Minimum (%i)", data_signed);
						break;
					case USB_HID_REPORT_GLOBAL_LOGICAL_MAXIMUM:
						usb_printf("    Logical Maximum (%i)", data_signed);
						break;
					case USB_HID_REPORT_GLOBAL_PHYSICAL_MINIMUM:
						usb_printf("    USB_HID_REPORT_GLOBAL_PHYSICAL_MINIMUM");
						break;
					case USB_HID_REPORT_GLOBAL_PHYSICAL_MAXIMUM:
						usb_printf("    USB_HID_REPORT_GLOBAL_PHYSICAL_MAXIMUM");
						break;
					case USB_HID_REPORT_GLOBAL_UNIT_EXPONENT:
						usb_printf("    USB_HID_REPORT_GLOBAL_UNIT_EXPONENT");
						break;
					case USB_HID_REPORT_GLOBAL_UNIT:
						usb_printf("    USB_HID_REPORT_GLOBAL_UNIT");
						break;
					case USB_HID_REPORT_GLOBAL_REPORT_SIZE:
						usb_printf("    Report Size (%i)", data);
						break;
					case USB_HID_REPORT_GLOBAL_REPORT_ID:
						usb_printf("    Report ID (%i)", data);
						if (usage == USB_HID_REPORT_USAGE_MOUSE &&
							usage_page == USB_HID_REPORT_USAGE_PAGE_GENERIC_DESKTOP &&
							collection_index > 0 && 
							collection_stack[0] == USB_HID_REPORT_COLLETION_APPLICATION) {
							report_id = data;
						}
						break;
					case USB_HID_REPORT_GLOBAL_REPORT_COUNT:
						usb_printf("    Report Count (%i)", data);
						break;
					case USB_HID_REPORT_GLOBAL_PUSH:
						usb_printf("    USB_HID_REPORT_GLOBAL_PUSH");
						break;
					case USB_HID_REPORT_GLOBAL_POP:
						usb_printf("    USB_HID_REPORT_GLOBAL_POP");
						break;
					default:
						usb_printf("    USB_HID_REPORT_GLOBAL_RESERVED");
						break;
				}
				break;
			case USB_HID_REPORT_ITEM_LOCAL:
				// usb_printf("  USB_HID_REPORT_ITEM_LOCAL");
				switch(item->bTag) {
					case USB_HID_REPORT_LOCAL_USAGE:
						usb_printf("    Usage ");
						switch(data) {
							case USB_HID_REPORT_USAGE_POINTER:
								usb_printf("(Pointer)");
								break;
							case USB_HID_REPORT_USAGE_MOUSE:
								usb_printf("(Mouse)");
								break;
							case USB_HID_REPORT_USAGE_JOYSTICK:
								usb_printf("(Joystick)");
								break;
							case USB_HID_REPORT_USAGE_GAMEPAD:
								usb_printf("(Gamepad)");
								break;
							case USB_HID_REPORT_USAGE_KEYBOARD:
								usb_printf("(Keyboard)");
								break;
							case USB_HID_REPORT_USAGE_KEYPAD:
								usb_printf("(Keypad)");
								break;
							case USB_HID_REPORT_USAGE_MULTIAXIS:
								usb_printf("(Multiaxis)");
								break;
							case USB_HID_REPORT_USAGE_X:
								usb_printf("(X)");
								break;
							case USB_HID_REPORT_USAGE_Y:
								usb_printf("(Y)");
								break;
							case USB_HID_REPORT_USAGE_Z:
								usb_printf("(Z)");
								break;
							case USB_HID_REPORT_USAGE_WHEEL:
								usb_printf("(Wheel)");
								break;
							case USB_HID_REPORT_USAGE_HAT:
								usb_printf("(Hat)");
								break;
							default:
								usb_printf("(Unknown, %i)", data);
								break;
						}
						if (collection_index == 0) {
							usage = data;
						}
						break;
					case USB_HID_REPORT_LOCAL_USAGE_MINIMUM:
						usb_printf("    Usage Minimum (%i)", data);
						break;
					case USB_HID_REPORT_LOCAL_USAGE_MAXIMUM:
						usb_printf("    Usage Maximum (%i)", data);
						break;
					case USB_HID_REPORT_LOCAL_DESIGNATOR_INDEX:
						usb_printf("    USB_HID_REPORT_LOCAL_DESIGNATOR_INDEX");
						break;
					case USB_HID_REPORT_LOCAL_DESIGNATOR_MINIMUM:
						usb_printf("    USB_HID_REPORT_LOCAL_DESIGNATOR_MINIMUM");
						break;
					case USB_HID_REPORT_LOCAL_DESIGNATOR_MAXIMUM:
						usb_printf("    USB_HID_REPORT_LOCAL_DESIGNATOR_MAXIMUM");
						break;
					case USB_HID_REPORT_LOCAL_STRING_INDEX:
						usb_printf("    USB_HID_REPORT_LOCAL_STRING_INDEX");
						break;
					case USB_HID_REPORT_LOCAL_STRING_MINIMUM:
						usb_printf("    USB_HID_REPORT_LOCAL_STRING_MINIMUM");
						break;
					case USB_HID_REPORT_LOCAL_STRING_MAXIMUM:
						usb_printf("    USB_HID_REPORT_LOCAL_STRING_MAXIMUM");
						break;
					case USB_HID_REPORT_LOCAL_DELIMITER:
						usb_printf("    USB_HID_REPORT_LOCAL_DELIMITER");
						break;
					default:
						usb_printf("    USB_HID_REPORT_LOCAL_RESERVED");
						break;
				}
				break;
			case USB_HID_REPORT_ITEM_RESERVED:
				usb_printf("  USB_HID_REPORT_ITEM_RESERVED");
				break;
		}

		usb_printf("  DATA = %08x\n\r", data);
	}
	return report_id;
}