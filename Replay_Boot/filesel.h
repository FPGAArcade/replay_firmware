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

typedef struct
{
  char       file_ext[4]; // extension string used for scan (including /0)
  char*      pPath;       // pointer to the path

  char       file_filter[11]; // filter string used for scan (including /0)
  uint8_t    file_filter_len;

  uint32_t   total_entries; // files + directories
  uint8_t    prevc;
  uint8_t    nextc;
  uint8_t    refc; // ref valid

  uint8_t    offset; // 128 = ref, >128 next, <128 prev
  uint8_t    sel; // as above

  FF_DIRENT  dPrev[MAXDIRENTRIES];
  FF_DIRENT  dRef;
  FF_DIRENT  dNext[MAXDIRENTRIES];
} tDirScan;

void Filesel_ScanUpdate(tDirScan* dir_entries);

void Filesel_ScanFirst(tDirScan* dir_entries);
void Filesel_ScanFind(tDirScan* dir_entries, uint8_t search);
void Filesel_Init(tDirScan* dir_entries, char* pPath, char* pExt);
void Filesel_ChangeDir(tDirScan* dir_entries, char* pPath);
void Filesel_AddFilterChar(tDirScan* dir_entries, char letter);
void Filesel_DelFilterChar(tDirScan* dir_entries);

FF_DIRENT Filesel_GetEntry(tDirScan* dir_entries, uint8_t entry);
FF_DIRENT Filesel_GetLine(tDirScan* dir_entries, uint8_t pos);
uint8_t   Filesel_GetSel(tDirScan* dir_entries);

uint8_t Filesel_Update(tDirScan* dirdir_entries, uint8_t opt);



#endif
