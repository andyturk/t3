#pragma once

#include <cstring>

#include "assert.h"
#include "bluetooth_constants.h"
#include "bd_addr.h"
#include "pool.h"
#include "debug.h"

using namespace HCI;

class FlipBuffer {
 protected:
  uint8_t *storage;
  uint16_t capacity, position, limit;

 public:
  FlipBuffer() : storage(0), capacity(0), position(0), limit(0) {}
  FlipBuffer(uint8_t *s, uint16_t c) : storage(s), capacity(c), position(0), limit(c) {}
  uint16_t get_capacity() const {return capacity;}
  uint16_t get_position() const {return position;}
  uint16_t get_limit() const {return limit;}
  uint16_t get_remaining() const {return limit - position;}
  uint8_t *ptr() {return storage + position;}

  void initialize(uint8_t *s, uint16_t c) {
    storage = s;
    limit = capacity = c;
    position = 0;
  }

  void set_limit(uint16_t l) {limit = l;}
  void reset(uint16_t lim=0) {position = 0; limit = (lim == 0) ? capacity : lim;}
  void flip() {limit = position; position = 0;}
  void unflip() {position = limit; limit = capacity;}
  void rewind() {position = 0;}
  void seek(uint16_t p) {position = p;}
  void skip(int offset) {position += offset;}
  
  uint8_t get() {return storage[position++];}
  uint8_t get(uint16_t pos) {return storage[pos];}
  uint8_t peek(int offset) {return storage[position + offset];}
  void put(const uint8_t x) {assert(position < limit); storage[position++] = x;}
};

class Packet : public Ring<Packet>, public FlipBuffer {
 public:
  const char *title;
  Ring<Packet> *free_packets;

 Packet() :
  title(0),
  free_packets(0)
 {}

  Packet(uint8_t *buf, uint16_t len) :
    FlipBuffer(buf, len),
    title(0),
    free_packets(0)
  {}
  void deallocate() {assert(free_packets != 0); join(free_packets);}

  Packet &read(uint8_t *p, uint16_t len) {
    assert(position + len <= limit);
    memcpy(p, storage + position, len);
    position += len;
    return *this;
  }

  Packet &write(const uint8_t *p, uint16_t len) {
    assert(position + len <= limit);
    memcpy(storage + position, p, len);
    position += len;
    return *this;
  }

  Packet &operator<<(uint8_t x) {
    put(x);
    return *this;
  }

  Packet &operator<<(uint16_t x) {
    put(x & 0x00ff);
    put(x >> 8);
    return *this;
  }

  Packet &operator<<(uint32_t x) {
    put((x >>  0) & 0x000000ff);
    put((x >>  8) & 0x000000ff);
    put((x >> 16) & 0x000000ff);
    put((x >> 24) & 0x000000ff);
    return *this;
  }

  Packet &operator>>(uint8_t &x) {
    x = get();
    return *this;
  }

  Packet &operator>>(uint16_t &x) {
    x = get() + (get() << 8);
    return *this;
  }

  Packet &operator>>(uint32_t &x) {
    x = get() + (get() << 8) + (get() << 16) + (get() << 24);
    return *this;
  }

  operator uint8_t *() {return storage + position;}

  void prepare_for_tx() {

    if (position != 0) flip();

    switch (storage[0]) {
    case COMMAND_PACKET :
      seek(3);
      *this << (uint8_t) (limit - 4); // command length
      break;

    case ACL_PACKET :
      seek(3);
      *this << (uint16_t) (limit - 5); // acl length
      *this << (uint16_t) (limit - 9); // l2cap length
      break;
    }

    seek(0);

    if (title) {
#ifdef DEBUG
      printf("%s:\n", title);
      dump();
#endif
      title = 0;
    }
  }

  enum {
    HCI_HEADER_SIZE = 1,
    ACL_HEADER_SIZE = HCI_HEADER_SIZE + 4,
    L2CAP_HEADER_SIZE = ACL_HEADER_SIZE + 4
  };

  Packet &uart(enum packet_indicator ind) {reset(); return *this << (uint8_t) ind;}
  Packet &hci(HCI::opcode c) {return uart(COMMAND_PACKET) << (uint16_t) c << (uint8_t) 0;}
  Packet &hci(HCI::event e) {return uart(EVENT_PACKET) << (uint8_t) e;}
  Packet &acl(uint16_t handle, uint8_t pb, uint8_t bc) {
    return uart(ACL_PACKET) << (uint16_t) ((bc << 14) + (pb << 12) + handle) << (uint16_t) 0;
  }
  Packet &l2cap(uint16_t handle, uint16_t cid) { return acl(handle, 0x02, 0x00) << (uint16_t) 0 << cid;}

  /*
   * The following two methods re-use existing L2CAP framing information.
   * The packet is reset, but the L2CAP framing is either preserved
   * in place or copied from another packet. This includes the
   * source/destination handle and the channel ID. The position is left
   * at the first L2CAP payload byte. Length fields in the framing are not
   * modified here and must be set (via prepare_for_tx) before the packet
   * can be sent.
   */
  Packet &l2cap(uint16_t lim=0) {reset(lim); seek(L2CAP_HEADER_SIZE); return *this;}
  Packet &l2cap(Packet *other, uint16_t lim=0) {reset(lim); write(other->storage, L2CAP_HEADER_SIZE); return *this;}

  void dump() {
    void dump_hex_bytes(uint8_t *, size_t);

    dump_hex_bytes((uint8_t *) *this, get_remaining());
    printf("\n");
  }
};

template<unsigned int size>
class SizedPacket : public Packet {
  uint8_t _storage[size];
 public:
  SizedPacket() : Packet(_storage, size) {}
};

template<unsigned int packet_size, unsigned int packet_count>
class PacketPool : public Pool<SizedPacket<packet_size>, packet_count> {
 public:
  PacketPool() : Pool<SizedPacket<packet_size>, packet_count>() {
    for (unsigned int i=0; i < packet_count; ++i) {
      this->pool[i].free_packets = (Ring<Packet> *) &this->available;
    }
  }
};
