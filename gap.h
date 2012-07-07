#pragma once

// https://www.bluetooth.org/Technical/AssignedNumbers/Generic-Attribute-Profile.htm

enum gatt_service {
  GENERIC_ACCESS_PROFILE                     = x01800,
  GENERIC_ATTRIBUTE_PROFILE                  = 0x1801
};

enum gatt_attribute_type {
  PRIMARY_SERVICE                            = 0x2800,
  SECONDARY_SERVICE                          = 0x2801,
  INCLUDE                                    = 0x2801,
  CHARACTERISTIC                             = 0x2803
};

enum gatt_characteristic_descriptor {
  CHARACTERISTIC_EXTENDED_PROPERTIES         = 0x2900,
  CHARACTERISTIC_USER_DESCRIPTION            = 0x2901,
  CLIENT_CHARACTERISTIC_DESCRIPTION          = 0x2902,
  SERVER_CHARACTERISTIC_CONFIGURATION        = 0x2903,
  CHARACTERISTIC_FORMAT                      = 0x2904,
  CHARACTERISTIC_AGGREGATE_FORMAT            = 0x2905
};

enum gatt_characteristic_type {
  DEVICE_NAME                                = 0x2a00,
  APPEARANCE                                 = 0x2a01,
  PERIPHERAL_PRIVACY_FLAG                    = 0x2a02,
  RECONNECTION_ADDRESS                       = 0x2a03,
  PERIPHERAL_PREFERRED_CONNECTION_PARAMETERS = 0x2a04,
  SERVICE_CHANGED                            = 0x2a05
};
