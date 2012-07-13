#pragma once

#include "att.h"

struct CharacteristicDecl : public AttributeBase {
  struct {
    uint8_t properties;
    uint16_t handle;
    union {
      uint8_t full_uuid[sizeof(UUID)];
      uint16_t short_uuid;
    };
  } __attribute__ ((packed)) _decl ;

  CharacteristicDecl(uint16_t uuid) :
    AttributeBase(GATT::CHARACTERISTIC, &_decl, sizeof(_decl))
  {
    _decl.properties = 0;
    _decl.handle = 0;
    _decl.short_uuid = (uint16_t) uuid;
    length = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t);
  }

  CharacteristicDecl(const UUID &uuid) :
    AttributeBase(GATT::CHARACTERISTIC, &_decl, sizeof(_decl))
  {
    _decl.properties = 0;
    _decl.handle = 0;
    memcpy(_decl.full_uuid, (const uint8_t *) uuid, sizeof(UUID));
    length = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(UUID);
  }
};

template<typename T>
struct Characteristic : public CharacteristicDecl {
  Attribute<T> value;
  Characteristic(const UUID &uuid) :
    CharacteristicDecl(GATT::CHARACTERISTIC),
    value(uuid)
  {
    _decl.handle = value.handle;
  }
  Characteristic(uint16_t uuid) :
    CharacteristicDecl(GATT::CHARACTERISTIC),
    value(uuid)
  {
    _decl.handle = value.handle;
  }
    Characteristic &operator=(const T &rhs) {value = rhs; return *this;}
};


struct GATT_Service : public Attribute<uint16_t> {
  CharacteristicDecl changed;

  GATT_Service() :
    Attribute<uint16_t>(GATT::PRIMARY_SERVICE, GATT::GENERIC_ATTRIBUTE_PROFILE),
      changed(GATT::SERVICE_CHANGED)
  {
    changed._decl.properties = GATT::INDICATE | GATT::WRITE_WITHOUT_RESPONSE | GATT::READ;
  }
  virtual uint16_t group_end() {return changed.handle;}
};

struct MyService : public Attribute<uint16_t> {
  Characteristic<char> char_1;
  Characteristic<char> char_2;
  Characteristic<char> char_3;

  MyService() :
    Attribute<uint16_t>(GATT::PRIMARY_SERVICE, 0xfff0),
      char_1((uint16_t) 0xfff1),
      char_2((uint16_t) 0xfff2),
      char_3("00001234-0000-1000-8000-00805F9B34FB")
  {
  }

  virtual uint16_t group_end() {return char_3.handle;}
};

struct GAP_Service : public Attribute<uint16_t> {
  Characteristic<const char *> device_name;
  Characteristic<uint16_t> appearance;

  GAP_Service(const char *name, uint16_t a = 0) :
    Attribute<uint16_t>(GATT::PRIMARY_SERVICE, GATT::GENERIC_ACCESS_PROFILE),
      device_name(GATT::DEVICE_NAME),
      appearance(GATT::APPEARANCE)
  {
    appearance = a;
    device_name = name;
  }
  virtual uint16_t group_end() {return appearance.handle;}
};
