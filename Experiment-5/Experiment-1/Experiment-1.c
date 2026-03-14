#include "firebird_simulation.h"
#include <avr/interrupt.h>

void buzzer_pin_config(void) {
    buzzer_ddr_reg = (buzzer_ddr_reg | (1 << buzzer_pin));
    buzzer_port_reg = (buzzer_port_reg & ~(1 << buzzer_pin));
}

void interrupt_sw_pin_config(void) {
    interrupt_sw_ddr_reg = (interrupt_sw_ddr_reg & ~(1 << interrupt_sw_pin));
    interrupt_sw_port_reg = (interrupt_sw_port_reg | (1 << interrupt_sw_pin));
}

void interrupt_sw_config(void) {
    cli();
    EIMSK_reg = (EIMSK_reg | (1 << interrupt_switch_pin));
    EICRB_reg = (EICRB_reg & ~((1 << interrupt_ISC_switch_bit1) | (1 << interrupt_ISC_switch_bit0)));
    sei();
}

void buzzer_on(void) {
    buzzer_port_reg = (buzzer_port_reg | (1 << buzzer_pin));
}

void buzzer_off(void) {
    buzzer_port_reg = (buzzer_port_reg & ~(1 << buzzer_pin));
}

ISR(interrupt_isr_vect) {
    buzzer_on();
}

int main(void) {
    buzzer_pin_config();
    interrupt_sw_pin_config();
    interrupt_sw_config();
    while (1) {
        buzzer_off();
    }
}


