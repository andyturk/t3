#pragma once

#include <stdint.h>

extern "C" {
#include "grlib/grlib.h"
#include "drivers/set_pinout.h"
#include "drivers/kitronix320x240x16_ssd2119_8bit.h"
};

class Screen {
  uint16_t width, height;
  tContext context;
  const tFont *font;
  uint16_t lineheight;
  uint32_t foreground;
  uint32_t background;
  uint16_t x, y;

 public:
  void initialize();
  void clear();
  void text(const char *chars);
  void debug_(const char *format, ...);

  static Screen the_screen;
};

