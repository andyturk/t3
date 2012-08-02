#include <stdint.h>
#include "adc.h"

#include "LM3S9D96.h"

// #include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/adc.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"

#include "hal.h"

ADC adc0(0);

void adc_init() {
  //
  // The ADC0 peripheral must be enabled for use.
  //
  //  SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
  adc0.configure();
  adc0.initialize();

  //
  // Enable sample sequence 3 with a processor signal trigger.  Sequence 3
  // will do a single sample when the processor sends a singal to start the
  // conversion.  Each ADC module has 4 programmable sequences, sequence 0
  // to sequence 3.  This example is arbitrarily using sequence 3.
  //
  //ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);
  adc0.configure_sequence(ADC::SEQ_3, ADC::PROCESSOR, ADC::SEQ_0);

  //
  // Configure step 0 on sequence 3.  Sample the temperature sensor
  // (ADC_CTL_TS) and configure the interrupt flag (ADC_CTL_IE) to be set
  // when the sample is done.  Tell the ADC logic that this is the last
  // conversion on sequence 3 (ADC_CTL_END).  Sequence 3 has only one
  // programmable step.  Sequence 1 and 2 have 4 steps, and sequence 0 has
  // 8 programmable steps.  Since we are only doing a single conversion using
  // sequence 3 we will only configure step 0.  For more information on the
  // ADC sequences and steps, reference the datasheet.
  //
  //ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_TS | ADC_CTL_IE |
  //                           ADC_CTL_END);
  adc0.configure_step(ADC::SEQ_3, 0, (ADC::control) (ADC::TS | ADC::IE | ADC::END), ADC::CH0, ADC::NO_CMP);

  //
  // Since sample sequence 3 is now configured, it must be enabled.
  //
  //ADCSequenceEnable(ADC0_BASE, 3);
  adc0.set_sequence_enable(ADC::SEQ_3, true);

  //
  // Clear the interrupt status flag.  This is done to make sure the
  // interrupt flag is cleared before we sample.
  //
  //ADCIntClear(ADC0_BASE, 3);
  adc0.clear_interrupt(ADC::SEQ_3);
}

int get_temperature_farenheit() {
  uint32_t adc0_value[1], C, F;

  //
  // Trigger the ADC conversion.
  //
  //ADCProcessorTrigger(ADC0_BASE, 3);
  adc0.processor_trigger(ADC::SEQ_3);

  //
  // Wait for conversion to be completed.
  //
  //while(!ADCIntStatus(ADC0_BASE, 3, false))
  //{
  //}
  while (!adc0.get_interrupt_status(ADC::SEQ_3, false));

  //
  // Clear the ADC interrupt flag.
  //
  //ADCIntClear(ADC0_BASE, 3);
  adc0.clear_interrupt(ADC::SEQ_3);

  //
  // Read ADC Value.
  //
  //  ADCSequenceDataGet(ADC0_BASE, 3, adc0_value);
  adc0.get_samples(ADC::SEQ_3, adc0_value, 1);

  //
  // Use non-calibrated conversion provided in the data sheet.  Make
  // sure you divide last to avoid dropout.
  //
  C = ((1475 * 1023) - (2250 * adc0_value[0])) / 10230;

  //
  // Get fahrenheit value.  Make sure you divide last to avoid dropout.
  //
  F = ((C * 9) + 160) / 5;

  return F;
}
