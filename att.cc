#include <cstring>
#include <algorithm>

#include "utils/uartstdio.h"
#include "att.h"

uint16_t AttributeBase::group_end() {
  return handle;
}

int AttributeBase::compare(void *data, uint16_t len, uint16_t min_handle, uint16_t max_handle) {
  if (handle < min_handle) return -1;
  if (handle > max_handle) return 1;
  return compare(data, len);
}

ATT_Channel::ATT_Channel(HostController &hc) :
  Channel(L2CAP::ATTRIBUTE_CID, hc),
  att_mtu(23)
{
}

bool ATT_Channel::read_uuid(UUID &uuid, Packet *p, uint8_t opcode) {
  switch (p->get_remaining()) {
  case 2 : {
    uint16_t short_uuid;
    *p >> short_uuid;
    uuid = short_uuid;
    break;
  }

  case 16 :
    assert(sizeof(uuid.data) == 16);
    p->read(uuid.data, sizeof(uuid.data));
    break;

  default :
    UARTprintf("UUID must be either 2 or 16 bytes\n");
    p->l2cap();
    *p << (uint8_t) ATT::OPCODE_ERROR << opcode << (uint16_t) 0 << (uint8_t) ATT::INVALID_PDU;
    send(p);
    return false;
  }

  return true;
}

bool ATT_Channel::read_handles(uint16_t &from, uint16_t &to, Packet *p, uint8_t opcode) {
  *p >> from >> to;
  if (from > to || from == 0) {
    p->l2cap();
    *p << (uint8_t) ATT::OPCODE_ERROR << opcode << from << (uint8_t) ATT::INVALID_HANDLE;
    send(p);
    return false;
  }

  return true;
}

void ATT_Channel::attribute_not_found(Packet *p, uint16_t handle, uint8_t opcode) {
  p->l2cap() << (uint8_t) ATT::OPCODE_ERROR << opcode << handle << (uint8_t) ATT::ATTRIBUTE_NOT_FOUND;
}

bool ATT_Channel::is_grouping(const UUID &type) {
  return true;
}

void ATT_Channel::read_by_group_type(uint16_t from, uint16_t to, UUID &type, Packet *p) {
  if (!is_grouping(type)) {
    p->l2cap() << (uint8_t) ATT::OPCODE_ERROR << (uint8_t) ATT::OPCODE_READ_BY_GROUP_TYPE_REQUEST;
    *p << from << (uint8_t) ATT::UNSUPPORTED_GROUP_TYPE;
    send(p);
    return;
  }

  p->l2cap(att_mtu); // re-use request packet

  *p << (uint8_t) ATT::OPCODE_READ_BY_GROUP_TYPE_RESPONSE;
  uint8_t &attribute_data_length = *(uint8_t *) *p;
  *p << (uint8_t) 0; // placeholder

  uint16_t attr_length = 0;
  uint16_t data_length = 0;

  /*
   * AttributeBase::find_by_type returns 0 when a handle can't be found.
   * This is less than the lowest expected value of first_handle.
   */
  uint16_t h = AttributeBase::find_by_type(from, type);

  for (; p->get_remaining() > 2*sizeof(uint16_t) + 1;) {
    if (h < from || h > to) break;
    AttributeBase *attr = AttributeBase::get(h);
    if (attr_length == 0) { // first matching attribute
      attr_length = attr->length;
    } else if(attr_length != attr->length) { // stop if lengths differ
      break;
    }

    *p << attr->handle << attr->group_end();
    data_length = std::min(attr_length, p->get_remaining());
    p->write((const uint8_t *) attr->_data, data_length);
  }
  
  if (attr_length == 0) {
    attribute_not_found(p, from, ATT::OPCODE_READ_BY_GROUP_TYPE_REQUEST);
  } else {
    attribute_data_length = 2*sizeof(uint16_t) + data_length;
  }

  p->title = "read by group type response";
  send(p);
}

