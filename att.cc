#include <cstring>
#include <algorithm>

#include "utils/uartstdio.h"
#include "hci.h"

uint16_t AttributeBase::group_end() {
  return handle;
}

int AttributeBase::compare(void *data, uint16_t len, uint16_t min_handle, uint16_t max_handle) {
  if (handle < min_handle) return -1;
  if (handle > max_handle) return 1;
  return compare(data, len);
}

ATT_Channel::ATT_Channel() :
  Channel(L2CAP::ATTRIBUTE_CID),
  att_mtu(23)
{
}

void ATT_Channel::find_by_type_value(Packet *p) {
  uint16_t first_handle, last_handle, short_type, length;
  uint8_t *value;

  *p >> first_handle >> last_handle >> short_type;
  UUID type(short_type);

  value = (uint8_t *) *p;
  length = p->get_remaining(); // is this right?
  p->l2cap(att_mtu); // re-use existing L2CAP framing

  if (first_handle > last_handle || first_handle == 0) {
    *p << (uint8_t) ATT::OPCODE_ERROR << first_handle << (uint8_t) ATT::INVALID_HANDLE;
    send(p);
  } else {
    *p << (uint8_t) ATT::OPCODE_FIND_TYPE_BY_VALUE_RESPONSE;
    uint16_t found_attribute_handle = 0, group_end_handle;

    /*
     * AttributeBase::find_by_type_value returns 0 when a handle can't be found.
     * This is less than the lowest expected value of first_handle.
     */
    uint16_t h = AttributeBase::find_by_type_value(first_handle, short_type, value, length);

    for (;;) {
      if (h < first_handle || h > last_handle || (p->get_remaining() < 2*sizeof(uint16_t))) break;

      found_attribute_handle = h;
      group_end_handle = AttributeBase::get(h)->group_end();
      uint16_t next_h = AttributeBase::find_by_type_value(h+1, short_type, value, length);

      if (found_attribute_handle == group_end_handle) { // not a grouping attribute
        if (next_h < first_handle || next_h > last_handle) group_end_handle = 0xffff;
      }

      *p << found_attribute_handle << group_end_handle;
      h = next_h;
    }

    if (found_attribute_handle == 0) {
      p->l2cap() << (uint8_t) ATT::OPCODE_ERROR << (uint8_t) ATT::OPCODE_FIND_TYPE_BY_VALUE_REQUEST;
      *p << first_handle << (uint8_t) ATT::ATTRIBUTE_NOT_FOUND;
    }
  }

  send(p);
}

void ATT_Channel::read_by_type(Packet *p) {
  uint16_t starting_handle, ending_handle;
  UUID type;

  *p >> starting_handle >> ending_handle;

  if (starting_handle > ending_handle || starting_handle == 0x0000) {
    p->l2cap() << (uint8_t) ATT::OPCODE_ERROR << (uint8_t) ATT::OPCODE_READ_BY_TYPE_REQUEST;
    *p << starting_handle << (uint8_t) ATT::INVALID_HANDLE;
    send(p);
    return;
  }

  switch (p->get_remaining()) {
  case 2 : {
    uint16_t short_type;
    *p >> short_type;
    type = short_type;
    break;
  }
    
  case 16 :
    type = (uint8_t *) *p;
    break;
    
  default :
    UARTprintf("unexpected packet size in ATT_Channel::read_by_type\n");
    UARTprintf("should deallocate packet here\n");
    return;
  }

  p->l2cap(att_mtu);

  *p << (uint8_t) ATT::OPCODE_READ_BY_TYPE_RESPONSE;
  uint8_t &attribute_handle_pair_length = *(uint8_t *) *p;
  *p << (uint8_t) 0; // placeholder

  unsigned int attr_length = 0;
  unsigned int attr_length_as_written = 0;

  /*
   * AttributeBase::find_by_type returns 0 when a handle can't be found.
   * This is less than the lowest expected value of first_handle.
   */
  uint16_t h = AttributeBase::find_by_type(starting_handle, type);

  for (; p->get_remaining() > sizeof(uint16_t) + 1;) {
    if (h < starting_handle || h > ending_handle) break;
    AttributeBase *attr = AttributeBase::get(h);
    if (attr_length == 0) { // first matching attribute
      attr_length = attr->length;
    } else if(attr_length != attr->length) { // stop if lengths differ
      break;
    }

    *p << attr->handle;
    attr_length_as_written = std::min(attr_length, p->get_remaining());
    p->write((const uint8_t *) attr->_data, attr_length_as_written);
  }
  
  if (attr_length == 0) { // didn't find any attributes
    p->l2cap() << (uint8_t) ATT::OPCODE_ERROR << (uint8_t) ATT::OPCODE_READ_BY_TYPE_REQUEST;
    *p << starting_handle << (uint8_t) ATT::ATTRIBUTE_NOT_FOUND;
  } else {
    attribute_handle_pair_length = sizeof(uint16_t) + attr_length_as_written;
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

  case ATT::OPCODE_READ_BY_TYPE_REQUEST :
    read_by_type(p);
    break;

  default :
    UARTprintf("unrecognized att opcode: 0x%02x\n", opcode);
    break;
  }
}



