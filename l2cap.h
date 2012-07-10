#pragma once

#include <stdint.h>

#include "hci.h"
#include "ring.h"

namespace L2CAP {
  enum fixed_channel_ids {
    SIGNALLING_CID       = 0x0001,
    CONNECTIONLESS_CID   = 0x0002,
    AMP_MANAGER_CID      = 0x0003,
    ATTRIBUTE_CID        = 0x0004,
    LE_SIGNALLING_CID    = 0x0005,
    SECURITY_MANAGER_CID = 0x0006,
    AMP_TEST_MANAGER_CID = 0x003f
  };

  class Channel : public Ring<Channel> {
    static Ring<Channel> channels;

  public:
    const uint16_t channel_id;
    uint16_t mtu;

    Channel(uint16_t c);
    ~Channel();

    static Channel *find(uint16_t id);
  };
};
