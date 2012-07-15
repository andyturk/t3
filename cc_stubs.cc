#include <stdint.h>
#include <cstddef>
#include "assert.h"

extern "C" uint8_t __heap_start__;
extern "C" uint8_t __heap_end__;

enum {HEAP_ALIGNMENT = 4};
static uint8_t *unused_heap = &__heap_start__;

void *allocate_heap_space(size_t size) {
  if (size & (HEAP_ALIGNMENT-1)) {
    size += HEAP_ALIGNMENT - (size & (HEAP_ALIGNMENT-1));
  }

  assert((unused_heap + size) < &__heap_end__);
  __asm ("cpsie i");
  void *value = (void *) unused_heap;
  unused_heap += size;
  __asm ("cpsid i");
  return value;
}

extern "C" void *_sbrk(int nbytes) {
  return allocate_heap_space((size_t) nbytes);
}

void *operator new(size_t size) {
  return allocate_heap_space(size);
}

void *operator new[](size_t size) {
  return allocate_heap_space(size);
}

void operator delete(void *p) {
  assert(false);
}

extern "C" {
  int __aeabi_atexit(void *o, void (*fn)(void *), void *dso_handle) {
    return 0;
  }

  void __cxa_atexit(void (*arg1)(void *), void *arg2, void *arg3) {
  }

  void __cxa_guard_acquire() {
  }

  void __cxa_guard_release() {
  }

  void __cxa_pure_virtual() {
  }

  void *__dso_handle = 0;

  void __libc_init_array() {
    extern void (*__init_array_start []) (void) __attribute__((weak));
    extern void (*__init_array_end []) (void) __attribute__((weak));

    int count = __init_array_end - __init_array_start;
    for (int i = 0; i < count; i++) __init_array_start[i]();
  }

  void /* __attribute__ ((__interrupt__)) */ reset_handler() {
    extern int main();
    extern unsigned __data_start;    /* start of .data in the linker script */
    extern unsigned __data_end__;      /* end of .data in the linker script */
    extern unsigned const __data_load;  /* initialization values for .data  */
    extern unsigned __bss_start__;    /* start of .bss in the linker script */
    extern unsigned __bss_end__;        /* end of .bss in the linker script */
    unsigned const *src;
    unsigned *dst;

                 /* copy the data segment initializers from flash to RAM... */
    src = &__data_load;
    for (dst = &__data_start; dst < &__data_end__; ++dst, ++src) {
        *dst = *src;
    }

                                           /* zero fill the .bss segment... */
    for (dst = &__bss_start__; dst < &__bss_end__; ++dst) {
        *dst = 0;
    }

             /* call all static construcors in C++ (harmless in C programs) */
    __libc_init_array();

                                      /* call the application's entry point */
    main();

    for(;;);
  }

  void * memcpy(void * dst, void const * src, size_t len)
  {
    long * plDst = (long *) dst;
    long const * plSrc = (long const *) src;
    if (!(((uint32_t) src) & 0xFFFFFFFC) && !(((uint32_t) dst) & 0xFFFFFFFC))
      {
        while (len >= 4)
          {
            *plDst++ = *plSrc++;
            len -= 4;
          }
      }
    char * pcDst = (char *) plDst;
    char const * pcSrc = (char const *) plSrc;
    while (len--)
      {
        *pcDst++ = *pcSrc++;
      }
    return (dst);
  }

  int memcmp(const void *x0, const void *y0, size_t len) {
    if (len > 0) {
      const uint8_t *x = (const uint8_t *) x0;
      const uint8_t *y = (const uint8_t *) y0;

      do {
        if (*x++ != *y++) return *--x - *--y;
      } while (--len > 0);
    }
    return 0;
  }

  unsigned int strlen(const char *p0) {
    const char *p = p0;
    while (*p) p++;
    ++p;
    return p - p0;
  }
}
