#include "stringfunc.h"
#include "config.h"

/* $origo: tolower.c,2017.05 */
int tolower(int c)
{
    if( c > 0x60) { 
        return c;
    }
    else if(c > 0x40 && c < 0x5a) {
        return c + 0x20;
    } else {
        return c;
    }
}

int toupper(int c)
{
    if( c < 0x60) { 
        return c;
    }
    else if(c > 0x60 && c < 0x7a) {
        return c - 0x20;
    } else {
        return c;
    }
}

int isupper(int c)
{
    return (c >= 'A' && c <= 'Z');
}

int isalpha(int c)
{
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}

/**
 * isspace()
 * checks for white-space characters.  In the "C" and "POSIX" locales, these are: space,
 * form-feed ('\f'), newline ('\n'), carriage return ('\r'), horizontal tab ('\t'), and
 *  vertical tab ('\v').
 */
int isspace(int c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f');
}

int isdigit(int c)
{
    return (c >= '0' && c <= '9');
}

int isalnum(int c) {
  return isalpha(c) || isdigit(c);
}




int stricmp_logical(const char* pS1, const char* pS2)
{
    char c1, c2;
    int v;

    do {
        c1 = *pS1;
        c2 = *pS2;

        if (!c1 || !c2) {
            v = c1 - c2;
            break;
        }

        int d1 = isdigit((uint8_t)c1);
        int d2 = isdigit((uint8_t)c2);

        if (d1 || d2) {
            if (d1 && !d2) {
                return -1;

            } else if (!d1 && d2) {
                return 1;
            }

            char* s1, *s2;
            d1 = strtoul(pS1, &s1, 10) & 0x7fffffff;
            d2 = strtoul(pS2, &s2, 10) & 0x7fffffff;
            v = d1 - d2;
            pS1 = s1;
            pS2 = s2;

        } else {
            v = (unsigned int)tolower((uint8_t)c1) - (unsigned int)tolower((uint8_t)c2);
            pS1++;
            pS2++;
        }
    } while ((v == 0) && (c1 != '\0') && (c2 != '\0'));

    return v;
}



int strnicmp(const char* pS1, const char* pS2, size_t n)
{
    char c1, c2;
    int v;

    do {
        c1 = *pS1++;
        c2 = *pS2++;
        v = (unsigned int)tolower(c1) - (unsigned int)tolower(c2);
    } while ((v == 0) && (c1 != '\0') && (c2 != '\0') && (--n > 0));

    return v;
}

int stricmp(const char* pS1, const char* pS2)
{
  return strnicmp(pS1, pS2, strlen(pS1));
}

int strncmp(const char* pS1, const char* pS2, size_t n)
{
    char c1, c2;
    int v;

    do {
        c1 = *pS1++;
        c2 = *pS2++;
        v = (unsigned int)(c1) - (unsigned int)(c2);
    } while ((v == 0) && (c1 != '\0') && (c2 != '\0') && (--n > 0));

    return v;
}




/*
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


/*	$OpenBSD: strlen.c,v 1.7 2005/08/08 08:05:37 espie Exp $	*/
size_t strlen(const char *str)
{
	const char *s;

	for (s = str; *s; ++s)
		;
	return (s - str);
}

/*	$OpenBSD: strlcpy.c,v 1.11 2006/05/05 15:27:38 millert Exp $	*/
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0) {
		while (--n != 0) {
			if ((*d++ = *s++) == '\0')
				break;
		}
  }

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}


/*	$OpenBSD: rindex.c,v 1.6 2005/08/08 08:05:37 espie Exp $ */
char* strrchr(const char *p, int ch)
{
	char *save;
  
	for (save = NULL;; ++p) {
		if (*p == ch)
			save = (char *)p;
		if (!*p)
			return(save);
  }
	/* NOTREACHED */
}

/*	$OpenBSD: index.c,v 1.5 2005/08/08 08:05:37 espie Exp $ */
char* strchr(const char *p, int ch)
{
	for (;; ++p) {
		if (*p == ch)
			return((char *)p);
		if (!*p)
			return((char *)NULL);
  }
	/* NOTREACHED */
}

/*
 * Find the first occurrence of find in s.
 */
/*	$OpenBSD: strstr.c,v 1.5 2005/08/08 08:05:37 espie Exp $ */
char* strstr(const char *s, const char *find)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != 0) {
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == 0)
					return (NULL);
			} while (sc != c);
		} while (strncmp(s, find, len) != 0);
		s--;
    }
	return ((char *)s);
}

