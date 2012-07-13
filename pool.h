#pragma once

#include "ring.h"

template<class T, unsigned int size>
  class Pool {
 protected:
  T pool[size];
  Ring<T> available;

 public:
  Pool() {
    for (unsigned int i=0; i < size; ++i) {
      Ring<T> *p = (Ring<T> *) (pool + i);
      p->join(&available);
    }
  }

  T *allocate() {
    T *p = available.begin();
    if (p == available.end()) return 0;

    __asm("cpsid i");
    p->join(p);
    p->reset();
    __asm("cpsie i");

    return p;
  }

  void deallocate(T *p) {
    assert(p != 0);

    __asm("cpsid i");
    ((Ring<T> *) p)->join(&available);
    __asm("cpsie i");
  }

  size_t capacity() const {return size;}
};
