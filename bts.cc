#include "bts.h"

#ifndef __arm__
#include <cstdio>
#include <iostream>
using namespace std;
#endif

namespace BTS {
#ifndef __arm__
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

  void Recorder::error(const char *reason) {
  }
#endif

  void Player::reset(const uint8_t *bytes, uint16_t length) {
    script.initialize((uint8_t *) bytes, length);
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

#ifdef __arm__
  H4Player::H4Player(H4Tranceiver &h) :
    h4(h)
  {
  }

  void H4Player::reset(const uint8_t *bytes, uint16_t length) {
    Player::reset(bytes, length);
    h4.reset();
    status = HCI::SUCCESS;
    out = h4.command_packets.allocate();
    play_next_action();
  }

  void H4Player::sent(Packet *p) {
  }

  void H4Player::command_succeeded(uint16_t opcode, Packet *p) {
  }

  void H4Player::received(Packet *p) {
    uint8_t indicator, event, command_packet_budget;
    uint16_t opcode;

    *p >> indicator;
    if (indicator == HCI::EVENT_PACKET) {
      *p >> event;

      if (event == HCI::EVENT_COMMAND_COMPLETE) {
        *p >> command_packet_budget >> opcode >> status;

        if (last_opcode == 0 || last_opcode == opcode) {
          if (status == HCI::SUCCESS) {
            last_opcode = 0;
            command_succeeded(opcode, p);

            if (!is_complete()) play_next_action();
            return;
          }
        }
      }
    }

    status = HCI::COMMAND_DISALLOWED;
    script.seek(script.get_limit());
  }

  void H4Player::send(Packet &action) {
    out->reset();
    out->write((uint8_t *) action, action.get_remaining());
    out->flip();
    out->join(&h4.packets_to_send);

    uint8_t indicator;

    action >> indicator >> last_opcode;
    assert(indicator == HCI::COMMAND_PACKET);

    action.reset();
  }
#endif
};

#ifndef __arm__
int main(int argc, char *argv[]) {
}
#endif
