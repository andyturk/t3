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
  typedef void (*State)(StateMachine *);

  StateMachine(State s);

  inline void go() {(*state)(this);}
  inline void go(State s) {state = s; (*state)(this);}

 protected:
  State state;
};

class UARTTransportReader : public StateMachine {
 public:
  class Delegate {
  public:
    virtual void event_packet(uint8_t code, size_t size) = 0;
    virtual void acl_packet(uint16_t handle, uint8_t boundary, uint8_t broadcast, size_t size) = 0;
    virtual void synchronous_packet(uint16_t handle, uint8_t status, size_t size) = 0;
  };

  UARTTransportReader(RingBuffer &input);
  void set_delegate(Delegate *delegate);

 private:
  Delegate *delegate;
  RingBuffer &input;
  size_t packet_size;
  uint8_t event_code, acl_bounary, acl_broadcast, synchronous_status;
  uint16_t handle;

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
  virtual void event_packet(uint8_t code, size_t size);
  virtual void acl_packet(uint16_t handle, uint8_t boundary, uint8_t broadcast, size_t size);
  virtual void synchronous_packet(uint16_t handle, uint8_t status, size_t size);

  // StateMachine states
  void module_requires_initialization();
};

class Pan1323Bootstrap :
  public StateMachine,
  public UARTTransportReader::Delegate
{
  Baseband &baseband;

 public:
  Pan1323Bootstrap(Baseband &b);

  // UARTTransportReader::Delegate methods
  virtual void event_packet(uint8_t code, size_t size);
  virtual void acl_packet(uint16_t handle, uint8_t boundary, uint8_t broadcast, size_t size);
  virtual void synchronous_packet(uint16_t handle, uint8_t status, size_t size);

  // StateMachine states
  void module_requires_initialization();
  void reset_pending();
};


