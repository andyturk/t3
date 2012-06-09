#pragma once

#include <cstddef>
#include <stdarg.h>

#include "hal.h"

namespace HCI {
  enum packet_indicator {
    COMMAND_PACKET          = 0x01,
    ACL_PACKET              = 0x02,
    SYNCHRONOUS_DATA_PACKET = 0x03,
    EVENT_PACKET            = 0x04
  };

#define COMMAND(ogf,ocf,name,send,expect) extern HCI::Command name;
#define EVENT(code,name,args)
#define LE_EVENT(code,name,args)

  struct Command {
    const uint16_t opcode;
    const char *send;
  };

  struct BD_ADDR {
    uint8_t data[6];
  };

  #include "command_defs.h"
};

class StateMachine {
 public:
  //  typedef void (StateMachine::*State)();
  typedef void (*State)(StateMachine *);

  StateMachine();
  inline void go() {(*state)(this);}

 protected:
  State state;

  virtual void start() = 0;
};

class UARTTransportReader : public StateMachine {
 public:
  class Delegate {
  public:
    virtual void event_packet(size_t size) = 0;
    virtual void ACL_packet(size_t size) = 0;
    virtual void synchronous_packet(size_t size) = 0;
  };

  UARTTransportReader(RingBuffer &input, Delegate &delegate);
  virtual void start();

 private:
  Delegate &delegate;
  RingBuffer &input;
  size_t packet_size;

  // operating states
  void read_packet_indicator();
  void read_event_code_and_length();
  void read_event_parameters();
  void event_packet_complete();

  // error states
  void bad_packet_indicator();
};

class Baseband :
  public StateMachine,
  public BufferedUART::Delegate,
  public UARTTransportReader::Delegate
{
 public:
  BufferedUART &uart;
  IOPin &shutdown;
  UARTTransportReader reader;

  Baseband(BufferedUART &uart, IOPin &shutdown);

  void initialize();

  void send(HCI::Command const &cmd, ...);
  void data_received(BufferedUART *u);
  void error_occurred(BufferedUART *u);

  // UARTTransportReader::Delegate methods
  virtual void event_packet(size_t size);
  virtual void ACL_packet(size_t size);
  virtual void synchronous_packet(size_t size);

  // StateMachine states
  virtual void start();
          void shutdown_asserted();
          

};

