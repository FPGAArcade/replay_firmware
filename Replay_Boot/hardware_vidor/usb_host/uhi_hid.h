// ADD REPLAY LICENSE!

#pragma once

typedef struct {
	uint8_t hid_report[512];
} uhi_hid_report_data_t;

extern uhi_hid_report_data_t uhi_hid_report_data;

uint8_t get_report_id(uint8_t* hid_report, uint16_t hid_report_size);

