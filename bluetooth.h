#pragma once

#include <cstddef>
#include <stdarg.h>

#include "hal.h"
#include "hci.h"

class Baseband : public BufferedUART::Delegate {
 public:
  UART &uart;
  IOPin &shutdown;

  Baseband(UART &uart, IOPin &shutdown) : uart(uart), shutdown(shutdown) {}

  void configure();
  void initialize();

  void send(HCICommand const &cmd, ...);
  void data_received(BufferedUART *u);
  void error_occurred(BufferedUART *u);
};
