#pragma once

#include <stdint.h>
#include <cstdlib>

#include "buffer.h"
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
  void *base;
  uint32_t id;
  uint32_t interrupt;

  Peripheral();
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
  virtual void initialize();

  void set_enable(bool value);
  void set_baud(uint32_t bps);
  void set_fifo_enable(bool value);
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

class UART_0 : public UART {
 public:
  UART_0();
  virtual void configure();
};

class UART_1 : public UART {
 public:
  UART_1();
  virtual void configure();
};

class Systick {
  uint32_t msec;

 public:
  Systick(uint32_t msec);
  void configure();
  void initialize();
};

class ADC : public Peripheral {
 public:
  enum sequence_trigger {
    PROCESSOR = 0x00000000,  // Processor event             
    COMP0     = 0x00000001,  // Analog comparator 0 event   
    COMP1     = 0x00000002,  // Analog comparator 1 event   
    COMP2     = 0x00000003,  // Analog comparator 2 event   
    EXTERNAL  = 0x00000004,  // External event              
    TIMER     = 0x00000005,  // Timer event                 
    ADC_PWM0      = 0x00000006,  // PWM0 event                  
    ADC_PWM1      = 0x00000007,  // PWM1 event                  
    ADC_PWM2      = 0x00000008,  // PWM2 event                  
    ADC_PWM3      = 0x00000009,  // PWM3 event                  
    ALWAYS    = 0x0000000F,  // Always event                
  };

  enum control {
    TS              = 0x00000080,  // Temperature sensor select
    IE              = 0x00000040,  // Interrupt enable
    END             = 0x00000020,  // Sequence end select
    D               = 0x00000010,  // Differential select
  };

  enum input {
    CH0             = 0x00000000,  // Input channel 0
    CH1             = 0x00000001,  // Input channel 1
    CH2             = 0x00000002,  // Input channel 2
    CH3             = 0x00000003,  // Input channel 3
    CH4             = 0x00000004,  // Input channel 4
    CH5             = 0x00000005,  // Input channel 5
    CH6             = 0x00000006,  // Input channel 6
    CH7             = 0x00000007,  // Input channel 7
    CH8             = 0x00000008,  // Input channel 8
    CH9             = 0x00000009,  // Input channel 9
    CH10            = 0x0000000A,  // Input channel 10
    CH11            = 0x0000000B,  // Input channel 11
    CH12            = 0x0000000C,  // Input channel 12
    CH13            = 0x0000000D,  // Input channel 13
    CH14            = 0x0000000E,  // Input channel 14
    CH15            = 0x0000000F,  // Input channel 15
    CH16            = 0x00000100,  // Input channel 16
    CH17            = 0x00000101,  // Input channel 17
    CH18            = 0x00000102,  // Input channel 18
    CH19            = 0x00000103,  // Input channel 19
    CH20            = 0x00000104,  // Input channel 20
    CH21            = 0x00000105,  // Input channel 21
    CH22            = 0x00000106,  // Input channel 22
    CH23            = 0x00000107,  // Input channel 23
  };

  enum comparator {
    NO_CMP          = 0,           // no comparator
    CMP0            = 0x00080000,  // Select Comparator 0
    CMP1            = 0x00090000,  // Select Comparator 1
    CMP2            = 0x000A0000,  // Select Comparator 2
    CMP3            = 0x000B0000,  // Select Comparator 3
    CMP4            = 0x000C0000,  // Select Comparator 4
    CMP5            = 0x000D0000,  // Select Comparator 5
    CMP6            = 0x000E0000,  // Select Comparator 6
    CMP7            = 0x000F0000,  // Select Comparator 7
  };

  enum comparator_trigger {
    COMP_TRIG_NONE         = 0x00000000,  // Trigger Disabled
    COMP_TRIG_LOW_ALWAYS   = 0x00001000,  // Trigger Low Always
    COMP_TRIG_LOW_ONCE     = 0x00001100,  // Trigger Low Once
    COMP_TRIG_LOW_HALWAYS  = 0x00001200,  // Trigger Low Always (Hysteresis)
    COMP_TRIG_LOW_HONCE    = 0x00001300,  // Trigger Low Once (Hysteresis)
    COMP_TRIG_MID_ALWAYS   = 0x00001400,  // Trigger Mid Always
    COMP_TRIG_MID_ONCE     = 0x00001500,  // Trigger Mid Once
    COMP_TRIG_HIGH_ALWAYS  = 0x00001C00,  // Trigger High Always
    COMP_TRIG_HIGH_ONCE    = 0x00001D00,  // Trigger High Once
    COMP_TRIG_HIGH_HALWAYS = 0x00001E00,  // Trigger High Always (Hysteresis)
    COMP_TRIG_HIGH_HONCE   = 0x00001F00,  // Trigger High Once (Hysteresis)
  };

