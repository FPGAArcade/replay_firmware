#pragma once
#ifndef USB_H_INCLUDED
#define USB_H_INCLUDED

#include "config.h"

void USB_MountMassStorage(status_t* current_status);
void USB_UnmountMassStorage(status_t* current_status);
void USB_Update(status_t* current_status);

#endif // USB_H_INCLUDED