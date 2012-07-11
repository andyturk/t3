#include <cstring>
#include "utils/uartstdio.h"
#include "hci.h"

uint16_t AttributeBase::group_end_handle() {
  return handle;
}

int AttributeBase::compare(void *data, uint16_t len, uint16_t min_handle, uint16_t max_handle) {
  if (handle < min_handle) return -1;
  if (handle > max_handle) return 1;
  return compare(data, len);
}

void ATT_Channel::find_by_type_value(Packet *p) {
  uint16_t first_handle, last_handle, short_type, length;
  uint8_t *value;

  *p >> first_handle >> last_handle >> short_type;
  UUID type(short_type);

  value = (uint8_t *) *p;
  length = p->get_remaining(); // is this right?
  p->reset_l2cap(); // re-use L2CAP framing
  p->set_limit(23); // HACK!

  if (first_handle > last_handle || first_handle == 0) {
    *p << (uint8_t) ATT::OPCODE_ERROR << first_handle << (uint8_t) ATT::INVALID_HANDLE;
    send(p);
  } else {
    *p << (uint8_t) ATT::OPCODE_FIND_TYPE_BY_VALUE_RESPONSE;
    uint16_t found_attribute_handle = 0, group_end_handle;
    AttributeBase::Iterator i = attributes.begin();
    AttributeBase *matched, *next;

    while (i != attributes.end() &&
           ((i->handle < first_handle) || (i->handle > last_handle) ||
            type != i->type ||
            i->compare(value, length))) ++i;

    matched = i;

    while (matched != attributes.end() && p->get_remaining() >= 2*sizeof(uint16_t)) {
      found_attribute_handle = matched->handle;
      group_end_handle = matched->group_end_handle();

      while (i != attributes.end() &&
             ((i->handle < first_handle) || (i->handle > last_handle) ||
              type != i->type ||
              i->compare(value, length))) ++i;

      next = i;

      if (next == attributes.end()) group_end_handle = 0xffff;
      *p << found_attribute_handle << group_end_handle;
      matched = next;
    }

    if (found_attribute_handle == 0) {
      p->reset_l2cap();
      *p << (uint8_t) ATT::OPCODE_ERROR << first_handle << (uint8_t) ATT::ATTRIBUTE_NOT_FOUND;
    }
  }

  send(p);
}

void ATT_Channel::receive(Packet *p) {
  uint8_t opcode;

  *p >> opcode;
  switch (opcode) {
  case ATT::OPCODE_ERROR : {
    uint16_t handle;
    uint8_t error;

    *p >> opcode >> handle >> error;
    UARTprintf("ATT error for opcode: 0x%02x, handle: 0x%04x, error: 0x%02x\n", opcode, handle, error);
    break;
  }

  case ATT::OPCODE_FIND_TYPE_BY_VALUE_REQUEST :
    find_by_type_value(p);
    break;

  default :
    UARTprintf("unrecognized att opcode: 0x%02x\n", opcode);
    break;
  }
}



