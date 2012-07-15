#pragma once

#ifdef DEBUG
#include "utils/uartstdio.h"
#include "screen.h"
// #define printf(...) UARTprintf(__VA_ARGS__)
#define printf(...) Screen::the_screen.printf_(__VA_ARGS__)
#else
#define printf(...) 
#endif

#define assert(x) do {if(!(x)) for(;;);} while (0)

