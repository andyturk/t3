#pragma once

#include <stdint.h>
#include "ring.h"

template<class T>
class PoolBase {
 public:
  Ring<T> available;
  const uint32_t capacity;

  PoolBase(uint32_t cap) : capacity(cap) {}

  T *allocate() {
    T *p = available.begin();
    if (p == available.end()) return 0;

#ifdef __arm__
    __asm("cpsid i");
#endif
    p->join(p);
    p->reset();

#ifdef __arm__
    __asm("cpsie i");
#endif
    return p;
  }

  void deallocate(T *p) {
    assert(p != 0);

#ifdef __arm__
    __asm("cpsid i");
#endif
    ((Ring<T> *) p)->join(&available);
#ifdef __arm__
    __asm("cpsie i");
#endif
  }
};

template<class T, unsigned int size>
class Pool : public PoolBase<T> {
 protected:
  T pool[size];

 public:
  Pool() :
    PoolBase<T>(size)
  {
    reset();
  }

  void reset() {
    for (unsigned int i=0; i < size; ++i) {
      Ring<T> *p = (Ring<T> *) (pool + i);
      p->join(&this->available);
    }
  }
};
