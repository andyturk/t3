//////////////////////////////////////////////////////////////////////////////
// Minimal Embedded C++ support, no exception handling, no RTTI
// Date of the Last Update:  Sep 10, 2011
//
//                    Q u a n t u m     L e a P s
//                    ---------------------------
//                    innovating embedded systems
//
// Copyright (C) 2007-2011 Quantum Leaps, LLC. All rights reserved.
//
// Contact information:
// Quantum Leaps Web site:  http://www.quantum-leaps.com
// e-mail:                  info@quantum-leaps.com
//////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>                   // for prototypes of malloc() and free()
#include <sys/unistd.h>
//............................................................................
void *operator new(size_t size) throw() {
  for(;;);
}
//............................................................................
void operator delete(void *p) throw() {
}

extern "C" {

int __aeabi_atexit(void *o, void (*fn)(void *), void *dso_handle) {
  return 0;
}
//............................................................................
void __cxa_atexit(void (*arg1)(void *), void *arg2, void *arg3) {
}
//............................................................................
void __cxa_guard_acquire() {
}
//............................................................................
void __cxa_guard_release() {
}
void __cxa_pure_virtual() {
}
//............................................................................
void *__dso_handle = 0;

}                                                                // extern "C"
