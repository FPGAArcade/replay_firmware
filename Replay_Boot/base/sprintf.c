
#include <stdarg.h>
#include "stringfunc.h"
#include "printf.h"

#undef sprintf
int sprintf(char* s, const char* fmt, ...)
{
    *s = 0;
    va_list va;
    va_start(va, fmt);
    tfp_format(&s, putcp, (char*)fmt, va);
    putcp(&s, 0);
    va_end(va);
    return strlen(s);
}
