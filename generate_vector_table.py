import sys
import argparse
import string

known_chips=['lm3s9d96']

def declaration(name):
  name = '{0}_handler'.format(name)
  decl  = 'void __attribute__ ((weak)) {0}();\n'.format(name)
  decl += '#pragma weak {0} = undefined_handler\n'.format(name)
  return decl

def vector_entry(name):
  return '&{0}_handler'.format(name)

processor_exceptions = [
  'reset',
  'nmi',
  'hard_fault',
  'memmanage',
  'bus_fault',
  'usage_fault',
  'reserved_7',
  'reserved_8',
  'reserved_9',
  'reserved_10',
  'svcall',
  'debug',
  'pendsv',
  'systick']

interrupt_handlers = {
  'lm3s9d96':[
    'gpio_a',
    'gpio_b',
    'gpio_c',
    'gpio_d',
    'gpio_e',
    'uart_0',
    'uart_1',
    'ssi_0',
    'i2c_0',
    'pwm_fault',
    'pwm_generator_0',
    'pwm_generator_1',
    'pwm_generator_2',
    'qei_0',
    'adc_0_sequence_0',
    'adc_0_sequence_1',
    'adc_0_sequence_2',
    'adc_0_sequence_3',
    'watchdog',
    'timer_0_a',
    'timer_0_b',
    'timer_1_a',
    'timer_1_b',
    'timer_2_a',
    'timer_2_b',
    'comparator_0',
    'comparator_1',
    'comparator_2',
    'sysctl',
    'flash',
    'gpio_f',
    'gpio_g',
    'gpio_h',
    'uart_2',
    'ssi_1',
    'timer_3_a',
    'timer_3_b',
    'i2c_1',
    'qei_1',
    'can_0',
    'can_1',
    'reserved_57',
    'ethernet',
    'reserved_59',
    'usb',
    'pwm_generator_3',
    'udma_software',
    'udma_error',
    'adc_1_sequence_0',
    'adc_1_sequence_1',
    'adc_1_sequence_2',
    'adc_1_sequence_3',
    'i2s_0',
    'epi',
    'gpio_j']
}

template = string.Template("""
extern "C" {
void __attribute__ ((__interrupt__)) undefined_handler() {
  for(;;);
}

typedef void (*handler)();

${decls}

  extern "C" unsigned long __c_stack_top__;

__attribute__ ((section(".isr_vector"))) handler const vector_table[] = {
  (handler) &__c_stack_top__,
${vectors}
};
};
""")

parser = argparse.ArgumentParser(description='Generate Code',prog='gen_cm3')
parser.add_argument('chip', type=str,choices=known_chips)

if (len(sys.argv) == 1):
  parser.print_help()
else:
  d={}
  handlers = processor_exceptions + interrupt_handlers['lm3s9d96']
  d['decls'] = ''.join(map(declaration,handlers))
  d['vectors'] = ',\n'.join(map(vector_entry,handlers))
  print template.substitute(d)


