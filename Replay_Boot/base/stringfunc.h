#ifndef __STRING_H__
#define __STRING_H__

#include <stddef.h>

size_t strlen(const char* str);
size_t strlcpy(char* dst, const char* src, size_t bufsize);
int    stricmp_logical(const char* pS1, const char* pS2);
int    strnicmp(const char* pS1, const char* pS2, size_t n);
int    stricmp(const char* pS1, const char* pS2);
int    strncmp(const char* pS1, const char* pS2, size_t n);
char*  strcpy(char * dst, const char * src);
char*  strncpy(char* dst, const char* src, size_t num);
int    strcmp(const char * str1, const char * str2);
char*  strcat(char* dst, const char* src);
char*  strcasestr(const char *s, const char *find);
char*  strstr(const char *s, const char *find);
char*  strchr(const char *p, int ch);
char*  strrchr(const char *p, int ch);

void*  memcpy (void* dst, const void* src, size_t num);
void*  memset (void * ptr, int value, size_t num );
void   bzero (void * ptr, size_t num );
int    memcmp(const void* ptr1, const void* ptr2, size_t num);

int toupper(int c);
int tolower(int c);
  
// ctype.h
int isalnum(int c);
int isupper(int c);
int isalpha(int c);
int isspace(int c);
int isdigit(int c);

#endif
