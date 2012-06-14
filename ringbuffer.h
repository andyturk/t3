#pragma once

#include <cstdlib>
#include "assert.h"

template<class T>
class RingBuffer {
  T *const buffer;
  const size_t capacity;
  uint32_t read_position, write_position;

 public:
  RingBuffer(T *const buffer, size_t capacity) :
    buffer(buffer), capacity(capacity), read_position(0), write_position(0) {}
  
  void flush() {
    read_position = write_position = 0;
  }

  size_t read_capacity() const {
    size_t cap = 0xffffffff;

    if (write_position >= read_position) {
      cap = write_position - read_position;
    } else {
      cap = (capacity - read_position) + write_position;
    }

    return cap;
  }

  size_t write_capacity() const {
    if (write_position >= read_position) {
      return (capacity - write_position) + read_position;
    } else {
      return (read_position - write_position) - 1;
    }
  }

  size_t read(T *dst, size_t n) {
    size_t actually_read = 0;

    while (n > 0) {
      size_t run;

      if (write_position >= read_position) {
        run = write_position - read_position;
      } else {
        run = capacity - read_position;
      }

      if (run == 0) break;
      if (n < run) run = n;

      T *p = buffer + read_position;
      for(uint32_t i=run; i > 0; --i) *dst++ = *p++;
      read_position += run;
      if (read_position == capacity) read_position = 0;

      n -= run;
      actually_read += run;
    };

    return actually_read;
  }

  inline T &peek(int offset) const {
    assert(abs(offset) < capacity);
    if (offset < 0) offset += capacity;
    uint32_t position = read_position + offset;
    if (position > capacity) position -= capacity;
    return buffer[position];
  }

  void skip(uint32_t offset) {
    assert(offset < capacity);
    read_position += offset;
    if (read_position > capacity) read_position -= capacity;
  }

  inline T &poke(int offset) const {
    assert(abs(offset) < capacity);
    if (offset < 0) offset += capacity;
    uint32_t position = write_position + offset;
    if (position > capacity) position -= capacity;
    return buffer[position];
  }

  size_t write(const T *src, size_t n) {
    size_t written = 0;

    while (n > 0) {
      size_t run;

      if (write_position >= read_position) {
        run = capacity - write_position;
      } else {
        run = (read_position - write_position) - 1;
      }

      if (run == 0) break;
      if (n < run) run = n;

      T *p = buffer + write_position;
      for (uint32_t i=run; i > 0; --i) *p++ = *src++;
      write_position += run;
      if (write_position == capacity) write_position = 0;

      n -= run;
      written += run;
    };

    return written;
  }
};

