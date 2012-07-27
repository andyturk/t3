#pragma once

#include "assert.h"
#include "packet.h"
#include "h4.h"
/*
 * Derived from http://www.codeforge.com/article/39565
 */

namespace BTS {
  class Script {
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

    virtual void header(script_header &h) = 0;
    virtual void send(Packet &action) = 0;
    virtual void expect(uint32_t msec, Packet &action) = 0;
    virtual void configure(uint32_t baud, flow_control control) = 0;
    virtual void call(const char *filename) = 0;
    virtual void comment(const char *text) = 0;
    virtual void error(const char *reason) = 0;
    virtual void done() = 0;
  };

#ifndef __arm__
  class Recorder : public Script {
    SizedPacket<50000> script;

  public:
    Recorder() {}

    virtual void header(script_header &h);
    virtual void send(Packet &action);
    virtual void expect(uint32_t msec, Packet &action);
    virtual void configure(uint32_t baud, flow_control control);
    virtual void call(const char *filename);
    virtual void comment(const char *text);
    virtual void error(const char *reason);
    virtual void done();
  };
#endif

  class Player : public Script {
  protected:
    Packet script, command;
    uint16_t last_opcode;
    
  public:
    Player();
    virtual void reset(const uint8_t *bytes, uint16_t length);
    void play_next_action();
    virtual void header(script_header &h);
    bool is_complete() const {return script.get_remaining() == 0;}
  };

#ifndef __arm__
  class Filter : public Player {
  protected:
    Script *dst;

  public:
    Filter(Script *s) : dst(s) {}

    virtual void header(script_header &h) { dst->header(h); }
    virtual void send(Packet &action) { dst->send(action); }
    virtual void expect(uint32_t msec, Packet &action) { dst->expect(msec, action); }
    virtual void configure(uint32_t baud, flow_control control) { dst->configure(baud, control); }
    virtual void call(const char *filename) { dst->call(filename); }
    virtual void comment(const char *text) { dst->comment(text); }
    virtual void error(const char *reason) { dst->error(reason); }
    virtual void done() { dst->done(); }
    
    void copy(const uint8_t *bytes, uint16_t length, Script &s) {
      Filter f(&s);

      f.reset(bytes, length);
      while (!f.is_complete()) f.play_next_action();
    }
  };

  class ExpectationMinimizer : public Filter {
  protected:
    uint16_t hci_opcode;

  public:
    ExpectationMinimizer(Script *s);
    virtual void send(Packet &action);
    virtual void expect(uint32_t msec, Packet &action);
  };

  class SourceGenerator : public Player {
  protected:
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
  class H4Player : public Player, public H4Controller {
    H4Tranceiver &h4;
    uint16_t last_opcode;
    Packet *out;
    uint8_t status;

  public:
    H4Player(H4Tranceiver &h);

    virtual void command_succeeded(uint16_t opcode, Packet *p);

    // H4Controller methods
    virtual void sent(Packet *p);
    virtual void received(Packet *p);

    // Player methods
    virtual void reset(const uint8_t *bytes, uint16_t length);
    virtual void send(Packet &p);
  };
#endif
};


