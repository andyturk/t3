#include <stdint.h>
#include "assert.h"
#include "buffer.h"
#include "hal.h"
#include "hci.h"
#include "att.h"
#include "h4.h"
#include "att.h"
#include "gatt.h"

#ifdef DEBUG
#include "screen.h"
#endif

IOPin led1('F', 3, IOPin::LED);
IOPin pc4('C', 4, IOPin::OUTPUT);
//UART0 uart0;
UART_1 uart1;
BBand pan1323(uart1, pc4);
Systick systick(100);
H4Tranceiver h4(&uart1);
ATT_Channel att_channel(pan1323);
GAP_Service gap("Test Dev 1");
GATT_Service gatt;

class Temp {
  ADC adc0;
public:
  Temp() : adc0(0) {}
  void configure() { adc0.configure(); }
  void initialize() {
    adc0.initialize();
    // use sequence 3 triggered by the processor (priority 0)
    adc0.configure_sequence(ADC::SEQ_3, ADC::PROCESSOR, ADC::SEQ_0);

    // there's only one step in the conversion process
    // it reads from the internal temp sensor (TS), interrupts the CPU when it's done (IE)
    // it's the last step in the seqeunce (END)
    // it uses Channel 0, but has no comparator
    adc0.configure_step(ADC::SEQ_3, 0, (ADC::control) (ADC::TS | ADC::IE | ADC::END), ADC::CH0, ADC::NO_CMP);

    // enable the sequence
    adc0.set_sequence_enable(ADC::SEQ_3, true);

    // clear the interrupt before the first sample
    adc0.clear_interrupt(ADC::SEQ_3);
  }

  int farenheit() {
    uint32_t adc_samples[1], C, F;

    // trigger a sample on sequence #3
    adc0.processor_trigger(ADC::SEQ_3);

    // wait for it to finish
    while (!adc0.get_interrupt_status(ADC::SEQ_3, false));

    // clear the interrupt flag
    adc0.clear_interrupt(ADC::SEQ_3);

    // read one sample
    adc0.get_samples(ADC::SEQ_3, adc_samples, 1);

    //
    // Use non-calibrated conversion provided in the data sheet.  Make
    // sure you divide last to avoid dropout.
    //
    C = ((1475 * 1023) - (2250 * adc_samples[0])) / 10230;

    //
    // Get fahrenheit value.  Make sure you divide last to avoid dropout.
    //
    F = ((C * 9) + 160) / 5;

    return F;
  }
} internal_temperature;

class ExternalVoltage {
  IOPin pin;
  ADC adc;
  ADC::sequence seq;

  union {
    struct {
      uint32_t voltage;
      uint32_t temperature;
    };
    uint32_t samples[2];
  } values;

public:
  ExternalVoltage() :
    pin('B', 4, IOPin::ANALOG),
    adc(0),
    seq(ADC::SEQ_2)
  {}

  void configure() {
    adc.configure();
    pin.configure();
  }

  void initialize() {
    adc.initialize();
    pin.initialize();

    // disable the sequence while we work on it
    adc.set_sequence_enable(seq, false);

    // use sequence 2 triggered by the processor (priority 0)
    adc.configure_sequence(seq, ADC::PROCESSOR, ADC::SEQ_0);

    // it reads from CH10 and interrupts the CPU when it's done (IE)
    // it's the last step in the seqeunce (END)
    // it uses Channel 0, but has no comparator
    adc.configure_step(seq, 0, (ADC::control) 0,                              ADC::CH10, ADC::NO_CMP);
    adc.configure_step(seq, 1, (ADC::control) (ADC::TS | ADC::IE | ADC::END), ADC::CH0,  ADC::NO_CMP);

    // enable the sequence
    adc.set_sequence_enable(seq, true);

    // clear the interrupt before the first sample
    adc.clear_interrupt(seq);
  }

  void collect_samples() {
    // trigger a sample on sequence #3
    adc.processor_trigger(seq);

    // wait for it to finish
    while (!adc.get_interrupt_status(seq, false));

    // clear the interrupt flag
    adc.clear_interrupt(seq);

    // read one sample
    adc.get_samples(seq, values.samples, 2);
  }

  int millivolts() {
    return (values.voltage*3000)/0x3ff;
  }

  int degrees() {
    return ((1475 * 1023) - (2250 * values.temperature)) / 10230;
  }
} knob;

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

MyService my;

extern "C" int main() {
  CPU::set_clock_rate_50MHz();
  CPU::set_master_interrupt_enable(false);

#ifdef DEBUG
  Screen::the_screen.initialize();
#endif

  led1.configure();
  pc4.configure();
  //uart0.configure();
  uart1.configure();
  uart1.initialize();
  //internal_temperature.configure();
  //internal_temperature.initialize();
  knob.configure();
  knob.initialize();

  systick.configure();

  //  UARTStdioInitExpClk(0, 115200); // UART0 is the console
  // debug("console initialized\n");


  led1.initialize();
  led1.set_value(0);
  h4.set_controller(&pan1323);
  CPU::set_master_interrupt_enable(true);
  
#ifdef DEBUG
  AttributeBase::dump_attributes();
#endif

  pan1323.initialize();
  systick.initialize();

  do {
    led1.set_value(0);
    //internal_temperature.farenheit();
    pan1323.process_incoming_packets();
    led1.set_value(1);
    asm volatile ("wfi");
  } while (true);
}

extern "C" void __attribute__ ((isr)) uart_1_handler() {
  h4.uart_interrupt();
}

uint32_t systick_counter = 0;

extern "C" void __attribute__ ((isr)) systick_handler() {
  if (systick_counter++ == 10) {
    led1.set_value(0);
    //int degrees = internal_temperature.farenheit();
    //debug("the temperature is %dF\n", degrees);
    knob.collect_samples();
    debug("the voltage is %d, temp = %dC\n", knob.millivolts(), knob.degrees());
    systick_counter = 0;
    led1.set_value(1);
  }
}