  enum comparator_interrupt {
    COMP_INT_NONE         = 0x00000000,  // Interrupt Disabled
    COMP_INT_LOW_ALWAYS   = 0x00000010,  // Interrupt Low Always
    COMP_INT_LOW_ONCE     = 0x00000011,  // Interrupt Low Once
    COMP_INT_LOW_HALWAYS  = 0x00000012,  // Interrupt Low Always (Hysteresis)
    COMP_INT_LOW_HONCE    = 0x00000013,  // Interrupt Low Once (Hysteresis)
    COMP_INT_MID_ALWAYS   = 0x00000014,  // Interrupt Mid Always
    COMP_INT_MID_ONCE     = 0x00000015,  // Interrupt Mid Once
    COMP_INT_HIGH_ALWAYS  = 0x0000001C,  // Interrupt High Always
    COMP_INT_HIGH_ONCE    = 0x0000001D,  // Interrupt High Once
    COMP_INT_HIGH_HALWAYS = 0x0000001E,  // Interrupt High Always (Hysteresis)
    COMP_INT_HIGH_HONCE   = 0x0000001F,  // Interrupt High Once (Hysteresis)
  };

  enum processor_trigger {
    WAIT        = 0x08000000,  // Wait for the synchronous trigger
    SIGNAL      = 0x80000000,  // Signal the synchronous trigger
  };

  enum phase {
    PHASE_0             = 0x00000000,  // 0 degrees
    PHASE_22_5          = 0x00000001,  // 22.5 degrees
    PHASE_45            = 0x00000002,  // 45 degrees
    PHASE_67_5          = 0x00000003,  // 67.5 degrees
    PHASE_90            = 0x00000004,  // 90 degrees
    PHASE_112_5         = 0x00000005,  // 112.5 degrees
    PHASE_135           = 0x00000006,  // 135 degrees
    PHASE_157_5         = 0x00000007,  // 157.5 degrees
    PHASE_180           = 0x00000008,  // 180 degrees
    PHASE_202_5         = 0x00000009,  // 202.5 degrees
    PHASE_225           = 0x0000000A,  // 225 degrees
    PHASE_247_5         = 0x0000000B,  // 247.5 degrees
    PHASE_270           = 0x0000000C,  // 270 degrees
    PHASE_292_5         = 0x0000000D,  // 292.5 degrees
    PHASE_315           = 0x0000000E,  // 315 degrees
    PHASE_337_5         = 0x0000000F,  // 337.5 degrees
  }; 

  enum voltage_reference {
    INT             = 0x00000000,  // Internal reference
    EXT_3V          = 0x00000001,  // External 3V reference
    EXT_1V          = 0x00000003,  // External 1V reference
  };

  enum resolution {
    BITS_10           = 0x00000000,  // 10-bit resolution
    BITS_12           = 0x00000010,  // 12-bit resolution
  };

  enum sequence {
    SEQ_0,
    SEQ_1,
    SEQ_2,
    SEQ_3,
  };

  enum fstat_fields {
    FSTAT_FULL       = 0x00001000,  // FIFO Full
    FSTAT_EMPTY      = 0x00000100,  // FIFO Empty
    FSTAT_HPTR_M     = 0x000000F0,  // FIFO Head Pointer
    FSTAT_TPTR_M     = 0x0000000F,  // FIFO Tail Pointer
    FSTAT_HPTR_S     = 4,
    FSTAT_TPTR_S     = 0,
  };

  ADC(uint32_t n);

  void configure_sequence(sequence seq, sequence_trigger trig, sequence priority);
  void configure_step(sequence seq, unsigned int step, control sc, input is, comparator cs);
  void set_sequence_enable(sequence seq, bool value);
  void clear_interrupt(sequence seq);
  void processor_trigger(sequence seq, bool wait=false, bool signal=false);
  uint32_t get_interrupt_status(sequence seq, bool masked);
  uint32_t get_samples(sequence seq, uint32_t *buffer, uint32_t size);
};


