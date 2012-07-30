#include "sequence.h"
#include "assert.h"
#include "debug.h"

CannedSequence::CannedSequence(const uint8_t *bytes, uint16_t length) :
  bytes((uint8_t *) bytes, length),
  last_opcode(0)
{
}

bool CannedSequence::is_complete() const {
  return bytes.get_remaining() == 0 && last_opcode == 0;
}

bool CannedSequence::command_complete(uint16_t opcode, Packet *p) {
  if (opcode == last_opcode) {
    debug("OK 0x%04x\n", opcode);
    last_opcode = 0;
    p->deallocate();
    next();
    return true;
  } else {
    return false;
  }
}

bool CannedSequence::command_status(uint16_t opcode, Packet *p) {
  if (opcode == last_opcode) {
    p->deallocate();
    next();
    return true;
  } else {
    return false;
  }
}

void CannedSequence::restart() {
  bytes.rewind();
  last_opcode = 0;
}

void CannedSequence::next() {
  if (bytes.get_remaining() > 0) {
    assert(bytes.get_remaining() >= 4);
    assert(last_opcode == 0);

    uint8_t indicator, parameter_length, *start;
    
    start = (uint8_t *) bytes;
    bytes >> indicator >> last_opcode >> parameter_length;

    assert(bytes.get_remaining() >= parameter_length);
    command.initialize(start, 4 + parameter_length);

    if (last_opcode == OPCODE_PAN13XX_CHANGE_BAUD_RATE) {
      assert(parameter_length == sizeof(baud_rate));
      bytes >> baud_rate;
    } else {
      bytes.skip(parameter_length);
    }
  }
}
