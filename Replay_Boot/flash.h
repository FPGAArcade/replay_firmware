#pragma once
#ifndef FLASH_H_INCLUDED
#define FLASH_H_INCLUDED

#include "config.h"

uint8_t FLASH_VerifySRecords(const char* filename, uint32_t* crc_file, uint32_t* crc_flash);
uint8_t FLASH_RebootAndFlash(const char* filename);

#endif // FLASH_H_INCLUDED
