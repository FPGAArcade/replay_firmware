#define INCBIN_PREFIX _binary_
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#include "incbin.h"

INCBIN(buildnum, "build/buildnum");
INCBIN(build_loader, "build/loader");
INCBIN(build_replayhand, "build/replayhand");
INCBIN(replay_ini, "../loader_embedded/replay.ini");

char* end = 0;
