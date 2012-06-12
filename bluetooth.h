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

  struct Command {
    const uint16_t opcode;
    const char *send;
  };

  struct BD_ADDR {
    uint8_t data[6];
  };

  #define BEGIN_COMMANDS
  #define COMMAND(ogf,ocf,name,send,expect) extern HCI::Command name;
  #define END_COMMANDS

  #define BEGIN_EVENTS enum {
  #define EVENT(code,name,args) EVENT_##name = code,
  #define END_EVENTS };

  #define BEGIN_LE_EVENTS
  #define LE_EVENT(code,name,args)
  #define END_LE_EVENTS

  #include "command_defs.h"

  #undef BEGIN_COMMANDS
  #undef COMMAND
  #undef END_COMMANDS

  #undef BEGIN_EVENTS
  #undef EVENT
  #undef END_EVENTS

  #undef BEGIN_LE_EVENTS
  #undef LE_EVENT
  #undef END_LE_EVENTS
};

template<class Event>
class StateMachine {
 public:
  typedef void (StateMachine::*State)(Event);
  //typedef void (*State)(StateMachine *, Event);

  StateMachine() : state(0) {}
  StateMachine(State s) : state(s) {}
  //  inline void go(Event e) {(*state)(this, e);}
  inline void go(Event e) { (this->*state)(e); }
  inline void go(State s) {state = s;}

 protected:
  State state;
};

class UARTTransportReader : public StateMachine<RingBuffer<uint8_t> &> {
 public:
  class Delegate {
  public:
    virtual void event_packet(UARTTransportReader &r) {};
    virtual void acl_packet(UARTTransportReader &r) {};
    virtual void synchronous_packet(UARTTransportReader &r) {};
  };

  UARTTransportReader();
  void set_delegate(Delegate *delegate);
  size_t packet_size;
  uint8_t event_code, acl_bounary, acl_broadcast, synchronous_status;
  uint16_t handle;

 private:
  Delegate *delegate;

  // operating states
  void read_packet_indicator(RingBuffer<uint8_t> &b);
  void read_event_code_and_length(RingBuffer<uint8_t> &b);
  void read_event_parameters(RingBuffer<uint8_t> &b);

  // error states
  void bad_packet_indicator(RingBuffer<uint8_t> &b);
};

class Baseband :
  public BufferedUART::Delegate,
  public UARTTransportReader::Delegate
{
 public:
  BufferedUART &uart;
  IOPin &shutdown;
  UARTTransportReader reader;
  struct  {
      uint8_t hci_version, lmp_version;
      uint16_t hci_revision, manufacturer_name, lmp_subversion;
  } local_version_info;

  Baseband(BufferedUART &uart, IOPin &shutdown);

  void initialize();

  void send(HCI::Command const &cmd, ...);
  void receive(const char *format, ...);
  void data_received(BufferedUART *u);
  void error_occurred(BufferedUART *u);

  // UARTTransportReader::Delegate methods
  virtual void event_packet(UARTTransportReader &packet);

  // StateMachine states
  void module_requires_initialization();
};

class Pan1323Bootstrap :
  public StateMachine<uint16_t>, // opcode of successful HCI command
  public UARTTransportReader::Delegate
{
  Baseband &baseband;
  uint16_t opcode;
  uint8_t num_hci_packets, command_status;

 public:
  Pan1323Bootstrap(Baseband &b);
  void initialize();

  // UARTTransportReader::Delegate methods
  virtual void event_packet(UARTTransportReader &packet);

  // StateMachine states
  void reset_pending(uint16_t opcode);
  void read_version_info(uint16_t opcode);
  void baud_rate_negotiated(uint16_t opcode);
  void baud_rate_verified(uint16_t opcode);
  void something_bad_happened(uint16_t opcode);
};


