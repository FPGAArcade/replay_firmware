#include <errno.h>

static int s_errno_value = 0;
int*  __errno( void )
{
    return  &(s_errno_value);
}
