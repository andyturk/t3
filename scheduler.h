#pragma once
#include "statemachine.h"

class Callable {
  Callable *next;

 protected:
  virtual void call() = 0;

 public:
  Callable() : next(0) {}
  inline virtual ~Callable() {}

  void ready();
  friend class Scheduler;
};

class CSM : public StateMachine, public Callable {
 public:
  CSM(State start) :
    StateMachine(this, start),
    Callable()
  {}

  virtual void call() {StateMachine::operator()();}
};

class Scheduler {
 public:
  static void run_once();
  static void run_forever();

 protected:
  static struct ready_list_struct {
    Callable *head;
    Callable *tail;
  } ready_list;

  friend class Callable;
};
