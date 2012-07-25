#include "bts.h"

namespace BTS {
  void Script::header(script_header &h) {}
  void Script::send(Packet &action) {}
  void Script::expect(uint32_t msec, Packet &action) {}
  void Script::configure(uint32_t baud, flow_control control) {}
  void Script::call(const char *filename) {}
  void Script::comment(const char *text) {}
  void Script::error(const char *reason) {}

  void Recorder::header(script_header &h) {
    script.write((uint8_t *) &h, sizeof(h));
  }

  void Recorder::send(Packet &action) {
    uint16_t length = action.get_remaining();
    script << (uint16_t) SEND_COMMAND << length;
    script.write((uint8_t *) action, length);
  }

  void Recorder::expect(uint32_t msec, Packet &action) {
    uint16_t length = action.get_remaining();
    script << (uint16_t) WAIT_EVENT << msec << length;
    script.write((uint8_t *) action, length);
  }

  void Recorder::configure(uint32_t baud, flow_control control) {
    script << (uint16_t) SERIAL_PORT_PARAMETERS << baud << (uint32_t) control;
  }

  void Recorder::call(const char *filename) {
    // not implemented
  }

  void Recorder::comment(const char *text) {
    // not implemented
  }

  Player::Player(const uint8_t *bytes, uint16_t length) :
    script((uint8_t *) bytes, length)
  {
    script_header h;
    header(h);
  }

  void Player::play() {
    script_header h;
    header(h);

    while (script.get_remaining() > sizeof(command_header)) {
      uint16_t action, length;
      script >> action >> length;

      switch (action) {
      case SEND_COMMAND : {
        Packet command((uint8_t *) script, length);
        send(command);
        script.skip(length);
        break;
      }

      case WAIT_EVENT : {
        uint16_t msec;

        script >> msec;
        Packet event((uint8_t *) script, length);
        expect(msec, event);
        script.skip(length);
        break;
      }

      case SERIAL_PORT_PARAMETERS : {
        uint32_t baud, control;
        script >> baud >> control;
        configure(baud, (flow_control) control);
        script.skip(length);
        break;
      }

      case RUN_SCRIPT : {
        call((char *) (uint8_t *) script);
        script.skip(length);
        break;
      }

      case REMARKS : {
        comment((char *) (uint8_t *) script);
        script.skip(length);
        break;
      }

      default :
        error("bad action");
        return;
      }
    }
  }

  void Player::header(script_header &h) {
    assert(script.get_remaining() >= sizeof(h));
    script.read((uint8_t *) &h, sizeof(h));
    assert(h.magic == BTSB);
  }

  H4Player::H4Player(H4Tranceiver &h, const uint8_t *bytes, uint16_t length) :
    Player(bytes, length),
    h4(h)
  {
  }

};
