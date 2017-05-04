#pragma once
#include <stdint.h>

void ptp_send(uint8_t* packet, uint32_t length);

void PTP_USB_Start(void);
void PTP_USB_Stop(void);
uint8_t PTP_USB_Poll(void);
