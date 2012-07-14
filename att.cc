#include <cstring>
#include <algorithm>

#include "assert.h"
#include "att.h"

AttributeBase::AttributeBase(const UUID &t, void *d, uint16_t l) :
  type(t), handle(++next_handle), _data(d), length(l)
{
  all_handles[handle] = this;
}

AttributeBase::AttributeBase(int16_t t, void *d, uint16_t l) :
  type(t), handle(++next_handle), _data(d), length(l)
{
  all_handles[handle] = this;
}

uint16_t AttributeBase::find_by_type_value(uint16_t start, uint16_t type, void *value, uint16_t length) {
  assert(start > 0);
  for (uint16_t i=start; i < next_handle; ++i) {
    AttributeBase *attr = all_handles[i];

    if (attr->type != type) continue;
    if (length != attr->length) continue; // e.g., UUIDs could be either 2 or 16 bytes
    if (memcmp(attr->_data, value, length)) continue;

    return i;
  }

  return 0;
}

uint16_t AttributeBase::find_by_type(uint16_t start, const UUID &type) {
  assert(start > 0);
  for (uint16_t i=start; i < next_handle; ++i) {
    if (all_handles[i]->type == type) return i;
  }
  return 0;
}

uint16_t AttributeBase::group_end() {
  return handle;
}

int AttributeBase::compare(void *data, uint16_t len, uint16_t min_handle, uint16_t max_handle) {
  if (handle < min_handle) return -1;
  if (handle > max_handle) return 1;
  return compare(data, len);
}

#ifdef DEBUG
void AttributeBase::dump_attributes() {
  for (int i=1; i < next_handle; ++i) {
    AttributeBase *attr = all_handles[i];

    printf("%04x %s: (%d) ", attr->handle, attr->type.pretty_print(), attr->length);
    dump_hex_bytes((uint8_t *) attr->_data, attr->length);
    printf("\n");
  }
}
#endif

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
    printf("UUID must be either 2 or 16 bytes\n");
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

void ATT_Channel::error(uint8_t err, Packet *p, uint16_t handle, uint8_t opcode) {
  p->l2cap() << (uint8_t) ATT::OPCODE_ERROR << opcode << handle << err;  
}

bool ATT_Channel::is_grouping(const UUID &type) {
  return true;
}

void ATT_Channel::read_by_group_type(uint16_t from, uint16_t to, UUID &type, Packet *p) {
  if (!is_grouping(type)) {
    error(ATT::UNSUPPORTED_GROUP_TYPE, p, from, ATT::OPCODE_READ_BY_GROUP_TYPE_REQUEST);
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
    error(ATT::ATTRIBUTE_NOT_FOUND, p, from, ATT::OPCODE_READ_BY_GROUP_TYPE_REQUEST);
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
    error(ATT::ATTRIBUTE_NOT_FOUND, rsp, first_handle, ATT::OPCODE_FIND_BY_TYPE_VALUE_REQUEST);
  }

  rsp->title = "find by type value response";
  send(rsp);
  req->deallocate();
}

void ATT_Channel::read_by_type(uint16_t from, uint16_t to, UUID &type, Packet *p) {
  p->l2cap(att_mtu); // re-use request packet

  *p << (uint8_t) ATT::OPCODE_READ_BY_TYPE_RESPONSE;
  uint8_t &data_length = *(uint8_t *) *p;
  *p << (uint8_t) 0; // placeholder

  uint16_t attr_length = 0; // actual attribute length
  uint16_t h = from;

  do {
    h = AttributeBase::find_by_type(h, type);
    if (h < from || h > to) break;

    AttributeBase *attr = AttributeBase::get(h);
    assert(h == attr->handle);

    if (attr_length == 0) { // first matching attribute
      attr_length = attr->length;
      data_length = std::min(attr_length        + sizeof(uint16_t),
                             p->get_remaining() - sizeof(uint16_t));
    } else if(attr_length != attr->length) { // stop if lengths differ
      break;
    }

    *p << h;
    p->write((const uint8_t *) attr->_data, data_length - sizeof(uint16_t));
    h += 1;
  } while (p->get_remaining() >= data_length);
  
  if (attr_length == 0) {
    error(ATT::ATTRIBUTE_NOT_FOUND, p, from, ATT::OPCODE_READ_BY_TYPE_REQUEST);
  }

  p->title = "read by type response";
  send(p);
}

void ATT_Channel::read(uint16_t h, Packet *p) {
  AttributeBase *attr = AttributeBase::get(h);
  if (attr == 0) {
    error(ATT::INVALID_HANDLE, p, h, ATT::OPCODE_READ_REQUEST);
  } else {
    p->l2cap(att_mtu) << (uint8_t) ATT::OPCODE_READ_RESPONSE;
    p->write((uint8_t *) attr->_data, std::min(p->get_remaining(), attr->length));
  }

  send(p);
}

void ATT_Channel::read_blob(uint16_t h, uint16_t offset, Packet *p) {
  AttributeBase *attr = AttributeBase::get(h);
  if (attr == 0) {
    error(ATT::INVALID_HANDLE, p, h, ATT::OPCODE_READ_BLOB_REQUEST);
  } else {
    p->l2cap(att_mtu) << (uint8_t) ATT::OPCODE_READ_BLOB_RESPONSE;
    
    if (attr->length < att_mtu) {
      error(ATT::ATTRIBUTE_NOT_LONG, p, h, ATT::OPCODE_READ_BLOB_REQUEST);
    } else if (offset >= attr->length) {
      error(ATT::INVALID_OFFSET, p, h, ATT::OPCODE_READ_BLOB_REQUEST);
    } else {
      p->write(((uint8_t *) attr->_data) + offset, std::min(p->get_remaining(), attr->length));
    }
  }

  send(p);
}

void ATT_Channel::receive(Packet *p) {
  uint8_t opcode;
  uint16_t starting, ending, short_type;
  UUID type;

  *p >> opcode;

  switch (opcode) {
  case ATT::OPCODE_ERROR : {
    uint16_t handle;
    uint8_t error;

    *p >> opcode >> handle >> error;
    printf("ATT error for opcode: 0x%02x, handle: 0x%04x, error: 0x%02x\n",
           opcode, handle, error);
    break;
  }

  case ATT::OPCODE_FIND_BY_TYPE_VALUE_REQUEST :
    if (!read_handles(starting, ending, p, opcode)) return;
    *p >> short_type;
    type = short_type;
    printf("ATT: find %04x-%04x by type: %s, value: (%d bytes)\n",
           starting, ending, type.pretty_print(), p->get_remaining());
    find_by_type_value(starting, ending, type, p);
    break;

  case ATT::OPCODE_READ_BY_TYPE_REQUEST :
    if (!read_handles(starting, ending, p, opcode)) return;
    if (!read_uuid(type, p, opcode)) return;
    printf("ATT: read %04x-%04x by type: %s\n", starting, ending, type.pretty_print());
    read_by_type(starting, ending, type, p);
    break;

  case ATT::OPCODE_READ_BY_GROUP_TYPE_REQUEST :
    if (!read_handles(starting, ending, p, opcode)) return;
    if (!read_uuid(type, p, opcode)) return;
    printf("ATT: read %04x-%04x by group type: %s\n", starting, ending, type.pretty_print());
    read_by_group_type(starting, ending, type, p);
    break;

  case ATT::OPCODE_READ_REQUEST :
    *p >> starting;
    printf("ATT: read handle 0x%04x\n", starting);
    read(starting, p);
    break;
    
  default :
    printf("unrecognized att opcode: 0x%02x\n", opcode);
    break;
  }
}


