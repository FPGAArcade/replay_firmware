/*
 WWW.FPGAArcade.COM

 REPLAY Retro Gaming Platform
 No Emulation No Compromise

 All rights reserved
 Mike Johnson Wolfgang Scherr

 SVN: $Id:

--------------------------------------------------------------------

 Redistribution and use in source and synthezised forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.

 Redistributions in synthesized form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.

 Neither the name of the author nor the names of other contributors may
 be used to endorse or promote products derived from this software without
 specific prior written permission.

 THIS CODE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.

 You are responsible for any legal issues arising from your use of this code.

 The latest version of this file can be found at: www.FPGAArcade.com

 Email support@fpgaarcade.com
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

void FF_ExpandPath(char* acPath);

#if __GNUC__ >= 5
char* strcasestr(const char*, const char*);			// __BSD_VISIBLE
#define stricmp(a,b) strcasecmp(a,b) 				// int stricmp(const char *, const char *);
#define strnicmp(a,b,c) strncasecmp(a,b,c)			// int strnicmp(const char *, const char *, size_t);
#endif

#endif

