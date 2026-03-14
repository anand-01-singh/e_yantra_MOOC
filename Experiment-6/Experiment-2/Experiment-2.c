#include "firebird_simulation.h"
#include <util/delay.h>

void motors_pin_config(void) {
	motors_dir_ddr_reg = (motors_dir_ddr_reg & 0xF0)
	| (1 << motors_RB_pin)
	| (1 << motors_RF_pin)
	| (1 << motors_LF_pin)
	| (1 << motors_LB_pin);

	motors_dir_port_reg = (motors_dir_port_reg & 0xF0)
	& ~((1 << motors_RB_pin)
	| (1 << motors_RF_pin)
	| (1 << motors_LF_pin)
	| (1 << motors_LB_pin));
}

void pwm_pin_config(void) {
	motors_pwm_ddr_reg = (motors_pwm_ddr_reg & 0xE7)
	| (1 << motors_pwm_R_pin)
	| (1 << motors_pwm_L_pin);

	motors_pwm_port_reg = (motors_pwm_port_reg & 0xE7)
	| (1 << motors_pwm_R_pin)
	| (1 << motors_pwm_L_pin);
}

void motors_move_forward(void) {
	motors_dir_port_reg = (motors_dir_port_reg & 0xF0)
	| (1 << motors_RF_pin)
	| (1 << motors_LF_pin);
}

void timer_pwm_init(void) {
	TCCR5A_reg = (1 << COMA1_bit)
	| (0 << COMA0_bit)
	| (1 << COMB1_bit)
	| (0 << COMB0_bit)
	| (0 << WGM1_bit)
	| (1 << WGM0_bit);

	TCCR5B_reg = (0 << WGM3_bit)
	| (0 << WGM2_bit)
	| (0 << CS2_bit)
	| (1 << CS1_bit)
	| (1 << CS0_bit);

	TCNT5L_reg = 0x00;
	OCR5AL_reg = 0x00;
	OCR5BL_reg = 0x00;
}

void set_duty_cycle(unsigned char dcycle_pin_a, unsigned char dcycle_pin_b) {
	OCR5AL_reg = dcycle_pin_a;
	OCR5BL_reg = dcycle_pin_b;
}

int main(void) {
	unsigned char duty_cycle = 0;

	pwm_pin_config();
	timer_pwm_init();
	motors_pin_config();
	motors_move_forward();

	while(1) {
		set_duty_cycle(255 - duty_cycle, duty_cycle);
		duty_cycle++;
		_delay_ms(10);
	}

	return 0;
}

