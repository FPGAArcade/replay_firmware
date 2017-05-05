#pragma once
#include <stdint.h>

static const uint8_t DeviceDescriptor[] = {
    0x12,                       // Descriptor length (18 bytes)
    0x01,                       // Descriptor type (Device)
    0x10, 0x01,                 // Complies with USB Spec. Release 2.0
    0x00,                       // Class code (0)
    0x00,                       // Subclass code (0)
    0x00,                       // Protocol (No specific protocol)
    0x08,                       // Maximum packet size for Endpoint 0 (8 bytes)
    0xc5, 0x9a,                 // Vendor ID (random numbers)
    0x8f, 0x4b,                 // Product ID (random numbers)
    0x00, 0x01,                 // Device release number (0001)
    0x01,                       // Manufacturer string descriptor index
    0x02,                       // Product string descriptor index
    0x03,                       // Serial Number string descriptor index (None)
    0x01,                       // Number of possible configurations (1)
};

static const uint8_t ConfigurationDescriptor[] = {
    0x09,                       // Descriptor length (9 bytes)
    0x02,                       // Descriptor type (Configuration)
    0x20, 0x00,                 // Total data length (41 bytes)
    0x01,                       // Interface supported (1)
    0x01,                       // Configuration value (1)
    0x00,                       // Index of string descriptor (None)
    0xc0,                       // Configuration (Self powered)
    250,                        // Maximum power consumption (100mA)

    // Interface
    0x09,                       // Descriptor length (9 bytes)
    0x04,                       // Descriptor type (Interface)
    0x00,                       // Number of interface (0)
    0x00,                       // Alternate setting (0)
    0x02,                       // Number of interface endpoint (2)
    0x08,                       // Class code (MASS STORAGE Class)
    0x06,                       // Subclass code (SCSI transparent command set)
    0x50,                       // Protocol code (USB Mass Storage Class Bulk-Only (BBB) Transport)
    0x00,                       // Index of string()

    // endpoint 1
    0x07,                       // Descriptor length (7 bytes)
    0x05,                       // Descriptor type (Endpoint)
    0x81,                       // Encoded address (Respond to IN)
    0x02,                       // Endpoint attribute (Bulk transfer)
    0x40, 0x00,                 // Maximum packet size (64 bytes)
    0x00,                       // Polling interval (not used)

    // endpoint 2
    0x07,                       // Descriptor length (7 bytes)
    0x05,                       // Descriptor type (Endpoint)
    0x02,                       // Encoded address (Respond to IN)
    0x02,                       // Endpoint attribute (Bulk transfer)
    0x40, 0x00,                 // Maximum packet size (64 bytes)
    0x00,                       // Polling interval (not used)
};

static const uint8_t DeviceQualifierDescriptor[] = {
    10,
    0x06,
    0x00, 0x02,
    0xff,
    0xff,
    0x00,
    0x08,
    0,
    0
};

static const uint8_t StringDescriptor0[] = {
    0x04,                       // Length
    0x03,                       // Type is string
    0x09,                       // English
    0x04,                       //  US
};

// These string descriptors can be changed for the particular application.
// The string itself is Unicode, and the length is the total length of
// the descriptor, so it is the number of characters times two, plus two.

static const uint8_t StringDescriptor1[] = {
    22,             // Length
    0x03,           // Type is string
    'F', 0x00,
    'P', 0x00,
    'G', 0x00,
    'A', 0x00,
    'A', 0x00,
    'r', 0x00,
    'c', 0x00,
    'a', 0x00,
    'd', 0x00,
    'e', 0x00,
};

static const uint8_t StringDescriptor2[] = {
    14,             // Length
    0x03,           // Type is string
    'R', 0x00,
    'e', 0x00,
    'p', 0x00,
    'l', 0x00,
    'a', 0x00,
    'y', 0x00,
};

static const uint8_t StringDescriptor3[] = {
    34,             // Length
    0x03,           // Type is string
    '0', 0x00,
    '1', 0x00,
    '2', 0x00,
    '3', 0x00,
    '4', 0x00,
    '5', 0x00,
    '6', 0x00,
    '7', 0x00,
    '8', 0x00,
    '9', 0x00,
    'a', 0x00,
    'b', 0x00,
    'c', 0x00,
    'd', 0x00,
    'e', 0x00,
    'f', 0x00,
};

static const uint8_t StringDescriptor4[] = {
    8,             // Length
    0x03,           // Type is string
    'M', 0x00,
    'T', 0x00,
    'P', 0x00,
};

static const uint8_t StringDescriptorEE[] = {
    18,             // Length
    0x03,           // Type is string
    'M', 0x00,
    'S', 0x00,
    'F', 0x00,
    'T', 0x00,
    '1', 0x00,
    '0', 0x00,
    '0', 0x00,
    0x1, 0x00,
};

static const uint8_t * const StringDescriptors[] = {
    StringDescriptor0,
    StringDescriptor1,
    StringDescriptor2,
    StringDescriptor3,
    StringDescriptor4,
};

static const uint8_t RecipientDevice[] = {
    0x28, 0x00, 0x00, 0x00,     // Descriptor length (40 bytes)
    0x00, 0x01,                 // bcdVersion
    0x04, 0x00,                 // Extended Compat ID OS Feature Descriptor (wIndex = 0x0004)
    0x01,                       // bCount
    0x00, 0x00, 0x00, 0x00,     // (reserved)
    0x00, 0x00, 0x00,           // (reserved)
    0x00,                       // bFirstInterfaceNumber
    0x01,                       // bInterfaceCount
    'M', 'T', 'P', 0x00,        // compatibleID[8]
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     // subCompatibleID[8]
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,     // (reserved)
    0x00, 0x00,                 // (reserved)
};
