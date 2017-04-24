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
#ifndef FILESEL_H_INCLUDED
#define FILESEL_H_INCLUDED
#include "board.h"
#include "fullfat.h"

#define MAXDIRENTRIES 12  // display value (+1) --> see MENU_HEIGHT in menu.h

#define SCAN_NEXT  2       // find next file in directory
#define SCAN_PREV  3       // find previous file in directory
#define SCAN_NEXT_PAGE  4 // find next 8 files in directory
#define SCAN_PREV_PAGE  5 // find previous 8 files in directory

#define SCAN_OK 0
#define SCAN_END 0

typedef struct file_ext {
    char ext[4];  // "EXT\0"
} file_ext_t;

typedef struct {
    const file_ext_t* file_exts;  // list of extension strings used for scan (including /0)
    char*      pPath;       // pointer to the path

    char       file_filter[12]; // filter string used for scan (including /0)

    uint32_t   total_entries; // files + directories
    uint8_t    prevc;
    uint8_t    nextc;
    uint8_t    refc; // ref valid

    uint8_t    offset; // 128 = ref, >128 next, <128 prev
    uint8_t    sel; // as above

    FF_DIRENT  dPrev[MAXDIRENTRIES];
    FF_DIRENT  dRef;
    FF_DIRENT  dNext[MAXDIRENTRIES];
    const FF_DIRENT dNull;    /*static char null_string[] = "";*/
} tDirScan;

void Filesel_ScanUpdate(tDirScan* dir_entries);

void Filesel_ScanFirst(tDirScan* dir_entries);
void Filesel_ScanFind(tDirScan* dir_entries, uint8_t search);
void Filesel_Init(tDirScan* dir_entries, char* pPath, const file_ext_t* pExt);
void Filesel_ChangeDir(tDirScan* dir_entries, char* pPath);
void Filesel_AddFilterChar(tDirScan* dir_entries, char letter);
void Filesel_DelFilterChar(tDirScan* dir_entries);

FF_DIRENT Filesel_GetEntry(tDirScan* dir_entries, uint8_t entry);
FF_DIRENT Filesel_GetLine(tDirScan* dir_entries, uint8_t pos);
uint8_t   Filesel_GetSel(tDirScan* dir_entries);

uint8_t Filesel_Update(tDirScan* dirdir_entries, uint8_t opt);



#endif
