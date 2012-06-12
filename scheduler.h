#pragma once

class Callable {
  Callable *next;

 protected:
  virtual void call() = 0;

 public:
  Callable() : next(0) {}
  inline virtual ~Callable() {}

  void later();
  friend class Scheduler;
};

class Scheduler {
 public:
  static void run_once();
  static void run_forever();

 protected:
  static struct ready_list {
    Callable *head;
    Callable *tail;
  } ready_list;

  friend class Callable;
};
