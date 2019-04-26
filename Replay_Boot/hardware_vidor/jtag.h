#pragma once

#define PMUX(pin, v) (digitalPinToPort(pin)->PINCFG[g_APinDescription[pin].ulPin].bit.PMUXEN = v)

void JTAG_Init();
void JTAG_Reset();
int JTAG_StartBitstream();
void FPGA_WriteBitstream(uint8_t* buffer, uint32_t length, uint8_t done);
int JTAG_EndBitstream();
