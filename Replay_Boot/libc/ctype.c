#include <ctype.h>

// See http://en.cppreference.com/w/c/string/byte/isalnum et al

int	isalnum(int c)
{
	return isalpha(c) || isdigit(c);
}

int	isalpha(int c)
{
	return isupper(c) || islower(c);
}

int	iscntrl(int c)
{
	return (0x00 <= c && c <= 0x1f) || c == 0x7f;
}
int	isdigit(int c)
{
	return '0' <= c && c <= '9';
}
int	isgraph(int c)
{
	return 0x21 <= c && c <= 0x7e;
}
int	islower(int c)
{
	return 'a' <= c && c <= 'z';
}
int	isprint(int c)
{
	return !iscntrl(c);
}
int	ispunct(int c)
{
	return isgraph(c) && !isalnum(c);
}

int	isspace(int c)
{
	return c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r' || c ==  ' ';
}

int	isupper(int c)
{
	return 'A' <= c && c <= 'Z';
}

int	isxdigit(int c)
{
	return ('A' <= c && c <= 'F') || ('a' <= c && c <= 'f');
}

int	tolower(int c)
{
	if (isupper(c))
		return c | 0x20;
	return c;
}

int	toupper(int c)
{
	if (islower(c))
		return c & ~0x20;
	return c;
}
