#include "assert.h"
#include <stdint.h>
#include "scheduler.h"
#include "hal.h"
#include "utils/uartstdio.h"

struct Scheduler::ready_list Scheduler::ready_list = {0, 0};

// http://embeddedfreak.wordpress.com/2009/08/14/cortex-m3-global-interruptexception-control/

void Scheduler::run_once() {
  uint32_t primask;
  Callable *next;

  do {
    next = 0;
    asm volatile ("mrs %0, PRIMASK\n"
                  "cpsid i\n" : "=r" (primask));

    if (ready_list.head != 0) {
      Callable *next = ready_list.head;

      if (ready_list.head == ready_list.tail) {
        ready_list.head = ready_list.tail = 0;
      } else {
        ready_list.head = next->next;
        next->next = 0;
      }
    }

    asm volatile ("msr PRIMASK, %0\n" : "=r" (primask));
    if (next) next->call();
  } while (ready_list.head);
}

void Scheduler::run_forever() {
  do {
    run_once();
    UARTprintf("sleeping...\n");
    asm volatile ("wfi");
  } while (true);  
}

void Callable::later() {
  uint32_t primask;

  asm volatile ("mrs %0, PRIMASK\n"
                "cpsid i\n" : "=r" (primask));

  if (next == 0) {
    if (Scheduler::ready_list.head == 0) {
      // the ready_list is empty
      assert(Scheduler::ready_list.tail == 0);

      Scheduler::ready_list.head = this;
      Scheduler::ready_list.tail = this;
      next = this;
    } else if (next == 0 && Scheduler::ready_list.tail == this) {
      // this is already at the end of the ready_list
    } else if (next != 0) {
      // this is somewhere in the ready_list
    } else {
      // this is not in the ready_list
      assert(Scheduler::ready_list.tail != 0);
      assert(Scheduler::ready_list.tail->next == 0);

      Scheduler::ready_list.tail->next = this;
      Scheduler::ready_list.tail = this;
    }
  }

  asm volatile ("msr PRIMASK, %0\n" : "=r" (primask));
}
