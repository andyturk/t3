#include "l2cap.h"

namespace L2CAP {
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
};
