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

#include "stringlight.h"
#include <stdlib.h>

static char null_string[] = "";

void _strncpySpace(char* pStr1, const char* pStr2, unsigned long nCount)
{
// customized strncpy() function to fill remaing destination string part with spaces

    while (*pStr2 && nCount)
    {
        *pStr1++ = *pStr2++; // copy strings
        nCount--;
    }

    while (nCount--)
        *pStr1++ = ' '; // fill remaining space with spaces

}

// safe, copies bufsize-1 max and adds terminator
void _strlcpy(char* dst, const char* src, unsigned long bufsize)
{
  unsigned long srclen =strlen(src);
  unsigned long result =srclen; /* Result is always the length of the src string */
  if(bufsize>0) {
    if(srclen>=bufsize)
       srclen=bufsize-1;
    if(srclen>0)
       memcpy(dst,src,srclen);
    dst[srclen]='\0';
  }
  //return result;
}

int _stricmp_logical(const char *pS1, const char *pS2)
{
    char c1, c2;
    int v;

    do
    {
        c1 = *pS1;
        c2 = *pS2;

        if (!c1 || !c2)
        {
            v = c1 - c2;
            break;
        }

        int d1 = isdigit((uint8_t)c1);
        int d2 = isdigit((uint8_t)c2);

        if (d1 || d2)
        {
            if (d1 && !d2)
              return -1;
            else if (!d1 && d2)
              return 1;

            char *s1, *s2;
            d1 = strtoul(pS1, &s1, 10) & 0x7fffffff;
            d2 = strtoul(pS2, &s2, 10) & 0x7fffffff;
            v = d1 - d2;
            pS1 = s1;
            pS2 = s2;
        }
        else
        {
            v = (unsigned int)tolower((uint8_t)c1) - (unsigned int)tolower((uint8_t)c2);
            pS1++;
            pS2++;
        }
    }
    while ((v == 0) && (c1 != '\0') && (c2 != '\0'));

    return v;
}

int _strnicmp(const char *pS1, const char *pS2, unsigned long n)
{
    char c1, c2;
    int v;

    do
    {
        c1 = *pS1++;
        c2 = *pS2++;
        v = (unsigned int)tolower(c1) - (unsigned int)tolower(c2);
    }
    while ((v == 0) && (c1 != '\0') && (c2 != '\0') && (--n > 0));

    return v;
}

int _strncmp(const char *pS1, const char *pS2, unsigned long n)
{
    char c1, c2;
    int v;

    do
    {
        c1 = *pS1++;
        c2 = *pS2++;
        v = (unsigned int)(c1) - (unsigned int)(c2);
    }
    while ((v == 0) && (c1 != '\0') && (c2 != '\0') && (--n > 0));

    return v;
}

unsigned int _htoi (const char *pStr)
{
  unsigned int value = 0;
  char ch = *pStr;

  while (ch == ' ' || ch == '\t')
    ch = *(++pStr);

  // remove 0x
  if (ch == '0') ch = *(++pStr);
  if (ch == 'x' || ch == 'X') ch = *(++pStr);

  for (;;) {
    if (ch >= '0' && ch <= '9')
       value = (value << 4) + (ch - '0');
    else if (ch >= 'A' && ch <= 'F')
       value = (value << 4) + (ch - 'A' + 10);
    else if (ch >= 'a' && ch <= 'f')
       value = (value << 4) + (ch - 'a' + 10);
    else
       return value;
    ch = *(++pStr);
  }
}

void FileDisplayName(char *name, uint16_t name_len, char *path) // name_len includes /0
{
  uint16_t i = (uint16_t) strlen(path);

  while(i != 0) {
    if(path[i] == '\\' || path[i] == '/') {
      break;
    }
    i--;
  }
  _strncpySpace(name, (path + i + 1), name_len);
  name[name_len-1] = '\0';
}

char* GetExtension(char* filename)
{
  uint32_t len = strlen(filename);
  uint32_t i = 0;
  char* pResult = null_string;
  for (i = len-1; i > 0; --i) {
    if (filename[i] == '.')
      return ((char*) filename+i+1);
    if ((filename[i] == '/') || (filename[i] == '\\'))
      break;
  }
  return pResult;
}


// maybe not the best place for this - from FF demo
void FF_ExpandPath(char *acPath) {

    char        *pRel           = 0;
    char        *pRelStart      = 0;
    char        *pRelEnd        = 0;

    int         charRef         = 0;
    int         lenPath         = 0;
    int         lenRel          = 0;
    int         i                       = 0;
    int         remain          = 0;


    lenPath = strlen(acPath);
    pRel = strstr(acPath, "..");

    while(pRel) {       // Loop removal of Relativity
        charRef = pRel - acPath;

        /*
            We have found some relativity in the Path,
        */

        // Where it ends:

        if(pRel[2] == '\\' || pRel[2] == '/') {
            pRelEnd = pRel + 3;
        } else {
            pRelEnd = pRel + 2;
        }

        // Where it Starts:

        if(charRef == 1) {      // Relative Path comes after the root /
            return;     // Fixed, returns false appropriately, as in the TODO: above!
        } else {
            for(i = (charRef - 2); i >= 0; i--) {
                if(acPath[i] == '\\' || acPath[i] == '/') {
                    pRelStart = (acPath + (i + 1));
                    break;
                }
            }
        }

        // The length of the relativity
        lenRel = pRelEnd - pRelStart;

        remain = lenPath - (pRelEnd - acPath);  // Remaining Chars on the end of the path

        if(lenRel) {

            strncpy(pRelStart, pRelEnd, remain);
            pRelStart[remain] = '\0';
        }

        lenPath -= lenRel;
        pRel = strstr(acPath, "..");
    }
}



