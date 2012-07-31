#include "script.h"
#include "assert.h"
#include "debug.h"

Packet *Script::allocate_packet() {
  return h4.command_packets.allocate();
}

void Script::send(Packet *p) {
  p->join(&h4.packets_to_send);
  h4.fill_uart();
}

CannedScript::CannedScript(H4Tranceiver &t, const uint8_t *bytes, uint16_t length) :
  Script(t),
  bytes((uint8_t *) bytes, length),
  last_opcode(0),
  state(&next_canned_command)
{
}

bool CannedScript::is_complete() const {
  return bytes.get_remaining() == 0 && last_opcode == 0;
}

bool CannedScript::is_pending() const {
  return last_opcode != 0;
}

bool CannedScript::command_complete(uint16_t opcode, Packet *p) {
  if (opcode == last_opcode) {
    uint8_t status;

    *p >> status;
    debug("opcode 0x%04x status=0x%02x\n", opcode, status);
    last_opcode = 0;
    p->deallocate();
    next();
    return true;
  } else {
    return false;
  }
}

bool CannedScript::command_status(uint16_t opcode, Packet *p) {
  if (opcode == last_opcode) {
    p->deallocate();
    return true;
  } else {
    return false;
  }
}

void CannedScript::restart() {
  bytes.rewind();
  last_opcode = 0;
}

void CannedScript::send(Packet *p) {
  debug("send 0x%04x\n", last_opcode);
  Script::send(p);
}

Packet *CannedScript::next_canned_command() {
  if (bytes.get_remaining() > 0) {
    assert(bytes.get_remaining() >= 4);
    assert(last_opcode == 0);

    Packet *p = allocate_packet();
    assert(p != 0);

    uint8_t indicator, parameter_length, *start;
    
    start = (uint8_t *) bytes;
    bytes >> indicator >> last_opcode >> parameter_length;

    assert(bytes.get_remaining() >= parameter_length);

    p->reset();
    p->write(start, 4 + parameter_length);
    p->flip();

    send(p);

    if (last_opcode == OPCODE_PAN13XX_CHANGE_BAUD_RATE) {
      assert(parameter_length == sizeof(baud_rate));
      bytes >> baud_rate;
    } else {
      bytes.skip(parameter_length);
    }

    return p;
  } else {
    return 0;
  }
}
