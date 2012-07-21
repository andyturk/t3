#pragma once

#include "packet.h"

/*
 * Derived from http://www.codeforge.com/article/39565
 */

class BluetoothScript {
 protected:
  Packet script;

  enum magic {
    BTSB = 0x42535442 // 'BTSB'
  };

  enum action {
    SEND_COMMAND = 1,
    WAIT_EVENT = 2,
    SERIAL_PORT_PARAMETERS = 3,
    RUN_SCRIPT = 5,
    REMARKS = 6
  };

  enum flow_control {
    NONE = 0,
    HARDWARE = 1,
    NO_CHANGE = 0xffffffff
  };

  typedef union {
    uint32_t magic;
    uint8_t bytes[32];
  } script_header;

  typedef struct {
    uint16_t action;
    uint16_t size;
  } command_header;

  void (*state)(BluetoothScript *);

  // states
  void error() {}
  void done() {}

 public:
  virtual void send_command(Packet &action);
  virtual void wait_event(Packet &action);
  virtual void serial_port_parameters(Packet &action);
  virtual void run_script(Packet &action);
  virtual void remarks(Packet &action);

  BluetoothScript(const uint8_t *bytes, uint16_t length) :
    script((uint8_t *) bytes, length)
  {
  }
};

class BluetoothScriptSender : public BluetoothScript {
  UART &uart;

 public:
   BluetoothScriptSender(UART &u, const uint8_t *bytes, uint16_t length) :
     BluetoothScript(bytes, length),
     uart(u)
   {
   }

  void start() {
    script_header h;

    if (script.get_remaining() < sizeof(h)) {
      state = &error;
      return;
    }

    script.read((uint8_t *) &h, sizeof(h));

    state = (h.magic == BTSB) ? &sending : &error;
  }

  void sending() {
    command_header h;

    if (script.get_remaining() == 0) {
      state = &done;
      return;
    }

    if (script.get_remaining() < sizeof(h)) {
      state = &error;
      return;
    }

    script.read((uint8_t *) &h, sizeof(h));

    // the command buffer is just a piece of the overall script
    Packet command((uint8_t *) script, h.size);
    script.skip(h.size);
    
    switch (h.action) {
    case SEND_COMMAND :
      send_command(command);
      break;

    case WAIT_EVENT :
      wait_event(command);
      break;

    case SERIAL_PORT_PARAMETERS :
      serial_port_parameters(command);
      break;

    case RUN_SCRIPT :
      run_script(command);
      break;

    case REMARKS :
      remarks(command);
      break;

    default :
      state = &error;
      break;
    }
  }

   virtual void send_command(Packet &action) {
     //
   }
};


