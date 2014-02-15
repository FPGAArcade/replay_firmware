/** @file iniparser.c */

// Derived from inih http://code.google.com/p/inih/
//

/*
The "inih" library is distributed under the New BSD license:

Copyright (c) 2009, Brush Technology
All rights reserved.

Significantly modified by W. Scherr    (ws_arcade <at> pin4.at)
item list parser (c) 2013 by W. Scherr (http://www.pin4.at/)

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of Brush Technology nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY BRUSH TECHNOLOGY AND CO-AUTHORS ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL BRUSH TECHNOLOGY AND CO-AUTHORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "iniparser.h"
#include "messaging.h"

#include <ctype.h>
#include <string.h>

// ==========================================================================

// would be nicer in the header, but as it
// produces actually code it must stay here

/** Definitions of the symbols we can detect */
const ini_symtab_t symtab[] = {
// in text  ,token to use ,is section
  { "SETUP"   ,INI_SETUP    ,TRUE  },
  { "INFO"    ,INI_INFO     ,FALSE },
  { "BIN"     ,INI_BIN      ,FALSE },
  { "CLOCK"   ,INI_CLOCK    ,FALSE },
  { "PHASE"   ,INI_PHASE    ,FALSE },
  { "CODER"   ,INI_CODER    ,FALSE },
  { "VFILTER" ,INI_VFILTER  ,FALSE },
  { "EN_TWI"  ,INI_EN_TWI   ,FALSE },
  { "EN_SPI"  ,INI_EN_SPI   ,FALSE },
  { "SPI_CLK" ,INI_SPI_CLK  ,FALSE },
  { "BUTTON"  ,INI_BUTTON   ,FALSE },
  { "VIDEO"   ,INI_VIDEO    ,FALSE },
  { "CONFIG"  ,INI_CONFIG   ,FALSE },
  { "CSTORE"  ,INI_CSTORE   ,FALSE },
  { "UPLOAD"  ,INI_UPLOAD   ,TRUE  },
  { "VERIFY"  ,INI_VERIFY   ,FALSE },
  { "ROM"     ,INI_ROM      ,FALSE },
  { "DATA"    ,INI_DATA     ,FALSE },
  { "LAUNCH"  ,INI_LAUNCH   ,FALSE },
  { "MENU"    ,INI_MENU     ,TRUE  },
  { "TITLE"   ,INI_TITLE    ,FALSE },
  { "ITEM"    ,INI_ITEM     ,FALSE },
  { "OPTION"  ,INI_OPTION   ,FALSE },
  { ""        ,INI_UNKNOWN  ,FALSE,}, // no match found
  { ""        ,INI_START    ,TRUE, }, // start value, can never match
};

// ==========================================================================

/** removes trailing spaces */
static char* StripTrailingSpaces(char* s) {
    char* p = s + strlen(s);                 // TODO:
    while ((p>s) && isspace((int)(*--p)))   // we do int casting to avoid warning
        *p = '\0';
    return s;
}

/** returns pointer to first non-white char */
static char* FindFirstChar(const char* s) {
    if (!s || (strlen(s) == 0) ) return NULL;

                                        // TODO:
    while ((*s) && isspace((int)(*s)))  // we do int casting to avoid warning
        s++;
    return (char*) s;
}

/** returns pointer to last non-white char */
static char* FindLastChar(const char* s) {
    const char* p = s + strlen(s)-1;
    if (!s) return NULL;
                                        // TODO:
    while ((p>s) && isspace((int)(*p)))  // we do int casting to avoid warning
        --p;
    return (char*) p;
}

/** returns pointer to first specified char (or comment) */
static char* FindChar(const char* s, char c) {
    while (*s && (*s != c) && (*s != ';') && (*s != '#') )
        s++;
    return (char*) s;
}

// ==========================================================================

