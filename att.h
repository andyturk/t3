#pragma once

#include <stdint.h>
#include "hci.h"
#include "uuid.h"
#include "ring.h"

namespace ATT {
  enum opcodes {
    OPCODE_ERROR = 0x01,
    OPCODE_EXCHANGE_MTU_REQUEST = 0x02,
    OPCODE_EXCHANGE_MTU_RESPONSE = 0x03,
    OPCODE_FIND_INFORMATION_REQUEST = 0x04,
    OPCODE_FIND_INFORMATION_RESPONSE = 0x05,
    OPCODE_FIND_TYPE_BY_VALUE_REQUEST = 0x06,
    OPCODE_FIND_TYPE_BY_VALUE_RESPONSE = 0x07,
    OPCODE_READ_BY_TYPE_REQUEST = 0x08,
    OPCODE_READ_BY_TYPE_RESPONSE = 0x09,
    OPCODE_READ_REQUEST = 0x0a,
    OPCODE_READ_RESPONSE = 0x0b,
    OPCODE_READ_BLOB_REQUEST = 0x0c,
    OPCODE_READ_BLOB_RESPONSE = 0x0d,
    OPCODE_READ_MULTIPLE_REQUEST = 0x0e,
    OPCODE_READ_MULTIPLE_RESPONSE = 0x0f,
    OPCODE_READ_BY_GROUP_TYPE_REQUEST = 0x10,
    OPCODE_READ_BY_GROUP_TYPE_RESPONSE = 0x11,
    OPCODE_WRITE_REQUEST = 0x12,
    OPCODE_WRITE_RESPONSE = 0x13,
    OPCODE_WRITE_COMMAND = 0x52,
    OPCODE_SIGNED_WRITE_COMMAND = 0xd2,
    OPCODE_PREPARE_WRITE_REQUEST = 0x16,
    OPCODE_PREPARE_WRITE_RESPONSE = 0x17,
    OPCODE_EXECUTE_WRITE_REQUEST = 0x18,
    OPCODE_EXECUTE_WRITE_RESPONSE = 0x19,
    OPCODE_HANDLE_VALUE_NOTIFICATION = 0x1b,
    OPCODE_HANDLE_VALUE_INDICATION = 0x1d,
    OPCODE_HANDLE_VALUE_CONFIRMATION = 0x1e
  };

  enum error {
    INVALID_HANDLE = 0x01,
    READ_NOT_PERMITTED = 0x02,
    WRITE_NOT_PERMITTED = 0x03,
    INVALID_PDU = 0x04,
    INSUFFICIENT_AUTHENTICATION = 0x05,
    REQUEST_NOT_SUPPORTED = 0x06,
    INVALID_OFFSET = 0x07,
    INSUFFICIENT_AUTHORIZATION = 0x08,
    PREPARE_QUEUE_FULL = 0x09,
    ATTRIBUTE_NOT_FOUND = 0x0a,
    ATTRIBUTE_NOT_LONG = 0x0b,
    INSUFFICIENT_ENCRYPTION_KEY_SIZE = 0x0c,
    INVALID_ATTRIBUTE_VALUE_LENGTH = 0x0d,
    UNLIKELY_ERROR = 0x0e,
    INSUFFICIENT_ENCRYPTION = 0x0f,
    UNSUPPORTED_GROUP_TYPE = 0x10,
    INSUFFICIENT_RESOURCES = 0x11
  };

  struct AttributeBase {
    UUID type;
    uint16_t handle;
    void *_data;
    uint16_t length;

    AttributeBase(const UUID &t, void *d, uint16_t l) : type(t), _data(d), length(l) {}
    AttributeBase(int16_t t, void *d, uint16_t l) : type(t), _data(d), length(l) {}
  };

  template<typename T>
  class Attribute : public AttributeBase {
    T data;

  public:
    Attribute(const UUID &u) : AttributeBase(u, &data, sizeof(data)) {}
    Attribute(uint16_t u) : AttributeBase(u, &data, sizeof(data)) {}
  };
};

namespace GAP {
  // https://www.bluetooth.org/Technical/AssignedNumbers/generic_access_profile.htm
  enum advertising_data_type {
    FLAGS                              = 0x01,
    INCOMPLETE_16BIT_UUIDS             = 0x02,
    COMPLETE_16BIT_UUIDS               = 0x03,
    INCOMPLETE_32BIT_UUIDS             = 0x04,
    COMPLETE_32BIT_UUIDS               = 0x05,
    INCOMPLETE_128BIT_UUIDS            = 0x06,
    COMPLETE_128BIT_UUIDS              = 0x07,
    SHORTENED_LOCAL_NAME               = 0x08,
    COMPLETE_LOCAL_NAME                = 0x09,
    TX_POWER_LEVEL                     = 0x0a,
    CLASS_OF_DEVICE                    = 0x0d,
    SIMPLE_PAIRING_HASH_C              = 0x0e,
    SIMPLE_PAIRING_RANDOMIZER_R        = 0x0f,
    DEVICE_ID                          = 0x10,
    SECURITY_MANAGER_TK_VALUE          = 0x10,
    SECURITY_MANAGER_OUT_OF_BAND_FLAGS = 0x11,
    SLAVE_CONNECTION_INTERVAL_RANGE    = 0x12,
    SERVICE_SOLICITATION_16BIT_UUIDS   = 0x14,
    SERVICE_SOLICITATION_128BIT_UUIDS  = 0x15,
    PUBLIC_TARGET_ADDRESS              = 0x17,
    RANDOM_TARGET_ADDRESS              = 0x18,
    APPEARANCE                         = 0x19,
    MANUFACTURER_SPECIFIC_DATA         = 0xff
  };

