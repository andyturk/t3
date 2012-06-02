#pragma once

#include "hal.h"

class HCITransport {
  UART &uart;
  IOPin &shutdown;

 public:
  HCITransport(UART &uart, IOPin &shutdown);

  void configure();
  void initialize();
};
