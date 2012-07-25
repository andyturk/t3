#include "bts.h"

namespace BTS {
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

  void Recorder::done() {
    // nothing to do
  }

  Player::Player(const uint8_t *bytes, uint16_t length) :
    script((uint8_t *) bytes, length)
  {
    script_header h;
    header(h);
  }

  void Player::header(script_header &h) {
    assert(script.get_remaining() >= sizeof(h));
    script.read((uint8_t *) &h, sizeof(h));
    if (h.magic != BTSB) error("bad magic");
  }

  void Player::play_next_action() {
    if (script.get_remaining() == 0) {
      done();
    } else if(script.get_remaining() < sizeof(command_header)) {
      error("bad script");
    } else {
      uint16_t action, length;
      script >> action >> length;

      switch (action) {
      case SEND_COMMAND : {
        Packet command((uint8_t *) script, length);
        script.skip(length);
        send(command);
        break;
      }

      case WAIT_EVENT : {
        uint16_t msec;

        script >> msec;
        Packet event((uint8_t *) script, length);
        script.skip(length);
        expect(msec, event);
        break;
      }

      case SERIAL_PORT_PARAMETERS : {
        uint32_t baud, control;
        script >> baud >> control;
        script.skip(length);
        configure(baud, (flow_control) control);
        break;
      }

      case RUN_SCRIPT : {
        Packet filename((uint8_t *) script, length);
        script.skip(length);
        call((char *) (uint8_t *) filename);
        break;
      }

      case REMARKS : {
        Packet text((uint8_t *) script, length);
        script.skip(length);
        comment((char *) (uint8_t *) text);
        break;
      }

      default :
        error("bad action");
        return;
      }
    }
  }

  H4Player::H4Player(H4Tranceiver &h, const uint8_t *bytes, uint16_t length) :
    Player(bytes, length),
    h4(h)
  {
  }

};
