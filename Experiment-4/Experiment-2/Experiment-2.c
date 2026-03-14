
#include "firebird_simulation.h"
#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "lcd.h"
#include "uart.h"

// Global variable written by ISR — must be volatile
volatile unsigned int adc_10bit_data;

// Reference voltage: AVcc = 5.000 V = 5000 mV
#define VREF_MV     5000UL


/*
 * Function Name: sharp_sensor_pin_config
 * Input: None
 * Output: None
 * Logic: Configures Sharp sensor analog pin as INPUT with no pull-up
 * Example Call: sharp_sensor_pin_config();
 */
void sharp_sensor_pin_config(void)
{
    sharp_sensor_ddr_reg  &= ~(1 << sharp_sensor_pin);
    sharp_sensor_port_reg &= ~(1 << sharp_sensor_pin);
}


/*
 * Function Name: adc_init
 * Input: None
 * Output: None
 * Logic: Initializes ADC in interrupt-driven 10-bit right-adjust mode.
 *   ADEN=1  : Enable ADC
 *   ADIE=1  : Enable ADC conversion-complete interrupt
 *   ADPS2=1, ADPS1=1, ADPS0=0 : Prescaler /64
 *   REFS0=1 : AVcc reference
 *   ADLAR=0 : Right-adjust (10-bit result in ADCL + ADCH)
 *   ACD=1   : Disable analog comparator
 * Example Call: adc_init();
 */
void adc_init(void)
{
    cli();

    ADCSRA_reg = (1 << ADEN_bit)
               | (1 << ADIE_bit)
               | (1 << ADPS2_bit)
               | (1 << ADPS1_bit);

    ADCSRB_reg = 0x00;

    ADMUX_reg  =  (1 << REFS0_bit);
    ADMUX_reg &= ~(1 << ADLAR_bit);

    ACSR_reg   =  (1 << ACD_bit);

    sei();
}


/*
 * Function Name: select_adc_channel
 * Input: channel_num => ADC channel (0–15)
 * Output: None
 * Logic: Sets MUX5 in ADCSRB for channels > 7, writes lower bits into ADMUX
 * Example Call: select_adc_channel(10);
 */
void select_adc_channel(unsigned char channel_num)
{
    if (channel_num > 7)
    {
        ADCSRB_reg |=  (1 << MUX5_bit);
        channel_num -= 8;
    }
    else
    {
        ADCSRB_reg &= ~(1 << MUX5_bit);
    }
    ADMUX_reg = (ADMUX_reg & 0xE0) | (channel_num & 0x1F);
}


/*
 * Function Name: start_adc
 * Input: None
 * Output: None
 * Logic: Sets ADSC bit to start ADC conversion. ISR fires on completion.
 * Example Call: start_adc();
 */
void start_adc(void)
{
    ADCSRA_reg |= (1 << ADSC_bit);
}


/*
 * Function Name: reset_adc_config_registers
 * Input: None
 * Output: None
 * Logic: Clears MUX5 and channel bits only — does NOT disable ADC
 * Example Call: reset_adc_config_registers();
 */
void reset_adc_config_registers(void)
{
    ADCSRB_reg &= ~(1 << MUX5_bit);
    ADMUX_reg  &= ~(0x1F);
}


/*
 * ADC Interrupt Service Routine
 * Fires when ADC conversion completes.
 * ADCL must be read first — this locks ADCH until both are read.
 */
ISR(ADC_vect)
{
    unsigned char low  = ADCL;                              // Read ADCL first (locks ADCH)
    unsigned char high = ADCH;                              // Then read ADCH
    adc_10bit_data = low | ((unsigned int)high << 8);       // Combine into 10-bit result
}


/*
 * Function Name: convert_analog_channel_data
 * Input: sensor_channel_number => ADC channel to read
 * Output: None
 * Logic: Selects channel and triggers conversion. Result stored via ISR.
 * Example Call: convert_analog_channel_data(sharp_sensor_channel);
 */
void convert_analog_channel_data(unsigned char sensor_channel_number)
{
    select_adc_channel(sensor_channel_number);
    start_adc();
}


/*
 * Main Function
 *
 * LCD Layout:
 *   Row 1: "Raw: XXXX"
 *   Row 2: "Vol: X.XXX V"
 *
 * UART Output (one line per sample):
 *   "Raw: XXXX   Vol: X.XXX V\r\n"
 */
int main(void)
{
    sharp_sensor_pin_config();
    adc_init();
    lcd_port_config();
    lcd_init();
    uart_init(UBRR_VALUE);

    int          sharp_sensor_data;
    unsigned int voltage_mv;
    unsigned int volts;
    unsigned int millivolts;

    char tx_raw_adc_data_buffer[25];
    char tx_voltage_buffer[25];                             // FIX: now used for UART voltage output
    char lcd_print_raw_adc_data_string[25];
    char lcd_print_voltage_string[25];                      // FIX: now used for LCD voltage display

    // Trigger first conversion
    convert_analog_channel_data(sharp_sensor_channel);

    while (1)
    {
        // FIX: Wait for ADSC to be CLEAR (= conversion complete), not != 0x40
        // ADSC is cleared by hardware when conversion finishes
        if ((ADCSRA_reg & (1 << ADSC_bit)) == 0x00)
        {
            sharp_sensor_data = (int)adc_10bit_data;

            // --- Voltage calculation ---
            // voltage_mv = (raw * 5000) / 1023
            voltage_mv = (unsigned int)(((unsigned long)sharp_sensor_data * VREF_MV) / 1023UL);
            volts      = voltage_mv / 1000;
            millivolts = voltage_mv % 1000;

            // --- LCD display ---
            sprintf(lcd_print_raw_adc_data_string, "Raw: %04d", sharp_sensor_data);
            lcd_string(1, 1, lcd_print_raw_adc_data_string);

            sprintf(lcd_print_voltage_string, "Vol: %u.%03u V ", volts, millivolts);
            lcd_string(2, 1, lcd_print_voltage_string);     // FIX: lcd_print_voltage_string now used

            // --- UART output ---
            sprintf(tx_raw_adc_data_buffer, "Raw: %04d\t", sharp_sensor_data);
            uart_tx_string(tx_raw_adc_data_buffer);

            sprintf(tx_voltage_buffer, "Vol: %u.%03u V\r\n", volts, millivolts);
            uart_tx_string(tx_voltage_buffer);              // FIX: tx_voltage_buffer now used

            // Trigger next conversion
            convert_analog_channel_data(sharp_sensor_channel);

            _delay_ms(100);                                 // ~10 Hz refresh rate
        }
    }

    return 0;
}
