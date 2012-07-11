#pragma once

extern "C" {
  void * memcpy(void * dst, void const * src, size_t len);
  int memcmp(const void *x0, const void *y0, size_t len);
  unsigned int strlen(const char *p0);
};
