#pragma once

#include "fullfat-test.h"

void RunFullTestSuite()
{
    DEBUG(1, "\033[2J");
	RunFullFatTests();
    DEBUG(1, "DONE!");
}
