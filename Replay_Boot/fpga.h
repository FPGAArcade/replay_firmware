#ifndef FPGA_H_INCLUDED
#define FPGA_H_INCLUDED

#include "board.h"
#include "fullfat.h"


// Wolfgang: reduced phase by 10 as discussed with Mike (on 16feb2014)
#define kDRAM_PHASE (0x68-10)
#define kDRAM_SEL   0x02

uint8_t FPGA_Default(void);
uint8_t FPGA_Config(FF_FILE *pFile);

uint8_t FPGA_DramTrain(void);
uint8_t FPGA_DramEye(uint8_t mode);
uint8_t FPGA_ProdTest(void);

void    FPGA_ExecMem(uint32_t base, uint16_t len, uint32_t checksum);

#endif
