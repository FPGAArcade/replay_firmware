#define INCBIN_PREFIX _binary_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "../hardware_host/incbin.h"

INCBIN(buildnum, "build/buildnum");
