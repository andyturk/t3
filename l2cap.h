#pragma once

#include <stdint.h>

#include "hci.h"
#include "ring.h"

class Channel : public Ring<Channel> {
  static Ring<Channel> channels;

 protected:
  HostController &controller;

 public:
  const uint16_t channel_id;
  uint16_t mtu;

  Channel(uint16_t c, HostController &hc);
  ~Channel();

  virtual void receive(Packet *p);
  virtual void send(Packet *p);
  static Channel *find(uint16_t id);
};