  enum advertising_flag_masks {
    LE_LIMITED_DISCOVERABLE_MODE       = 0x01,
    LE_GENERAL_DISCOVERABLE_MODE       = 0x02,
    BR_EDR_NOT_SUPPORTED               = 0x04,
    SIMULTANEOUS_CONTROLLER            = 0x08,
    SIMULTANEOUS_HOST                  = 0x10
  };
};

namespace GATT {
  // https://www.bluetooth.org/Technical/AssignedNumbers/Generic-Attribute-Profile.htm

  enum service_type {
    GENERIC_ACCESS_PROFILE                     = 0x1800,
    GENERIC_ATTRIBUTE_PROFILE                  = 0x1801
  };

  enum attribute_type {
    PRIMARY_SERVICE                            = 0x2800,
    SECONDARY_SERVICE                          = 0x2801,
    INCLUDE                                    = 0x2801,
    CHARACTERISTIC                             = 0x2803
  };

  enum characteristic_descriptor {
    CHARACTERISTIC_EXTENDED_PROPERTIES         = 0x2900,
    CHARACTERISTIC_USER_DESCRIPTION            = 0x2901,
    CLIENT_CHARACTERISTIC_DESCRIPTION          = 0x2902,
    SERVER_CHARACTERISTIC_CONFIGURATION        = 0x2903,
    CHARACTERISTIC_FORMAT                      = 0x2904,
    CHARACTERISTIC_AGGREGATE_FORMAT            = 0x2905
  };

  enum characteristic_type {
    DEVICE_NAME                                = 0x2a00,
    APPEARANCE                                 = 0x2a01,
    PERIPHERAL_PRIVACY_FLAG                    = 0x2a02,
    RECONNECTION_ADDRESS                       = 0x2a03,
    PERIPHERAL_PREFERRED_CONNECTION_PARAMETERS = 0x2a04,
    SERVICE_CHANGED                            = 0x2a05
  };

  enum characteristic_formats {
    BOOLEAN = 0x01,
    TWOBIT  = 0x02,
    NIBBLE  = 0x03,
    UINT8   = 0x04,
    UINT12  = 0x05,
    UINT16  = 0x06,
    UINT24  = 0x07,
    UINT32  = 0x08,
    UINT48  = 0x09,
    UINT64  = 0x0a,
    UINT128 = 0x0b,
    SINT8   = 0x0c,
    SINT12  = 0x0d,
    SINT16  = 0x0e,
    SINT24  = 0x0f,
    SINT32  = 0x10,
    SINT48  = 0x11,
    SINT64  = 0x12,
    SINT128 = 0x13,
    FLOAT32 = 0x14,
    FLOAT64 = 0x15,
    SFLOAT  = 0x16,
    FLOAT   = 0x17,
    DUINT16 = 0x18,
    UTF8S   = 0x19,
    UTF16S  = 0x1a,
    STRUCT  = 0x1b
  };

  struct Server;

  struct Characteristic : public Ring<Characteristic> {
    enum property_masks {
      BROADCAST                   = 0x01,
      READ                        = 0x02,
      WRITE_WITHOUT_RESPONSE      = 0x04,
      WRITE                       = 0x08,
      NOTIFY                      = 0x10,
      INDICATE                    = 0x20,
      AUTHENTICATED_SIGNED_WRITES = 0x40,
      EXTENDED_PROPERTIES         = 0x80
    };

    uint8_t properties;
    uint16_t declaration_handle;
    ATT::AttributeBase *attribute;
    Characteristic(uint8_t p, ATT::AttributeBase *a);
  };

  struct Service : public Ring<Service> {
    const uint16_t type;
    uint16_t declaration_handle;
    Ring<Service> includes;
    Ring<Characteristic> characteristics;

    Service(bool primary);
    void add_to(Server &s);
  };

  class GAPService : public Service {
    ATT::AttributeBase name;
    Characteristic name_decl;

  public:
    GAPService(const char *name);
  };

  struct Server {
    uint16_t next_handle;
    Ring<Service> services;
    void add(Service &s);

    Server();

    void att_packet_handler(Packet *p);
    void find_by_type_value_request(uint16_t from, uint16_t to, uint16_t type, uint8_t *value, uint16_t length);
    
  };
};

