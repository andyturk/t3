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
      return read_position - (write_position + 1);
    }
  }

  size_t read(T *dst, size_t n) {
    size_t read_count = 0;

    while (n > 0 && read_capacity() > 0) {
      size_t run = (write_position >= read_position) ?
        (write_position - read_position) : (capacity - read_position);
      if (n < run) run = n;
      for (size_t i=0; i < run; ++i) dst[i] = buffer[read_position + i];
      read_position += run;
      if (read_position == capacity) read_position = 0;
      read_count += run;
      n -= run;
    };

    return read_count;
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
    size_t write_count = 0;

    while (n > 0 && write_capacity() > 0) {
      size_t run = (write_position >= read_position) ?
        (capacity - write_position) : (read_position - write_position);
      if (n < run) run = n;
      for (size_t i=0; i < run; ++i) buffer[write_position + i] = src[i];
      write_position += run;
      if (write_position == capacity) write_position = 0;
      write_count += run;
      n -= run;
    };

    return write_count;
  }
};

