#pragma once

#include <cstddef>
#include <stdarg.h>

#include "hal.h"
#include "statemachine.h"
#include "scheduler.h"

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

class UARTTransportReader : public CSM {
 public:
  class Delegate {
  public:
    virtual void event_packet(UARTTransportReader &r) {};
    virtual void acl_packet(UARTTransportReader &r) {};
    virtual void synchronous_packet(UARTTransportReader &r) {};
  };

  UARTTransportReader(RingBuffer<uint8_t> &buf);
  void set_delegate(Delegate *delegate);

  uint8_t packet_type;
  size_t packet_size;

  uint8_t event_code;
  uint8_t acl_bounary, acl_broadcast, synchronous_status;
  uint16_t handle;

  RingBuffer<uint8_t> &input;
  void get_next_packet();

 private:
  Delegate *delegate;

  // operating states
  void read_packet_indicator();
  void read_event_code_and_length();
  void read_event_parameters();
  void packet_is_ready();

  // error states
  void bad_packet_indicator();
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
  void send(uint8_t *data, size_t len);
  void receive(const char *format, ...);
  void data_received(BufferedUART *u);
  void error_occurred(BufferedUART *u);

  // UARTTransportReader::Delegate methods
  virtual void event_packet(UARTTransportReader &packet);

  // StateMachine states
  void module_requires_initialization();
};

class Pan1323Bootstrap :
  public CSM,
  public UARTTransportReader::Delegate
{
  Baseband &baseband;
  uint8_t *patch_data;
  size_t patch_len, patch_offset;
  uint16_t opcode, expected_opcode;
  uint8_t num_hci_packets, command_status;

  void send_patch_command();

 public:
  Pan1323Bootstrap(Baseband &b);
  void initialize();

  // UARTTransportReader::Delegate methods
  virtual void event_packet(UARTTransportReader &packet);

  // StateMachine states
  void reset_pending();
  void read_version_info();
  void baud_rate_negotiated();
  void baud_rate_verified();
  void verify_patch_command();
  void something_bad_happened();
  void bootstrap_complete();
};


