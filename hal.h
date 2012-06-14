#pragma once

#include <stdint.h>
#include <cstdlib>

#include "ringbuffer.h"
#include "assert.h"

class CPU {
 public:
  static void set_clock_rate_50MHz();
  static uint32_t get_clock_rate();
  static void delay(uint32_t msec);
  static bool set_master_interrupt_enable(bool value);
};

class Peripheral {
 public:
  const void *base;
  const uint32_t id;
  const uint32_t interrupt;

  Peripheral(void *base, uint32_t id, uint32_t interrupt);
  virtual void configure();
  virtual void initialize();

  void set_interrupt_enable(bool value);
  void pend_interrupt();
};

class IOPort : public Peripheral {
 public:
  IOPort(char name);
};

class IOPin : public IOPort {
  const uint8_t mask;

 public:
  const enum pin_type {
    INPUT,
    OUTPUT,
    LED,
    UART
  } type;

  IOPin(char name, uint8_t pin, pin_type type);
  virtual void configure();
  void set_value(bool value);
  bool get_value();
};

class UART : public Peripheral {
 public:
  UART(uint32_t n);
  UART(void *base, uint32_t id);

  enum interrupt_mask {
    RX    = 0x01,
    TX    = 0x02,
    ERROR = 0x04
  };

  virtual void configure() = 0;
  virtual void initialize() {}

  void set_enable(bool value);
  void set_baud(uint32_t bps);
  uint32_t clear_interrupt_cause(uint32_t mask);
  void set_interrupt_sources(uint32_t mask);
  uint32_t disable_all_interrupt_sources();
  void reenable_interrupt_sources(uint32_t mask);
  virtual void flush_rx_fifo();
  virtual void flush_tx_buffer();
  virtual bool can_read();
  virtual bool can_write();
  virtual size_t read(uint8_t *dst, size_t max);
  virtual size_t write(const uint8_t *src, size_t max);
};

class BufferedUART : public UART {
 public:
  class Delegate {
  public:
    virtual void data_received(BufferedUART *u) = 0;
    virtual void error_occurred(BufferedUART *u) = 0;
  };

  BufferedUART(uint32_t n, uint8_t *rx, size_t rx_len, uint8_t *tx, size_t tx_len);
  virtual size_t write(const uint8_t *buffer, size_t length);
  virtual size_t read(uint8_t *buffer, size_t length);
  void skip(size_t length);
  void interrupt_handler();
  void drain_rx_fifo();
  void fill_tx_fifo();
  virtual void flush_tx_buffer();
  inline uint8_t &peek(int offset) { return rx.peek(offset);}
  inline uint8_t &poke(int offset) { return tx.poke(offset);}
  bool set_tx_enable(bool value);
  void set_delegate(Delegate *delegate) {this->delegate = delegate;}
  inline size_t read_capacity() {return rx.read_capacity();}

 protected:
  RingBuffer<uint8_t> rx;
  RingBuffer<uint8_t> tx;
  Delegate *delegate;
  bool tx_enabled;
};

class UART0 : public UART {
 public:
  UART0();
  virtual void configure();
};

class UART1 : public BufferedUART {
  uint8_t rx_buffer[512];
  uint8_t tx_buffer[512];

 public:
  UART1();
  virtual void configure();
};

class SysTick {
  uint32_t msec;

 public:
  SysTick(uint32_t msec);
  void configure();
  void initialize();
};



