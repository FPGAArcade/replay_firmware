#ifndef FILEIO_DRV_H_INCLUDED
#define FILEIO_DRV_H_INCLUDED

#include "fileio.h"
#include "board.h"
#include "fullfat.h"

void FileIO_Drv01_Process(uint8_t ch, fch_arr_t *pHandle, uint8_t status); // floppy
void FileIO_Drv00_Process(uint8_t ch, fch_arr_t *pHandle, uint8_t status); // generic

uint8_t FileIO_Drv01_InsertInit(uint8_t ch, uint8_t drive_number, fch_t *drive, char *ext);
uint8_t FileIO_Drv00_InsertInit(uint8_t ch, uint8_t drive_number, fch_t *drive, char *ext);


#endif
