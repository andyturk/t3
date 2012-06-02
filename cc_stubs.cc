#include <stdint.h>
#include <cstddef>

void *operator new(size_t size) {
  for(;;);
}

void operator delete(void *p) {
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

}
