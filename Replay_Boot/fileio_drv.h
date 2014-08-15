#ifndef FILEIO_DRV_H_INCLUDED
#define FILEIO_DRV_H_INCLUDED

#include "fileio.h"
#include "board.h"
#include "fullfat.h"

//void FileIO_Drv01_Handle(uint8_t ch, fch_t *handle, uint8_t status);
//void FileIO_Drv01_Handle(uint8_t ch, fch_t (*handle)[2][FCH_MAX_NUM], uint8_t status);
void FileIO_Drv01_Process(uint8_t ch, fch_arr_t *handle, uint8_t status); // amiga

uint8_t FileIO_Drv01_InsertInit(uint8_t ch, fch_t *drive, char *ext);


#endif
