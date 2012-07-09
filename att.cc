#include <cstring>
#include "att.h"
#include "utils/uartstdio.h"

namespace GATT {
  Characteristic::Characteristic(uint8_t p, ATT::AttributeBase *a) :
    properties(p),
    declaration_handle(0),
    attribute(a)
  {
  }

  Service::Service(bool primary) :
    type(primary ? PRIMARY_SERVICE : SECONDARY_SERVICE)
  {
  }

  void Service::add_to(Server &server) {
    declaration_handle = server.next_handle++;
    for (Characteristic::Iterator i = characteristics.begin();
         i != characteristics.end(); ++i) {
      i->declaration_handle = server.next_handle++;
      i->attribute->handle = server.next_handle++;
    }
    join(&server.services);
  }

  void Server::add(Service &s) {
    s.join(&services);
  }

  void Server::att_packet_handler(Packet *p) {
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

    case ATT::OPCODE_FIND_TYPE_BY_VALUE_REQUEST : {
      uint16_t first_handle, last_handle, type, length;
      //uint8_t *value;

      *p >> first_handle >> last_handle >> type;
      length = p->get_remaining(); // is this right?

      for (Service::Iterator i = services.begin(); i != services.end(); ++i) {
      }
      break;
    }

    default :
      UARTprintf("unrecognized att opcode: 0x%02x\n", opcode);
      break;
    }
  }

  GAPService::GAPService(const char *s) :
    Service(true),
    name(DEVICE_NAME, (void *) s, strlen(s)),
    name_decl(Characteristic::READ, 0)
  {
  }
};



