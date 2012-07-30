#ifndef __arm__
#include <cstdio>
#include <iostream>
#include <fstream>
#include <iomanip>
using namespace std;
#endif

#include "bts.h"
#include "hal.h"

#ifdef __arm__
extern uint8_t bluetooth_init_cc2564[];
extern size_t bluetooth_init_cc2564_size;
#endif

namespace BTS {
  Script::Script(uint8_t *bytes, uint16_t length) :
    script(bytes, length)
  {
  }


  void Script::reset(const uint8_t *bytes, uint16_t length) {
    script_header h;

    script.initialize((uint8_t *) bytes, length);
    assert(script.get_remaining() >= sizeof(h));
    script.read((uint8_t *) &h, sizeof(h));
    header(h);
  }

  void Script::header(script_header &h) {
    if (h.magic != BTSB) error("bad magic");
  }


  void Script::send(Packet &action) {
  }

  void Script::expect(uint32_t msec, Packet &action) {
  }

  void Script::configure(uint32_t baud, flow_control control) {
  }

  void Script::call(const char *filename) {
  }

  void Script::comment(const char *text) {
  }

  void Script::error(const char *reason) {
  }

  void Script::done() {
  }

  void Script::play_next_action() {
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
          debug("sending 0x%04x\n", last_opcode);
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
        command.rewind();
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
  SourceGenerator::SourceGenerator(const char *n, uint8_t *bytes, uint16_t length) :
    Script(bytes, length),
    name(n)
  {
    cout << "#include <stdint.h>\n";
    cout << "extern \"C\" const uint8_t " << name << "[] = {\n";

    script_header sh;
    memset(&sh, 0, sizeof(sh));
    sh.magic = BTSB;

    cout << "// 32-byte BTS header\n";
    as_hex((const uint8_t *) &sh, sizeof(sh), "  ");
  }

  SourceGenerator::~SourceGenerator() {
    cout << "};\n";
    cout << "extern \"C\" const uint32_t " << name << "_size = sizeof(" << name << ");\n";
    
  }

  void SourceGenerator::emit(const uint8_t *bytes, uint16_t size) {
    reset(bytes, size);
    while (!is_complete()) play_next_action();
  }

  void SourceGenerator::as_hex(const uint8_t *bytes, uint16_t size, const char *start) {
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

  void SourceGenerator::send(Packet &action) {
    command_header ch = {SEND_COMMAND, action.get_remaining()};

    uint16_t pos = action.tell();

    uint8_t indicator;
    action >> indicator;

    if (indicator == COMMAND_PACKET) {
      uint16_t opcode;
      action >> opcode;

      if (opcode == OPCODE_SLEEP_MODE_CONFIGURATIONS) {
        // omit the OEM sleep mode configuration command
        return;
      }
    }

    action.rewind(pos);
    as_hex((uint8_t *) &ch, sizeof(ch), "  ");
    as_hex((uint8_t *) action, action.get_remaining(), "  ");
    cout << endl;
  }

  void SourceGenerator::expect(uint32_t msec, Packet &action) {
    cout << "// within " << msec << " msec expect:" << endl;
    as_hex((uint8_t *) action, action.get_remaining(), "// ");
    cout << endl;
  }

  void SourceGenerator::configure(uint32_t baud, flow_control control) {
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
    //command.rewind();
    //as_hex((uint8_t *) command, command.get_remaining(), "  ");
  }

  void SourceGenerator::call(const char *filename) {
    cerr << "script chaining not supported\n";
    exit(1);
  }

  void SourceGenerator::comment(const char *text) {
    cout << "// " << text << endl;
  }

  void SourceGenerator::error(const char *msg) {
    cerr << msg;
    exit(1);
  }
#endif

#ifdef __arm__
  H4Script::H4Script(H4Tranceiver &h, uint8_t *bytes, uint16_t length) :
    Script(bytes, length),
    Sequence(h)
  {
  }

  bool H4Script::is_complete() const {
    return Script::is_complete() && last_opcode == 0;
  }

  void H4Script::reset(const uint8_t *bytes, uint16_t length) {
    Script::reset(bytes, length);
    h4.reset();
    status = HCI::SUCCESS;
    play_next_action();
  }

  bool H4Script::command_complete(uint16_t opcode, Packet *p) {
    if (opcode != last_opcode) return false;
    last_opcode = 0; // clear it out

    switch (opcode) {
    case OPCODE_PAN13XX_CHANGE_BAUD_RATE:
      h4.uart->set_baud(baud_rate);
      break;

    default :
      break;
    }

    debug("OK 0x%04x\n", opcode);
    p->deallocate();
    play_next_action();
    return true;
  }

  void H4Script::play_next_action() {
    Script::play_next_action();
    h4.fill_uart();
  }

  void H4Script::send(Packet &action) {
    Packet *out = h4.command_packets.allocate();

    out->reset();
    out->write((uint8_t *) action, action.get_remaining());
    out->flip();
    out->join(&h4.packets_to_send);

    uint8_t indicator;

    action >> indicator >> last_opcode;
    assert(indicator == HCI::COMMAND_PACKET);

    if (last_opcode == OPCODE_PAN13XX_CHANGE_BAUD_RATE) {
      uint8_t parameter_length;
      action >> parameter_length >> baud_rate;
      assert(parameter_length == sizeof(baud_rate));
    }
    action.reset();
  }

  void H4Script::configure(uint32_t baud, flow_control control) {
    h4.uart->set_baud(baud);
    assert(control == HARDWARE || control == NO_CHANGE);
    play_next_action();
  }

  void H4Script::done() {
    for(;;);
  }


  void H4Script::go() {
    /*
  __asm("cpsid i");
    H4Controller *saved = h4.get_controller();
    h4.set_controller(this);
  __asm("cpsie i");
    */
  reset(bluetooth_init_cc2564, bluetooth_init_cc2564_size);

  /*
  while (!is_complete()) {
    asm volatile ("wfi");
  }
  */
  /*
  __asm("cpsid i");
    h4.set_controller(saved);
  __asm("cpsie i");
  */
  }
#endif
};


#ifndef __arm__
void file_as_bytes(const char *file, uint8_t *&value, size_t &size) {
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

  file_as_bytes(argv[1], raw_patch, raw_patch_size);

  if (raw_patch == 0 || raw_patch_size == 0) {
    cerr << "couldn't load " << argv[1] << endl;
    exit(1);
  }

  BTS::SourceGenerator code("bluetooth_init_cc2564", raw_patch, raw_patch_size);
  SizedPacket<259> p;

  code.comment("Reset Pan13XX");
  p.hci(HCI::OPCODE_RESET);
  p.prepare_for_tx();
  code.send(p);

  code.comment("Read local version info");
  p.hci(HCI::OPCODE_READ_LOCAL_VERSION_INFORMATION);
  p.prepare_for_tx();
  code.send(p);

  // code.comment("Change baud rate to 921600 since we'll be downloading large patches");
  // (p.hci(HCI::OPCODE_PAN13XX_CHANGE_BAUD_RATE) << (uint32_t) 921600L).flip();
  // code.send(p);

  // send OEM patch
  code.comment(argv[1]);
  code.emit(raw_patch, raw_patch_size);
  delete raw_patch;

  code.comment("Disable sleep modes");
  p.hci(HCI::OPCODE_SLEEP_MODE_CONFIGURATIONS);
  p << (uint8_t) 0x01; // big sleep enable
  p << (uint8_t) 0x00; // deep sleep enable
  p << (uint8_t) 0x00; // deep sleep mode (0xff = no change)
  p << (uint8_t) 0xff; // output I/O select (0xff = no change)
  p << (uint8_t) 0xff; // output pull enable (0xff = no change)
  p << (uint8_t) 0xff; // input pull enable (0xff = no change)
  p << (uint8_t) 0xff; // input I/O select (0xff = no change)
  p << (uint16_t) 100; // reserved -- must be 0?
  p.prepare_for_tx();
  code.send(p);

  code.comment("get BD_ADDR");
  p.hci(HCI::OPCODE_READ_BD_ADDR);
  p.prepare_for_tx();
  code.send(p);

  code.comment("get buffer size info");
  p.hci(HCI::OPCODE_READ_BUFFER_SIZE_COMMAND);
  p.prepare_for_tx();
  code.send(p);

  code.comment("write page timeout");
  (p.hci(HCI::OPCODE_WRITE_PAGE_TIMEOUT) << (uint16_t) 0x2000);
  p.prepare_for_tx();
  code.send(p);

  code.comment("read page timeout");
  (p.hci(HCI::OPCODE_READ_PAGE_TIMEOUT));
  p.prepare_for_tx();
  code.send(p);

  code.comment("write local name");
  char local_name[248];
  strncpy(local_name, "Super Whizzy Gizmo 1.1", sizeof(local_name));
  p.hci(HCI::OPCODE_WRITE_LOCAL_NAME_COMMAND);
  p.write((uint8_t *) local_name, sizeof(local_name));
  p.prepare_for_tx();
  code.send(p);

  code.comment("write scan enable");
  (p.hci(HCI::OPCODE_WRITE_SCAN_ENABLE) << (uint8_t) 0x03);
  p.prepare_for_tx();
  code.send(p);

  code.comment("write class of device");
  code.comment("see http://bluetooth-pentest.narod.ru/software/bluetooth_class_of_device-service_generator.html");
  p.hci(HCI::OPCODE_WRITE_CLASS_OF_DEVICE);
  p << (uint32_t) 0x0098051c;
  p.prepare_for_tx();
  code.send(p);

  code.comment("set event mask");
  p.hci(HCI::OPCODE_SET_EVENT_MASK) << (uint32_t) 0xffffffff << (uint32_t) 0x20001fff;
  p.prepare_for_tx();
  code.send(p);

  code.comment("write le host support");
  p.hci(HCI::OPCODE_WRITE_LE_HOST_SUPPORT) << (uint8_t) 1 << (uint8_t) 1;
  p.prepare_for_tx();
  code.send(p);

  code.comment("le set event mask");
  p.hci(HCI::OPCODE_LE_SET_EVENT_MASK) << (uint32_t) 0x0000001f << (uint32_t) 0x00000000;
  p.prepare_for_tx();
  code.send(p);

  code.comment("le read buffer size");
  p.hci(HCI::OPCODE_LE_READ_BUFFER_SIZE);
  p.prepare_for_tx();
  code.send(p);

  code.comment("le read supported states");
  p.hci(HCI::OPCODE_LE_READ_SUPPORTED_STATES);
  p.prepare_for_tx();
  code.send(p);

  code.comment("le set advertising parameters");
  p.hci(HCI::OPCODE_LE_SET_ADVERTISING_PARAMETERS);
  p << (uint16_t) 0x0800;
  p << (uint16_t) 0x4000;
  p << (uint8_t) 0x00;
  p << (uint8_t) 0x00;
  p << (uint8_t) 0x00;
  p.prepare_for_tx();
  code.send(p);

}
#endif
