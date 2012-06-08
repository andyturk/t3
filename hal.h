#pragma once

#include <stdint.h>
#include <cstdlib>

#define assert(x) do {if(!(x)) for(;;);} while (0)

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

  virtual void flush_rx_fifo();
  virtual bool can_read();
  virtual uint8_t read1();
  virtual void write1(uint8_t c);
  virtual bool can_write();
  virtual size_t read(uint8_t *dst, size_t max);
  virtual size_t write(const uint8_t *src, size_t max);
};

class RingBuffer {
  uint8_t *const buffer;
  const size_t capacity;
  uint32_t read_position, write_position;

 public:
  RingBuffer(uint8_t *const buffer, size_t capacity) :
    buffer(buffer), capacity(capacity), read_position(0), write_position(0) {}
  
  void flush() {
    read_position = write_position = 0;
  }

  size_t read_capacity() const {
    size_t cap = 0xffffffff;

    if (write_position >= read_position) {
      cap = write_position - read_position;
    } else {
      cap = (capacity - read_position) + write_position;
    }

    return cap;
  }

  size_t write_capacity() const {
    if (write_position >= read_position) {
      return (capacity - write_position) + read_position;
    } else {
      return read_position - (write_position + 1);
    }
  }

  /*
   * 
   */
  size_t read(uint8_t *dst, size_t n) {
    size_t read_count = 0;

    while (n > 0 && read_capacity() > 0) {
      size_t run = (write_position >= read_position) ?
        (write_position - read_position) : (capacity - read_position);
      if (n < run) run = n;
      for (size_t i=0; i < run; ++i) dst[i] = buffer[read_position + i];
      read_position += run;
      if (read_position == capacity) read_position = 0;
      read_count += run;
      n -= run;
    };

    return read_count;
  }

  inline bool read1(uint8_t &dst) {
    return read(&dst, 1) == 1;
  }

  size_t write(const uint8_t *src, size_t n) {
    size_t write_count = 0;

    while (n > 0 && write_capacity() > 0) {
      size_t run = (write_position >= read_position) ?
        (capacity - write_position) : (read_position - write_position);
      if (n < run) run = n;
      for (size_t i=0; i < run; ++i) buffer[write_position + i] = src[i];
      write_position += run;
      if (write_position == capacity) write_position = 0;
      write_count += run;
      n -= run;
    };

    return write_count;
  }

  inline bool write1(const uint8_t &src) {
    return write(&src, 1) == 1;
  }
};

class BufferedUART : public UART {
 public:
  class Delegate {
  public:
    virtual void data_received(BufferedUART *u);
    virtual void error_occurred(BufferedUART *u);
  };

  RingBuffer rx;
  RingBuffer tx;

  BufferedUART(uint32_t n, uint8_t *rx, size_t rx_len, uint8_t *tx, size_t tx_len);
  virtual size_t write(const uint8_t *buffer, size_t length);
  virtual void write1(uint8_t c);
  void interrupt_handler();

 protected:
  void drain_rx_fifo();
  void fill_tx_fifo();
  Delegate *delegate;
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



