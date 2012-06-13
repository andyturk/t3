#pragma once

class StateMachine {
 public:
  //typedef void (*State)(StateMachine *);
  typedef void (StateMachine::*State)();

  StateMachine() : method(0), machine(0) {}
  StateMachine(void *sm, State m) :
    method(m),
    machine((StateMachine *) sm)
  {
  }

  inline virtual ~StateMachine() {}

  virtual void operator()() {
    //method(machine);
    (machine->*method)();
  }

  void go(void *m, State s) {
    machine = (StateMachine *) m;
    method = s;
  }

 protected:
  State method;
  StateMachine *machine;

  virtual void dummy() {}
};

