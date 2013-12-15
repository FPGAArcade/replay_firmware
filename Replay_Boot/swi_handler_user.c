/* User SWI Handler for SWI's not handled in swi_handler.S */

#include <stdio.h>
#include "Board.h"

unsigned long SWI_Handler_User(unsigned long reg0,
	unsigned long reg1,
	unsigned long reg2,
	unsigned long swi_num )
{

	return 0;
}
