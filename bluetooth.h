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

template<class Event>
class StateMachine {
 public:
  typedef void (*State)(StateMachine *, Event);
  StateMachine() : state(0) {}
  StateMachine(State s) : state(s) {}
  inline void go(Event e) {(*state)(this, e);}
  inline void go(State s) {state = s;}

 protected:
  State state;
};

class UARTTransportReader : public StateMachine<RingBuffer &> {
 public:
  class Delegate {
  public:
    virtual void event_packet(UARTTransportReader &r) = 0;
    virtual void acl_packet(UARTTransportReader &r) = 0;
    virtual void synchronous_packet(UARTTransportReader &r) = 0;
  };

  UARTTransportReader();
  void set_delegate(Delegate *delegate);
  size_t packet_size;
  uint8_t event_code, acl_bounary, acl_broadcast, synchronous_status;
  uint16_t handle;

 private:
  Delegate *delegate;

  // operating states
  void read_packet_indicator(RingBuffer &b);
  void read_event_code_and_length(RingBuffer &b);
  void read_event_parameters(RingBuffer &b);

  // error states
  void bad_packet_indicator(RingBuffer &b);
};

class Baseband :
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
  virtual void event_packet(UARTTransportReader &packet);
  virtual void acl_packet(UARTTransportReader &packet);
  virtual void synchronous_packet(UARTTransportReader &packet);

  // StateMachine states
  void module_requires_initialization();
};

class Pan1323Bootstrap :
  public StateMachine<UARTTransportReader &>,
  public UARTTransportReader::Delegate
{
  Baseband &baseband;

 public:
  Pan1323Bootstrap(Baseband &b);
  void initialize();

  // UARTTransportReader::Delegate methods
  virtual void event_packet(UARTTransportReader &packet);
  virtual void acl_packet(UARTTransportReader &packet);
  virtual void synchronous_packet(UARTTransportReader &packet);

  // StateMachine states
  void reset_pending(UARTTransportReader &packet);
};


