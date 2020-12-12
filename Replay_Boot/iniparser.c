

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
#include <stdlib.h>

// ==========================================================================

// would be nicer in the header, but as it
// produces actually code it must stay here

/** Definitions of the symbols we can detect */
const ini_symtab_t symtab[] = {
    {.keyword = "SETUP",        .token = INI_SETUP,        .section = TRUE  },
    {.keyword = "INFO",         .token = INI_INFO,         .section = FALSE },
    {.keyword = "OSD_INIT",     .token = INI_OSD_INIT,     .section = FALSE },
    {.keyword = "OSD_TIMEOUT",  .token = INI_OSD_TIMEOUT,  .section = FALSE },
    {.keyword = "BIN",          .token = INI_BIN,          .section = FALSE },
    {.keyword = "CLOCK",        .token = INI_CLOCK,        .section = FALSE },
    {.keyword = "PHASE",        .token = INI_PHASE,        .section = FALSE },
    {.keyword = "CODER",        .token = INI_CODER,        .section = FALSE },
    {.keyword = "VFILTER",      .token = INI_VFILTER,      .section = FALSE },
    {.keyword = "EN_TWI",       .token = INI_EN_TWI,       .section = FALSE },
    {.keyword = "EN_SPI",       .token = INI_EN_SPI,       .section = FALSE },
    {.keyword = "SPI_CLK",      .token = INI_SPI_CLK,      .section = FALSE },
    {.keyword = "BUTTON",       .token = INI_BUTTON,       .section = FALSE },
    {.keyword = "HOTKEY",       .token = INI_HOTKEY,       .section = FALSE },
    {.keyword = "KEYBOARD",     .token = INI_KEYB_MODE,    .section = FALSE },
    {.keyword = "CLOCKMON",     .token = INI_CLOCKMON,     .section = FALSE },
    {.keyword = "VIDEO",        .token = INI_VIDEO,        .section = FALSE },
    {.keyword = "CONFIG",       .token = INI_CONFIG,       .section = FALSE },
    {.keyword = "CSTORE",       .token = INI_CSTORE,       .section = FALSE },
    {.keyword = "UPLOAD",       .token = INI_UPLOAD,       .section = TRUE  },
    {.keyword = "FILES",        .token = INI_FILES,        .section = TRUE  },
    {.keyword = "VERIFY",       .token = INI_VERIFY,       .section = FALSE },
    {.keyword = "ROM",          .token = INI_ROM,          .section = FALSE },
    {.keyword = "CHA_MOUNT",    .token = INI_CHA_MOUNT,    .section = FALSE },
    {.keyword = "CHB_MOUNT",    .token = INI_CHB_MOUNT,    .section = FALSE },
    {.keyword = "CHA_CFG",      .token = INI_CHA_CFG,      .section = FALSE },
    {.keyword = "CHB_CFG",      .token = INI_CHB_CFG,      .section = FALSE },
    {.keyword = "DATA",         .token = INI_DATA,         .section = FALSE },
    {.keyword = "LAUNCH",       .token = INI_LAUNCH,       .section = FALSE },
    {.keyword = "MENU",         .token = INI_MENU,         .section = TRUE  },
    {.keyword = "TITLE",        .token = INI_TITLE,        .section = FALSE },
    {.keyword = "ITEM",         .token = INI_ITEM,         .section = FALSE },
    {.keyword = "OPTION",       .token = INI_OPTION,       .section = FALSE },
    {.keyword = "TARGETS",      .token = INI_TARGETS,      .section = TRUE  },
    {.keyword = "TARGET",       .token = INI_TARGET,       .section = FALSE },
    {.keyword = "",             .token = INI_UNKNOWN,      .section = FALSE }, // no match found
    {.keyword = "",             .token = INI_START,        .section = TRUE  } // start value, can never match
};

// ==========================================================================

/** removes trailing spaces */
static char* StripTrailingSpaces(char* s)
{
    char* p = s + strlen(s);                 // TODO:

    while ((p > s) && isspace((int)(*--p))) { // we do int casting to avoid warning
        *p = '\0';
    }

    return s;
}

/** returns pointer to first non-white char */
static char* FindFirstChar(const char* s)
{
    if (!s || (s[0] == 0) ) {
        return NULL;
    }

    while ((*s) && isspace((int)(*s))) { // we do int casting to avoid warning
        s++;
    }

    return (char*) s;
}

