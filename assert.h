#pragma once

#ifdef DEBUG
#include "utils/uartstdio.h"
#include "screen.h"
// #define debug(...) UARTdebug(__VA_ARGS__)
#define debug(...) Screen::the_screen.debug_(__VA_ARGS__)
#else
#define debug(...) 
#endif

#define assert(x) do {if(!(x)) for(;;);} while (0)