uint8_t ParseIni(FF_FILE *pFile, 
                uint8_t(*parseHandle)(void*, const ini_symbols_t, const ini_symbols_t, const char*), 
                void* config) {

  char lineBuffer[MAX_LINE_LEN];
  ini_symbols_t section = INI_UNKNOWN;

  uint32_t i;
  uint32_t lineNumber = 1;
  uint32_t lineError = 0;
  int32_t ch = 0; // note signed

  char* start;
  char* end;
  char* name;
  char* value;

  do {
    // read line from file
    for (i=0; (i< (sizeof(lineBuffer)-1) && ((ch = FF_GetC(pFile)) >= 0) && (ch !='\n')); i++) {
      lineBuffer[i] = (char) ch;
    }
    lineBuffer[i] = '\0';
    // read rest of line if it was too big for buffer
    while (( ch != -1) && (ch !='\n')) {
      ch = FF_GetC(pFile);
    }

    start = FindFirstChar(StripTrailingSpaces(lineBuffer));

    if (*start == '[') {
      end = FindChar(start+1, ']'); // find end or comment
      if (*end == ']') {
        int32_t idx=-1;
        *end = '\0';
        // get the token for further handling
        do {
          if (stricmp(symtab[++idx].keyword, start+1) == 0) break;
        } while (symtab[idx].token!=INI_UNKNOWN);
        section=symtab[idx].token;
        // check correctness, then call parser to indicate new section
        if (symtab[idx].token==INI_UNKNOWN) {
          ERROR("Unknown keyword.");
          lineError = lineNumber;
        }
        else if (symtab[idx].section==FALSE) {
          ERROR("Keyword not valid here.");
          lineError = lineNumber;
        }
        else if (parseHandle(config, section, INI_UNKNOWN, NULL)) {
          lineError = lineNumber;
        }
      }
      else {
        lineError = lineNumber;
      }
    }
    else if (*start && (*start != ';') && (*start != '#')) {
      end = FindChar(start+1, '=');
      if (*end == '=') {
        int32_t idx=-1;
        *end = '\0';
        name = StripTrailingSpaces(start);
        value = FindFirstChar(end + 1);
        //end = FindChar(value+1, ' ');
        end = FindLastChar(value)+1;
        *end = '\0';
        // get the token for further handling
        do {
          if (stricmp(symtab[++idx].keyword, name) == 0) break;
        } while (symtab[idx].token!=INI_UNKNOWN);
        // call parser if token is fine
        if (symtab[idx].token==INI_UNKNOWN) {
          ERROR("Unknown keyword.");
          lineError = lineNumber;
        }
        else if (symtab[idx].section==TRUE) {
          ERROR("Keyword not valid here.");
          lineError = lineNumber;
        }
        if (parseHandle(config, section, symtab[idx].token, value)) {
          lineError = lineNumber;
        }
      }
    }
    lineNumber ++;
  } while ((ch != -1) && !lineError); // -1 is EOF
  if (!lineError) {
    // call again to notify end of ini file
    parseHandle(config, INI_UNKNOWN, INI_UNKNOWN, NULL); 
  }
  return lineError;
}

// ==========================================================================

uint16_t ParseList(const char* value, ini_list_t *valueList, const uint16_t maxlen) {
  // index to data array
  uint16_t idx=0, len;

  // pointers on value to parse
  const char* start;
  const char* end = value;

  // walk through the entries separated by comma
  do {

    // find start of value and end/separator (# or ; will usually not occur)
    start = FindFirstChar(end);
    end = FindChar(start, ',');  
    len = end-start;

    // clear array entry first, then we copy the string and convert it to decimal
    valueList[idx].strval[len]=0;
    if (len) {
      if (*start=='"') {
        // remove "" but keep string as is
        strncpy(valueList[idx].strval,start+1,len);
        *FindChar(valueList[idx].strval, '"')=0;
      }
      else {
        // take as is, remove any spaces at the end
        strncpy(valueList[idx].strval,start,len);
        StripTrailingSpaces(valueList[idx].strval);
      }
      if (strnicmp(valueList[idx].strval, "*", 1)==0) {      // unsigned: "*<binary>" (not with strtoul and auto-base)
        valueList[idx].intval = (int32_t) strtoul(valueList[idx].strval+1, NULL, 2);  // binary
      }
      else if (strnicmp(valueList[idx].strval, "-", 1)==0) { // signed: "-<decimal>" (saturates 0x80000000/0x7fffffff!)
        valueList[idx].intval = (int32_t) strtol(valueList[idx].strval, NULL, 10);  // decimal
      } else {                                                // unsigned: "0<octal>" or "0x<hex>" or "<decimal>"
        valueList[idx].intval = (uint32_t) strtoul(valueList[idx].strval, NULL, 0);    // auto-detect value
      }
    } else {
      valueList[idx].intval=0;
    }
    if ((++idx)==maxlen) break;

    DEBUG(3,"%d: %s %x",idx-1,valueList[idx-1].strval,valueList[idx-1].intval); // details about values

  } while (*end++); // check for end otherwise "remove" comma and do again

  // return # entries
  return idx;
}