void ATT_Channel::find_by_type_value(uint16_t first_handle, uint16_t last_handle, UUID &type, Packet *req) {
  uint16_t length;
  uint8_t *value;

  value = (uint8_t *) *req;
  length = req->get_remaining(); // is this right?

  // can't re-use the request packet because we need the data at the end
  Packet *rsp = controller.acl_packets->allocate();
  assert(rsp != 0);
  rsp->l2cap(req, att_mtu); // re-use existing L2CAP framing from request

  *rsp << (uint8_t) ATT::OPCODE_FIND_BY_TYPE_VALUE_RESPONSE;
  uint16_t found_attribute_handle = 0, group_end_handle;
  uint16_t short_type = (uint16_t) type;

  /*
   * AttributeBase::find_by_type_value returns 0 when a handle can't be found.
   * This is less than the lowest expected value of first_handle.
   */
  uint16_t h = AttributeBase::find_by_type_value(first_handle, short_type, value, length);

  for (;;) {
    if (h < first_handle || h > last_handle || (rsp->get_remaining() < 2*sizeof(uint16_t))) break;

    found_attribute_handle = h;
    group_end_handle = AttributeBase::get(h)->group_end();
    uint16_t next_h = AttributeBase::find_by_type_value(h+1, short_type, value, length);

    if (found_attribute_handle == group_end_handle) { // not a grouping attribute
      if (next_h < first_handle || next_h > last_handle) group_end_handle = 0xffff;
    }

    *rsp << found_attribute_handle << group_end_handle;
    h = next_h;
  }

  if (found_attribute_handle == 0) {
    attribute_not_found(rsp, first_handle, ATT::OPCODE_FIND_BY_TYPE_VALUE_REQUEST);
  }

  send(rsp);
  req->deallocate();
}

void ATT_Channel::read_by_type(uint16_t starting_handle, uint16_t ending_handle, UUID &type, Packet *p) {
  p->l2cap(att_mtu); // re-use request packet

  *p << (uint8_t) ATT::OPCODE_READ_BY_TYPE_RESPONSE;
  uint8_t &attribute_data_length = *(uint8_t *) *p;
  *p << (uint8_t) 0; // placeholder

  uint16_t attr_length = 0; // actual attribute length
  uint16_t data_length = 0; // how much fits in this response

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
    data_length = std::min(attr_length, p->get_remaining());
    p->write((const uint8_t *) attr->_data, data_length);
  }
  
  if (attr_length == 0) {
    attribute_not_found(p, starting_handle, ATT::OPCODE_READ_BY_TYPE_REQUEST);
  } else {
    attribute_data_length = sizeof(uint16_t) + data_length;
  }

  send(p);
}

void ATT_Channel::receive(Packet *p) {
  uint8_t opcode;
  uint16_t starting, ending, short_type;
  UUID type;

  *p >> opcode;

  UARTprintf("ATT opcode 0x%02x\n", opcode);

  switch (opcode) {
  case ATT::OPCODE_ERROR : {
    uint16_t handle;
    uint8_t error;

    *p >> opcode >> handle >> error;
    UARTprintf("ATT error for opcode: 0x%02x, handle: 0x%04x, error: 0x%02x\n", opcode, handle, error);
    break;
  }

  case ATT::OPCODE_FIND_BY_TYPE_VALUE_REQUEST :
    if (!read_handles(starting, ending, p, opcode)) return;
    *p >> short_type;
    type = short_type;
    find_by_type_value(starting, ending, type, p);
    break;

  case ATT::OPCODE_READ_BY_TYPE_REQUEST :
    if (!read_handles(starting, ending, p, opcode)) return;
    if (!read_uuid(type, p, opcode)) return;
    read_by_type(starting, ending, type, p);
    break;

  case ATT::OPCODE_READ_BY_GROUP_TYPE_REQUEST :
    if (!read_handles(starting, ending, p, opcode)) return;
    if (!read_uuid(type, p, opcode)) return;
    read_by_group_type(starting, ending, type, p);
    break;

  default :
    UARTprintf("unrecognized att opcode: 0x%02x\n", opcode);
    break;
  }
}



