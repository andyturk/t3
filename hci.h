#pragma once

#include <stdint.h>
#include <stdarg.h>

#include "hal.h"
#include "buffer.h"

#define OPCODE(ogf,ocf) (((ogf) << 10) | (ocf))

namespace HCI {
  enum hci_status {
    SUCCESS = 0x00,
    UNKNOWN_HCI_COMMAND = 0x01,
    UNKNOWN_CONNECTION_IDENTIFIER = 0x02,
    HARDWARE_FAILURE = 0x03,
    PAGE_TIMEOUT = 0x04,
    AUTHENTICATION_FAILURE = 0x05,
    PIN_OR_KEY_MISSING = 0x06,
    MEMORY_CAPACITY_EXCEEDED = 0x07,
    CONNECTION_TIMEOUT = 0x08,
    CONNECTION_LIMIT_EXCEEDED = 0x09,
    SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED = 0x0a,
    ACL_CONNECTION_ALREADY_EXISTS = 0x0b,
    COMMAND_DISALLOWED = 0x0c,
    CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES = 0x0d,
    CONNECTION_REJECTED_DUE_TO_SECURITY_REASONS = 0x0e,
    CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR = 0x0f,
    CONNECTION_ACCEPT_TIMEOUT_EXCEEDED = 0x10,
    UNSUPPORTED_FEATURE_OR_PARAMETER_VALUE = 0x11,
    INVALID_HCI_COMMAND_PARAMETERS = 0x12,
    REMOTE_USER_TERMINATED_CONNECTION = 0x13,
    REMOTE_DEVICE_TERMINATED_CONNECTION_DUE_TO_LOW_RESOURCES = 0x14,
    REMOTE_DEVICE_TERMINATED_CONNECTION_DUE_TO_POWER_OFF = 0x15,
    CONNECTION_TERMINATED_BY_LOCAL_HOST = 0x16,
    REPEATED_ATTEMPTS = 0x17,
    PAIRING_NOT_ALLOWED = 0x18,
    UNKNOWN_LMP_PDU = 0x19,
    UNSUPPORTED_REMOTE_LMP_FEATURE = 0x1a,
    SCO_OFFSET_REJECTED = 0x1b,
    SCO_INTERVAL_REJECTED = 0x1c,
    SCO_AIR_MODE_REJECTED = 0x1d,
    INVALID_LMP_PARAMETERS = 0x1e,
    UNSPECIFIED_ERROR = 0x1f,
    UNSUPPORTED_LMP_PARAMETER_VALUE = 0x20,
    ROLE_CHANGE_NOT_ALLOWED = 0x21,
    LMP_LL_RESPONSE_TIMEOUT = 0x22,
    LMP_ERROR_TRANSACTION_COLLISION = 0x23,
    LMP_PDU_NOT_ALLOWED = 0x24,
    ENCRYPTION_MODE_NOT_ACCEPTABLE = 0x25,
    LINK_KEY_CANNOT_BE_CHANGED = 0x26,
    REQUESTED_QOS_NOT_SUPPORTED = 0x27,
    INSTANT_PASSED = 0x28,
    PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED = 0x29,
    DIFFERENT_TRANSACTION_COLLISION = 0x2a,
    QOS_UNACCEPTABLE_PARAMETER = 0x2c,
    QOS_REJECTED = 0x2d,
    CHANNEL_ASSESSMENT_NOT_SUPPORTED = 0x2e,
    INSUFFICIENT_SECURITY = 0x2f,
    PARAMETER_OUT_OF_MANDATORY_RANGE = 0x30,
    ROLE_SWITCH_PENDING = 0x32,
    RESERVED_SLOT_VIOLATION = 0x34,
    ROLE_SWITCH_FAILED = 0x35,
    EXTENDED_INQUIRY_RESPONSE_TOO_LARGE = 0x36,
    SIMPLE_PAIRING_NOT_SUPPORTED_BY_HOST = 0x37,
    HOST_BUSY_PAIRING = 0x38,
    CONNECTION_REJECTED_DUE_TO_NO_SUITABLE_CHANNEL_FOUND = 0x39,
    CONTROLLER_BUSY = 0x3a,
    UNACCEPTABLE_CONNECTION_INTERVAL = 0x3b,
    DIRECTED_ADVERTISING_TIMEOUT = 0x3c,
    CONNECTION_TERMINATED_DUE_TO_MIC_FAILURE = 0x3e,
    MAC_CONNECTION_FAILED = 0x3f
  };

  enum opcode_group {
    LC = 0x01, // Link Control
    LP = 0x02, // Link Policy
    BB = 0x03, // Controller & Baseband
    IF = 0x04, // Informational
    ST = 0x05, // Status
    TS = 0x06, // Testing
    VS = 0x3f  // Vendor Specific
  };

  enum hci_version {
    SPECIFICATION_1_1B,
    SPECIFICATION_1_1,
    SPECIFICATION_1_2,
    SPECIFICATION_2_0,
    SPECIFICATION_2_1_EDR,
    SPECIFICATION_3_0_HS,
    SPECIFICATION_4_0
  };

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

  class Packet : public FlipBuffer<uint8_t> {
    enum {MAX_PACKET = 259};
    uint8_t buf[MAX_PACKET];

  public:
    Packet *next;

    Packet() : FlipBuffer<uint8_t>(buf, MAX_PACKET), next(0) {}
    void fput(HCI::Command const &cmd, ...);
    void fget(const char *format, ...);
  };

};

class BBand {
  enum {PACKET_POOL_SIZE = 4};
  typedef void (*uart_state)(BBand *);
  typedef void (*packet_state)(BBand *, HCI::Packet *);

  UART &uart;
  IOPin &shutdown;
  HCI::Packet *rx;
  uart_state rx_state;
  HCI::Packet *tx;
  HCI::Packet packet_pool[PACKET_POOL_SIZE];
  HCI::Packet *free_packets;
  HCI::Packet *incoming_packets;
  packet_state packet_handler;
  uint8_t command_packet_budget;
  HCI::BD_ADDR bd_addr;

  void fill_uart();
  void drain_uart();

  HCI::Packet *allocate_packet();
  void deallocate_packet(HCI::Packet *packet);
  void send(HCI::Packet *p);

  // uart states
  void rx_expect_packet_indicator();
  void rx_expect_event_code_and_length();
  void rx_expect_event_parameters();

  // initialization states
  void reset_pending(HCI::Packet *p);
  void read_version_info(HCI::Packet *p);
  void baud_rate_negotiated(HCI::Packet *p);
  void read_bd_addr(HCI::Packet *p);
  void send_patch_command(HCI::Packet *p);

  struct {
    uint16_t expected_opcode;
    size_t offset;
    size_t length;
    uint8_t *data;
  } patch_state;

 public:
  BBand(UART &u, IOPin &s);
  void initialize();
  void uart_interrupt_handler();
  void process_incoming_packets();
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

namespace HCI {
#include "command_defs.h"
};

#undef BEGIN_COMMANDS
#undef COMMAND
#undef END_COMMANDS
#undef BEGIN_EVENTS
#undef EVENT
#undef END_EVENTS
#undef BEGIN_LE_EVENTS
#undef LE_EVENT
#undef END_LE_EVENTS
