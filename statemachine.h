#pragma once

/*
 * Classes derived from StateMachine should have it as the first
 * first superclass, otherwise derived virtual states won't work.
 */
class StateMachine {
 public:
  typedef void (StateMachine::*State)();

  StateMachine() :
    method(0),
    machine(0)
  {
  }

  StateMachine(void *sm, State m) :
    method(m),
    machine((StateMachine *) sm)
  {
  }

  inline virtual ~StateMachine() {}

  virtual void operator()() {
    (machine->*method)();
  }

  void go(void *m, State s) {
    machine = (StateMachine *) m;
    method = s;
  }

 protected:
  State method;
  StateMachine *machine;
};

