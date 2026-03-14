#define F_CPU 14745600UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>

//Bargraph LED Pin Configuration
void bar_graph_led_pins_config(void)
{
	DDRJ |= 0xA2;     // PJ1, PJ5, PJ7 as output
	PORTJ |= 0x02;    // LED2 ON initially (PJ1)
}

//Interrupt Switch Configuration (PE7)
void interrupt_sw_pin_config(void)
{
	DDRE &= 0x7F;     // PE7 as input
	PORTE |= 0x80;    // Enable internal pull-up
}

//Check if Switch Pressed
bool interrupt_switch_pressed(void)
{
	if((PINE & 0x80) == 0)   // Active LOW
		return true;
	else
		return false;
}

//Turn ON specific LED
void turn_on_bar_graph_led(unsigned char led_pin)
{
	PORTJ |= (1 << led_pin);
}

//Turn OFF specific LED
void turn_off_bar_graph_led(unsigned char led_pin)
{
	PORTJ &= ~(1 << led_pin);
}

//Main Function (DO NOT MODIFY)
int main(void)
{
	bar_graph_led_pins_config();
	interrupt_sw_pin_config();
	
	turn_on_bar_graph_led(5);   // LED6 ON (PJ5)

	while (1)
	{
		if (interrupt_switch_pressed())
		{
			turn_off_bar_graph_led(1); // LED2 OFF
			turn_on_bar_graph_led(7);  // LED8 ON
			
			_delay_ms(50);
		}
		else
		{
			turn_on_bar_graph_led(1);  // LED2 ON
			turn_off_bar_graph_led(7); // LED8 OFF
		}
	}
}