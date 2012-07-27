#ifndef __arm__
#include <cstdio>
#include <iostream>
#include <fstream>
#include <iomanip>
using namespace std;
#endif

#include "bts.h"

namespace BTS {
#ifndef __arm__
  void Recorder::header(script_header &h) {
    script.write((uint8_t *) &h, sizeof(h));
  }

  void Recorder::send(Packet &action) {
    const uint16_t length = action.get_remaining();
    script << (uint16_t) SEND_COMMAND << length;
    script.write((uint8_t *) action, length);
  }

  void Recorder::expect(uint32_t msec, Packet &action) {
    const uint16_t length2 = action.get_remaining();
    const uint32_t length4 = length2 + 2*sizeof(uint32_t);
    script << (uint16_t) WAIT_EVENT << length2;
    script << msec << length2;
    script.write((uint8_t *) action, length2);
  }

  void Recorder::configure(uint32_t baud, flow_control control) {
    const uint16_t length = sizeof(configuration);
    script << (uint16_t) SERIAL_PORT_PARAMETERS << length << baud << control;
  }

  void Recorder::call(const char *filename) {
    const uint16_t length = strlen(filename);
    script << (uint16_t) RUN_SCRIPT << length;
    script.write((const uint8_t *) filename, length);
  }

  void Recorder::comment(const char *text) {
    const uint16_t length = strlen(text);
    script << (uint16_t) RUN_SCRIPT << length;
    script.write((const uint8_t *) text, length);
  }

  void Recorder::done() {
    // nothing to do
  }

  void Recorder::error(const char *reason) {
  }
#endif

  Player::Player() : last_opcode(0) {
  }

  void Player::reset(const uint8_t *bytes, uint16_t length) {
    script.initialize((uint8_t *) bytes, length);
    script_header h;
    header(h);
  }

  void Player::header(script_header &h) {
    assert(script.get_remaining() >= sizeof(h));
    script.read((uint8_t *) &h, sizeof(h));
    if (h.magic != BTSB) error("bad magic");
  }

  void Player::play_next_action() {
    if (script.get_remaining() == 0) {
      done();
    } else if(script.get_remaining() < sizeof(command_header)) {
      error("bad script");
    } else {
      uint8_t *start = (uint8_t *) script;
      uint16_t action, length;

      script >> action >> length;
      command.initialize(start, length + sizeof(command_header));
      script.skip(length);
      command.seek(sizeof(command_header));

      switch (action) {
      case SEND_COMMAND : {
        if (command.peek(0) == HCI::COMMAND_PACKET) {
          uint8_t indicator;
          uint16_t p = command.get_position();
          command >> indicator >> last_opcode;
          command.seek(p);
        }
        send(command);
        break;
      }

      case WAIT_EVENT : {
        uint32_t msec, length2;

        command >> msec >> length2;
        expect(msec, command);
        break;
      }

      case SERIAL_PORT_PARAMETERS : {
        configuration config;
        command >> config.baud >> config.control;
        command.reset();
        configure(config.baud, (flow_control) config.control);
        break;
      }

      case RUN_SCRIPT : {
        call((char *) (uint8_t *) command);
        break;
      }

      case REMARKS : {
        comment((char *) (uint8_t *) command);
        break;
      }

      default :
        error("bad action");
        return;
      }
    }
  }

#ifndef __arm__
  StreamlineForDevice::StreamlineForDevice(const char *n) :
    declaration_name(n)
  {
    cout << "const uint8_t " << declaration_name << "[] = {\n";
  }

  void StreamlineForDevice::emit(const uint8_t *bytes, uint16_t size) {
    reset(bytes, size);
    while (!is_complete()) play_next_action();
  }

  void StreamlineForDevice::as_hex(const uint8_t *bytes, uint16_t size, const char *start) {
    const uint8_t *limit = bytes + size;

    cout << hex << setfill('0');
    while (bytes < limit) {
      if (start) cout << start;
      for (int i=0; i < 16 && bytes < limit; ++i)
        cout << "0x" << setw(2) << (int) *bytes++ << ", ";
      cout << "\n";
    }
    cout << dec;
  }

  StreamlineForDevice::~StreamlineForDevice() {
    cout << "};\n";
  }

  void StreamlineForDevice::send(Packet &action) {
    action.reset();
    as_hex((uint8_t *) action, action.get_remaining(), "  ");
    cout << endl;
  }