/** returns pointer to last non-white char */
static char* FindLastChar(const char* s, uint16_t len)
{
    const char* p = s + len - 1;

    if (!s) {
        return NULL;
    }

    while ((p > s) && isspace((int)(*p))) { // we do int casting to avoid warning
        --p;
    }

    return (char*) p;
}

/** returns pointer to first specified char (or comment) */
static char* FindChar(const char* s, char c)
{
    while (*s && (*s != c) && (*s != ';') && (*s != '#') ) {
        s++;
    }

    return (char*) s;
}

/** returns pointer to last char c, starting from e, searching
    backwards until min. if char is not found, returns NULL
*/
static char* FindCharr(const char* e, const char* min, char c)
{
    const char* p = e - 1;

    while ((p > min) && (*p && (*p != c)) ) {
        --p;
    }

    return (char*) ( (p == min) && (*p != c)  ? NULL : p );
}

// ==========================================================================

static uint32_t ParseLine(uint8_t(*parseHandle)(void*, const ini_symbols_t, const ini_symbols_t, const char*),
                          void* config,
                          ini_symbols_t* section,
                          uint32_t lineNumber,
                          char* lineBuffer)
{
    uint32_t lineError = 0;

    char* start;
    char* end;
    char* name;
    char* value;

    FreeList(NULL, 0);

    start = FindFirstChar(StripTrailingSpaces(lineBuffer));

    if (!start) {
        return 0;
    }

    if (*start == '[') {
        end = FindChar(start + 1, ']'); // find end or comment

        if (*end == ']') {
            int32_t idx = -1;
            *end = '\0';

            // get the token for further handling
            do {
                if (stricmp(symtab[++idx].keyword, start + 1) == 0) {
                    break;
                }
            } while (symtab[idx].token != INI_UNKNOWN);

            *section = symtab[idx].token;

            // check correctness, then call parser to indicate new section
            if (symtab[idx].token == INI_UNKNOWN) {
                ERROR("Unknown keyword. Line %d", lineNumber);
                lineError = lineNumber;

            } else if (symtab[idx].section == FALSE) {
                ERROR("Keyword not valid here. Line %d", lineNumber);
                lineError = lineNumber;

            } else if (parseHandle(config, *section, INI_UNKNOWN, NULL)) {
                lineError = lineNumber;
            }

        } else {
            lineError = lineNumber;
        }

    } else if (*start && (*start != ';') && (*start != '#')) {
        end = FindChar(start + 1, '=');

        if (*end == '=') {
            int32_t idx = -1;
            *end = '\0';
            name = StripTrailingSpaces(start);
            value = FindFirstChar(end + 1);
            end = FindLastChar(value, strlen(value)) + 1;
            *end = '\0';

            // get the token for further handling
            do {
                if (stricmp(symtab[++idx].keyword, name) == 0) {
                    break;
                }
            } while (symtab[idx].token != INI_UNKNOWN);

            // call parser if token is fine
            if (symtab[idx].token == INI_UNKNOWN) {
                ERROR("Unknown keyword. Line %d", lineNumber);
                lineError = lineNumber;

            } else if (symtab[idx].section == TRUE) {
                ERROR("Keyword not valid here. Line %d", lineNumber);
                lineError = lineNumber;
            }

            if (parseHandle(config, *section, symtab[idx].token, value)) {
                lineError = lineNumber;
            }
        }
    }

    return lineError;
}

uint8_t ParseIni(FF_FILE* pFile,
                 uint8_t(*parseHandle)(void*, const ini_symbols_t, const ini_symbols_t, const char*),
                 void* config)
{
    ini_symbols_t section = INI_UNKNOWN;

    char lineBuffer[MAX_LINE_LEN];

    uint32_t i;
    uint32_t lineNumber = 1;
    uint32_t lineError = 0;
    int32_t ch = 0; // note signed

    do {
        // read line from file
        for (i = 0; (i < (sizeof(lineBuffer) - 1) && ((ch = FF_GetC(pFile)) >= 0) && (ch != '\n')); i++) {
            lineBuffer[i] = (char) ch;
        }

        lineBuffer[i] = '\0';

        // read rest of line if it was too big for buffer
        while (( ch != -1) && (ch != '\n')) {
            ch = FF_GetC(pFile);
        }

        lineError = ParseLine(parseHandle, config, &section, lineNumber, lineBuffer);
        lineNumber ++;

    } while ((ch != -1) && !lineError); // -1 is EOF

    if (!lineError) {
        // call again to notify end of ini file
        parseHandle(config, INI_UNKNOWN, INI_UNKNOWN, NULL);
    }

    return lineError;
}

