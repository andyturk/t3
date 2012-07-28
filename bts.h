#pragma once

#include "assert.h"
#include "packet.h"
#include "h4.h"
/*
 * Derived from http://www.codeforge.com/article/39565
 */

namespace BTS {
  class Script {
  protected:
    Packet script, command;
    uint16_t last_opcode;

  public:
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

    typedef struct {
      uint32_t baud;
      uint32_t control;
    } configuration;

    virtual void reset(const uint8_t *bytes, uint16_t length);
    virtual void play_next_action();
    bool is_complete() const {return script.get_remaining() == 0;}

    virtual void header(script_header &h);
    virtual void send(Packet &action);
    virtual void expect(uint32_t msec, Packet &action);
    virtual void configure(uint32_t baud, flow_control control);
    virtual void call(const char *filename);
    virtual void comment(const char *text);
    virtual void error(const char *reason);
    virtual void done();
  };

#ifndef __arm__
  class SourceGenerator : public Script {
  protected:
    const char *name;
    void as_hex(const uint8_t *bytes, uint16_t size, const char *start = 0);

  public:
    SourceGenerator(const char *n);
    ~SourceGenerator();

    void emit(const uint8_t *bytes, uint16_t size);
    virtual void send(Packet &action);
    virtual void expect(uint32_t msec, Packet &action);
    virtual void configure(uint32_t baud, flow_control control);
    virtual void call(const char *filename);
    virtual void comment(const char *text);
    virtual void error(const char *msg);
    virtual void done() { cout << "done!\n"; }
    
  };
#endif

#ifdef __arm__
  class H4Player : public Script, public H4Controller {
    H4Tranceiver &h4;
    uint16_t last_opcode;
    Packet *out;
    uint8_t status;

  public:
    H4Player(H4Tranceiver &h);

    void go();
    virtual void command_succeeded(uint16_t opcode, Packet *p);

    // H4Controller methods
    virtual void sent(Packet *p);
    virtual void received(Packet *p);

    // Player methods
    virtual void reset(const uint8_t *bytes, uint16_t length);
    virtual void play_next_action();
    virtual void send(Packet &p);
    virtual void expect(uint32_t msec, Packet &p) {}
    virtual void configure(uint32_t baud, flow_control control);
    virtual void call(const char *filename) {}
    virtual void comment(const char *text) {}
    virtual void done();
    virtual void error(const char *text) {}

  };
#endif
};


