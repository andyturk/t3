#include "hci.h"
#include "l2cap.h"
#include "utils/uartstdio.h"

extern BBand pan1323;

  Ring<Channel> Channel::channels;

  Channel::Channel(uint16_t c) : channel_id(c) {
    join(&channels);
    assert(find(c) == this);
  }

  Channel::~Channel() {
    join(this);
  }

  Channel *Channel::find(uint16_t id) {
    for (Channel::Iterator i = channels.begin(); i != channels.end(); ++i) {
      if (i->channel_id == id) return i;
    }

    return 0;
  }

  void Channel::receive(Packet *p) {
    UARTprintf("received data for channel 0x%04x\n", channel_id);
    pan1323.deallocate_packet(p);
  }