/*--------------------------------------------------------------------
 *                       Replay Firmware
 *                      www.fpgaarcade.com
 *                     All rights reserved.
 *
 *                     admin@fpgaarcade.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *--------------------------------------------------------------------
 *
 * Copyright (c) 2020, The FPGAArcade community (see AUTHORS.txt)
 *
 */

#ifndef STRINGLIGHT_H_INCLUDED
#define STRINGLIGHT_H_INCLUDED

#include "board.h"
// the aim is to get rid of these
#include <ctype.h>
#include <string.h>
//

// FIX FOR COMMON NAMEING SCHEME
#define BYTETOBINARYPATTERN4 "%d%d%d%d"
#define BYTETOBINARY4(byte)  \
    (byte & 0x08 ? 1 : 0), \
    (byte & 0x04 ? 1 : 0), \
    (byte & 0x02 ? 1 : 0), \
    (byte & 0x01 ? 1 : 0)

void _strncpySpace(char* pStr1, const char* pStr2, unsigned long nCount);
void _strlcpy(char* dst, const char* src, unsigned long bufsize);
int _stricmp_logical(const char* pS1, const char* pS2);
#ifndef WIN32
int  _strnicmp(const char* pS1, const char* pS2, unsigned long n);
#endif
int  _strncmp(const char* pS1, const char* pS2, unsigned long n);
unsigned int _htoi (const char* ptr);

//
// additional string stuff
//
void FileDisplayName(char* name, uint16_t name_len, char* path);
char* GetExtension(char* filename);

#if __GNUC__ >= 5 || __clang__ || __GNUC__ == 4 && __GNUC_MINOR__ >= 8
char* strcasestr(const char*, const char*);			// __BSD_VISIBLE
#define stricmp(a,b) strcasecmp(a,b) 				// int stricmp(const char *, const char *);
#define strnicmp(a,b,c) strncasecmp(a,b,c)			// int strnicmp(const char *, const char *, size_t);
#endif

void pathcat(char* dest, const char* path1, const char* path2);

#endif