  void StreamlineForDevice::expect(uint32_t msec, Packet &action) {
    cout << "// within " << msec << " msec expect:" << endl;
    as_hex((uint8_t *) action, action.get_remaining(), "// ");
    cout << endl;

    uint8_t indicator, event, parameter_length, hci_packets, status;
    action >> indicator >> event >> parameter_length;

    if (indicator != HCI::EVENT_PACKET || event != HCI::EVENT_COMMAND_COMPLETE) {
      cerr << "unsupported expectation\n";
      exit(1);
    }

    uint16_t opcode;
    action >> hci_packets >> opcode >> status;

    if (last_opcode != 0 && last_opcode != opcode) {
      cerr << "expectation not for last opcode\n";
      exit(1);
    }

    last_opcode = 0;
  }

  void StreamlineForDevice::configure(uint32_t baud, flow_control control) {
    const char *c;

    switch (control) {
    case NONE : c = "NONE"; break;
    case HARDWARE : c = "HARDWARE"; break;
    case NO_CHANGE : c = "NO_CHANGE"; break;
    default :
      cerr << "bad flow control specification\n";
      exit(1);
    }

    cout << "// set baud to " << baud << " with flow control: " << c << endl;
    command.reset();
    as_hex((uint8_t *) command, command.get_remaining());
  }

  void StreamlineForDevice::call(const char *filename) {
    cerr << "script chaining not supported\n";
    exit(1);
  }

  void StreamlineForDevice::comment(const char *text) {
    cout << "//" << text << endl;
  }

  void StreamlineForDevice::error(const char *msg) {
    cerr << msg;
    exit(1);
  }
#endif

#ifdef __arm__
  H4Player::H4Player(H4Tranceiver &h) :
    h4(h)
  {
  }

  void H4Player::reset(const uint8_t *bytes, uint16_t length) {
    Player::reset(bytes, length);
    h4.reset();
    status = HCI::SUCCESS;
    out = h4.command_packets.allocate();
    play_next_action();
  }

  void H4Player::sent(Packet *p) {
  }

  void H4Player::command_succeeded(uint16_t opcode, Packet *p) {
  }

  void H4Player::received(Packet *p) {
    uint8_t indicator, event, command_packet_budget;
    uint16_t opcode;

    *p >> indicator;
    if (indicator == HCI::EVENT_PACKET) {
      *p >> event;

      if (event == HCI::EVENT_COMMAND_COMPLETE) {
        *p >> command_packet_budget >> opcode >> status;

        if (last_opcode == 0 || last_opcode == opcode) {
          if (status == HCI::SUCCESS) {
            last_opcode = 0;
            command_succeeded(opcode, p);

            if (!is_complete()) play_next_action();
            return;
          }
        }
      }
    }

    status = HCI::COMMAND_DISALLOWED;
    script.seek(script.get_limit());
  }

  void H4Player::send(Packet &action) {
    out->reset();
    out->write((uint8_t *) action, action.get_remaining());
    out->flip();
    out->join(&h4.packets_to_send);

    uint8_t indicator;

    action >> indicator >> last_opcode;
    assert(indicator == HCI::COMMAND_PACKET);

    action.reset();
  }
#endif
};


#ifndef __arm__
void bluetooth_init_cc2564_raw(const char *file, uint8_t *&value, size_t &size) {
  value = 0;
  size = 0;

  ifstream bts_file(file, ios::in | ios::binary | ios::ate);

  if (bts_file.is_open()) {
    bts_file.seekg(0, ios::end);
    size = bts_file.tellg();
    bts_file.seekg(0, ios::beg);

    value = new uint8_t[size];
    bts_file.read((char *) value, size);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    cerr << "usage: bts filename\n";
    exit(1);
  }

  uint8_t *raw_patch;
  size_t raw_patch_size;

  bluetooth_init_cc2564_raw(argv[1], raw_patch, raw_patch_size);

  if (raw_patch == 0 || raw_patch_size == 0) {
    cerr << "couldn't load " << argv[1] << endl;
    exit(1);
  }

  BTS::StreamlineForDevice streamliner("bluetooth_init_cc2564");
  streamliner.emit(raw_patch, raw_patch_size);

  delete raw_patch;
}
#endif
