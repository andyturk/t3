#pragma once

#include <cstdlib>
#include <stdint.h>
#include "assert.h"

template<class T>
class FlipBuffer {
  T *const storage;
  const size_t capacity;
  size_t position, limit, mark;

 public:
  enum {
    NO_MARK = 0xffffffff
  };

  FlipBuffer(T *s, size_t c) : storage(s), capacity(c), position(0), limit(c), mark(NO_MARK) {}
  size_t get_capacity() const {return capacity;}
  size_t get_position() const {return position;}
  size_t get_limit() const {return limit;}
  size_t get_mark() const {return mark;}
  size_t get_remaining() const {return limit - position;}

  void set_limit(size_t l) {limit = l;}
  void reset() {position = 0; limit = capacity; mark = NO_MARK;}
  void flip() {limit = position; position = 0;}
  void rewind() {position = 0;}
  void set_mark() {mark = position;}
  void back_to_mark() {position = mark;}
  
  T get() {return storage[position++];}
  T peek(int offset) {return storage[position + offset];}
  void put(const T x) {assert(position < limit); storage[position++] = x;}
};

template<class T>
class RingBuffer {
  T *const storage, *const limit;
  T *write_position, *read_position;

 public:
  RingBuffer(T *const s, size_t capacity) :
  storage(s), limit(storage + capacity), write_position(s), read_position(s) {}
  
  void flush() {
    write_position = read_position = storage;
  }

  size_t read_capacity() const {
    if (write_position >= read_position) {
      return (size_t) (write_position - read_position);
    } else {
      return (size_t) ((limit - read_position) + write_position);
    }
  }

  size_t write_capacity() const {
    if (write_position >= read_position) {
      return (size_t) ((limit - write_position) + read_position);
    } else {
      return (size_t) ((read_position - write_position) - 1);
    }
  }

  size_t read(T *dst, size_t n) {
    size_t actually_read = 0;

    while (n > 0) {
      size_t run;

      if (write_position >= read_position) {
        run = write_position - read_position;
      } else {
        run = limit - read_position;
      }

      if (run == 0) break;
      if (n < run) run = n;

      for(uint32_t i=run; i > 0; --i) *dst++ = *read_position++;
      if (read_position == limit) read_position = storage;

      n -= run;
      actually_read += run;
    };

    return actually_read;
  }

  inline T &peek(int offset) const {
    ptrdiff_t capacity = limit - storage;
    assert(abs(offset) < capacity);
    T *p = read_position + offset;
    if (p < storage) p += capacity;
    if (p > limit) p -= capacity;
    return *p;
  }

  void skip(uint32_t offset) {
    ptrdiff_t capacity = limit - storage;
    assert(abs(offset) < capacity);
    read_position += offset;
    if (read_position > limit) read_position -= capacity;
  }

  inline T &poke(int offset) const {
    ptrdiff_t capacity = limit - storage;
    assert(abs(offset) < capacity);
    if (offset < 0) offset += capacity;
    T *p = write_position + offset;
    if (p > limit) p -= capacity;
    if (p < storage) p += capacity;
    return *p;
  }

  size_t write(const T *src, size_t n) {
    size_t written = 0;

    while (n > 0) {
      size_t run;

      if (write_position >= read_position) {
        run = limit - write_position;
      } else {
        run = (read_position - write_position) - 1;
      }

      if (run == 0) break;
      if (n < run) run = n;

      for (uint32_t i=run; i > 0; --i) *write_position++ = *src++;
      if (write_position == limit) write_position = storage;

      n -= run;
      written += run;
    };

    return written;
  }
};

