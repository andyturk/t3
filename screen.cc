#ifdef DEBUG
#include <cstdio>
#include <stdarg.h>
#include "screen.h"

bool pinout_has_been_set = false;

Screen Screen::the_screen;

void Screen::initialize() {
  if (!pinout_has_been_set) {
    PinoutSet();
    pinout_has_been_set = true;
  }

  Kitronix320x240x16_SSD2119Init();
  GrContextInit(&context, &g_sKitronix320x240x16_SSD2119);

  width = GrContextDpyWidthGet(&context);
  height = GrContextDpyHeightGet(&context);
  font = g_pFontFixed6x8;
  foreground = ClrWhite;
  background = ClrBlack;
  GrContextForegroundSet(&context, foreground);
  GrContextBackgroundSet(&context, background);
  GrContextFontSet(&context, font);
  lineheight = GrFontBaselineGet(font) + 1;
  x = y = 0;
}

void Screen::clear() {
  tRectangle sRect;

  sRect.sXMin = 0;
  sRect.sYMin = 0;
  sRect.sXMax = width - 1;
  sRect.sYMax = height - 1;
  GrContextForegroundSet(&context, background);
  GrRectFill(&context, &sRect);
  GrContextForegroundSet(&context, foreground);
  GrFlush(&context);
}

void Screen::text(const char *chars) {
  while (*chars) {
    const char *p = chars;
    while (*p && *p != '\n') ++p;
    uint16_t run = p - chars;

    GrStringDraw(&context, chars, run, x, y, true);
    x += GrStringWidthGet(&context, chars, run);

    if (*p == '\n') {
      ++p; ++run;
      x = 0;
      y += lineheight;

      tRectangle r;
      r.sXMin = x;
      r.sYMin = y;
      r.sXMax = width - 1;
      r.sYMax = y + 3*lineheight;
      GrContextForegroundSet(&context, background);
      GrRectFill(&context, &r);
      GrContextForegroundSet(&context, foreground);
    }

    if (y > (height - lineheight)) {
      y = 0;
    }

    chars += run;
  }

  GrFlush(&context);
}

void Screen::debug_(const char *format, ...) {
  va_list args;
  va_start(args, format);
  char buffer[256];
  vsprintf(buffer, format, args);
  va_end(args);
  text(buffer);
}
#endif