uint8_t ParseIniFromString(const char* str, size_t strlen,
                           uint8_t(*parseHandle)(void*, const ini_symbols_t, const ini_symbols_t, const char*),
                           void* config)
{
    const char* strend = str + strlen;
    ini_symbols_t section = INI_UNKNOWN;
    char lineBuffer[MAX_LINE_LEN];

    uint32_t i;
    uint32_t lineNumber = 1;
    uint32_t lineError = 0;
    int32_t ch = 0; // note signed

    do {
        // read line from file
        for (i = 0; (i < (sizeof(lineBuffer) - 1) && ((ch = (str < strend ? *str++ : -1)) >= 0) && (ch != '\n')); i++) {
            lineBuffer[i] = (char) ch;
        }

        lineBuffer[i] = '\0';

        // read rest of line if it was too big for buffer
        while (( ch != -1) && (ch != '\n')) {
            ch = (str < strend ? *str++ : -1);
        }

        lineError = ParseLine(parseHandle, config, &section, lineNumber, lineBuffer);
        lineNumber ++;

    } while ((ch != -1) && !lineError); // -1 is EOF

    if (!lineError) {
        // call again to notify end of ini file
        parseHandle(config, INI_UNKNOWN, INI_UNKNOWN, NULL);
    }

    return lineError;
}

// ==========================================================================
// Holds the temporary string tokens when parsing a value list
static char s_ValueBuffer[MAX_LINE_LEN];
static uint16_t s_CurrentValueOffset = 0;

void FreeList(ini_list_t* valueList, const uint16_t len)
{
    s_CurrentValueOffset = 0;
}

uint16_t ParseList(const char* value, ini_list_t* valueList, const uint16_t maxlen)
{
    // index to data array
    uint16_t idx = 0, len;

    // pointers on value to parse
    const char* start;
    const char* end = value;

    // walk through the entries separated by comma
    do {
        // find start of value and end/separator (# or ; will usually not occur)
        start = FindFirstChar(end);
        end = FindChar(start, ',');
        len = end - start;

        // clear array entry first, then we copy the string and convert it to decimal
        valueList[idx].strval = NULL;
        valueList[idx].intval = 0;

        if (len) {
            if (*start == '"') { // string for sure
                // remove "" but keep string as is
                start++;
                const char* send = end - 1;

                if (*send != '"') {
                    send = FindCharr(end, start, '"');
                }

                if (send != start) {
                    len = send - start;

                } else {
                    WARNING("missing end quote: %s", start);
                }

                Assert(sizeof(s_ValueBuffer) - s_CurrentValueOffset > len);
                valueList[idx].strval = &s_ValueBuffer[s_CurrentValueOffset];
                s_CurrentValueOffset += len + 1;
                strncpy(valueList[idx].strval, start, len);
                valueList[idx].strval[len] = 0;

            } else {
                // grab as (unquoted) string
                Assert(sizeof(s_ValueBuffer) - s_CurrentValueOffset > len);
                valueList[idx].strval = &s_ValueBuffer[s_CurrentValueOffset];
                s_CurrentValueOffset += len + 1; // +1 for \0
                strncpy(valueList[idx].strval, start, len);
                valueList[idx].strval[len] = 0;
                StripTrailingSpaces(valueList[idx].strval);
            }

            start = valueList[idx].strval;  // temporarily point start to the actual new string

            if (*start == '*') {    // unsigned: "*<binary>" (not with strtoul and auto-base)
                valueList[idx].intval = (int32_t) strtoul(start + 1, NULL, 2); // binary

            } else if (*start == '-') { // signed: "-<decimal>" (saturates 0x80000000/0x7fffffff!)
                valueList[idx].intval = (int32_t) strtol(start + 1, NULL, 10); // decimal

            } else if (*start >= '0' && *start <= '9') { // unsigned:"0<octal>", "0x<hex>" or "<decimal>"
                valueList[idx].intval = (uint32_t) strtoul(start, NULL, 0);    // auto-detect value
            }

        } else {
            valueList[idx].intval = 0;
        }

        if ((++idx) == maxlen) {
            break;
        }

        if ((*end == ';') || (*end == '#')) {
            break; // terminate if comment follows
        }

        DEBUG(3, "%d: %s %x", idx - 1, valueList[idx - 1].strval, valueList[idx - 1].intval); // details about values
    } while (*end++); // check for end otherwise "remove" comma and do again

    // return # entries
    return idx;
}
