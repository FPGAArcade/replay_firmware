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
int  _strnicmp(const char *pS1, const char *pS2, unsigned long n);
unsigned int _htoi (const char *ptr);

//
// additional string stuff
//
void FileDisplayName(char *name, uint16_t name_len, char *path);
char* GetExtension(char* filename);

void FF_ExpandPath(char *acPath);

#endif