/*	$OpenBSD: strcpy.c,v 1.8 2005/08/08 08:05:37 espie Exp $	*/
char* strcpy(char *to, const char *from)
{
	char *save = to;

	for (; (*to = *from) != '\0'; ++from, ++to);
	return(save);
}

/*
 * Copy src to dst, truncating or null-padding to always copy n bytes.
 * Return dst.
 */
/*	$OpenBSD: strncpy.c,v 1.6 2005/08/08 08:05:37 espie Exp $	*/
char* strncpy(char *dst, const char *src, size_t n)
{
	if (n != 0) {
		char *d = dst;
		const char *s = src;

		do {
			if ((*d++ = *s++) == 0) {
				/* NUL pad the remaining n-1 bytes */
				while (--n != 0)
					*d++ = 0;
      break;
  }
		} while (--n != 0);
	}
	return (dst);
}


void* memcpy (void* dst, const void* src, size_t num)
{
    const uint8_t *t = src;
    for (uint8_t* p = dst; num--; p++,t++) {
        *p = *t;
    }
    return dst;
}

void* memset (void * ptr, int value, size_t num )
{
    uint8_t v = value & 0xff;
    for (uint8_t* p = ptr; num--; p++) {
        *p = v;
    }
    return ptr;
}

void bzero (void *p, size_t n) {
  memset(p, 0, n);
}

int memcmp (const void* ptr1, const void* ptr2, size_t num)
{
    const uint8_t* p1 = ptr1;
    const uint8_t* p2 = ptr2;
    for ( ; num--; p1++, p2++) {
        int d = *p1 - *p2;
        if (d != 0)
            return d;
    }
    return 0;
}

int strcmp(const char * str1, const char * str2)
{
    const char* p1 = str1;
    const char* p2 = str2;
    for ( ; *p1 && *p2; p1++, p2++) {
        int d = *p1 - *p2;
        if (d != 0)
            return d;
    }
    return *p1 - *p2;
}

char* strcat(char* dst, const char* src)
{
    char* p = dst;

    while (*p)
      p++;
    while (*src)
      *p++ = *src++;
    return dst;
}


/*
 * Find the first occurrence of find in s, ignore case.
 */
char* strcasestr(const char *s, const char *find)
{
  char c, sc;
  size_t len;

  if ((c = *find++) != 0) {
    c = (char)tolower((unsigned char)c);
    len = strlen(find);
    do {
      do {
        if ((sc = *s++) == 0)
          return (NULL);
      } while ((char)tolower((unsigned char)sc) != c);
    } while (strnicmp(s, find, len) != 0);
    s--;
  }
  return ((char *)s);
}

#ifndef LONG_MIN
#define LONG_MIN (-(1UL<<31))
#endif
#ifndef LONG_MAX
#define LONG_MAX (1UL<<31)
#endif


/*
 * Convert a string to a long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
long
strtol(nptr, endptr, base)
	const char *nptr;
	char **endptr;
	int base;
{
	const char *s = nptr;
	unsigned long acc;
	int c;
	unsigned long cutoff;
	int neg = 0, any, cutlim;

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else if (c == '+')
		c = *s++;
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	} else if ((base == 0 || base == 2) &&
	    c == '0' && (*s == 'b' || *s == 'B')) {
		c = s[1];
		s += 2;
		base = 2;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for longs is
	 * [-2147483648..2147483647] and the input base is 10,
	 * cutoff will be set to 214748364 and cutlim to either
	 * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
	 * a value > 214748364, or equal but the next digit is > 7 (or 8),
	 * the number is too big, and we will return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	cutoff = neg ? -(unsigned long)LONG_MIN : LONG_MAX;
	cutlim = cutoff % (unsigned long)base;
	cutoff /= (unsigned long)base;
	for (acc = 0, any = 0;; c = *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= base;
			acc += c;
		}
	}
	if (any < 0) {
		acc = neg ? LONG_MIN : LONG_MAX;
//		errno = ERANGE;
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *)(any ? s - 1 : nptr);
	return (acc);
}

unsigned long
strtoul(nptr, endptr, base)
	const char *nptr;
	char **endptr;
	int base;
{
    return (unsigned long)strtol(nptr, endptr, base);
}
