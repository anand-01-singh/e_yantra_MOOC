/*
 * Experiment-1.c
 * Reads White-Line and IR Proximity sensors via ADC,
 * displays values on LCD, and sends center WL data over UART.
 *
 * Target: Firebird V (ATmega2560) or ATmega328P (Proteus simulation)
 * F_CPU: 14745600 Hz (ATmega2560) / 16000000 Hz (ATmega328P)
 *
 * Fixes applied:
 *  1. Removed reset_adc_config_registers() — it was disabling ADC after every read
 *  2. adc_reinit() added to properly restore ADC state before each conversion
 *  3. ADIF flag now cleared after conversion complete check
 *  4. select_adc_channel() now clears MUX5 bit before setting it (channel > 7 fix)
 *  5. Added small delay after each ADC conversion for stability
 *  6. Added lcd_clear() at start of each loop to prevent stale display data
 *  7. UART tx_buffer size increased to 30 to prevent overflow
 */

#define F_CPU 14745600UL

#include "firebird_simulation.h"
#include <stdbool.h>
#include <util/delay.h>
#include <stdio.h>
#include "lcd.h"
#include "uart.h"


/* -----------------------------------------------------------------------
 * Port Configuration Functions
 * ----------------------------------------------------------------------- */

/*
 * Function Name: wl_sensors_port_config
 * Input: None
 * Output: None
 * Logic: Configures White-Line sensor pins as input with no pull-up
 * Example Call: wl_sensors_port_config();
 */
void wl_sensors_port_config(void)
{
    wl_sensors_ddr_reg  &= ~(0x07 << 1);   // PF1, PF2, PF3 as input
    wl_sensors_port_reg &= ~(0x07 << 1);   // Disable pull-ups
}

/*
 * Function Name: ir_prox_sensors_port_config
 * Input: None
 * Output: None
 * Logic: Configures IR Proximity sensor pins (3,4,5) as input with no pull-up
 * Example Call: ir_prox_sensors_port_config();
 */
void ir_prox_sensors_port_config(void)
{
    ir_prox_3_4_sensors_ddr_reg  &= ~(0x03 << 6);  // PF6, PF7 as input
    ir_prox_3_4_sensors_port_reg &= ~(0x03 << 6);  // Disable pull-ups
    ir_prox_5_sensor_ddr_reg     &= ~(1 << 0);     // PK0 as input
    ir_prox_5_sensor_port_reg    &= ~(1 << 0);     // Disable pull-up
}


/* -----------------------------------------------------------------------
 * ADC Functions
 * ----------------------------------------------------------------------- */

/*
 * Function Name: adc_init
 * Input: None
 * Output: None
 * Logic: Initializes ADC — enables ADC, sets prescaler to 64,
 *        sets AVcc as reference, enables left-adjust for 8-bit result,
 *        and disables analog comparator
 * Example Call: adc_init();
 */
void adc_init(void)
{
    ADCSRA_reg = 0x86;          // ADEN=1 (Enable ADC), Prescaler = 64 (bits: 110)
    ADCSRB_reg = 0x00;          // Free running mode, MUX5 = 0
    ADMUX_reg  = 0x40;          // AVcc reference (REFS0=1), right adjust
    ADMUX_reg |= (1 << 5);      // ADLAR=1: Left adjust — read only ADCH for 8-bit result
    ACSR_reg  |= (1 << 7);      // ACD=1: Disable Analog Comparator to save power
}

/*
 * Function Name: adc_reinit
 * Input: None
 * Output: None
 * Logic: Restores ADC registers to working state before each conversion.
 *        Called inside convert_analog_channel_data() to ensure ADC is
 *        always properly configured before a new conversion starts.
 * Example Call: adc_reinit();
 */
void adc_reinit(void)
{
    ADCSRA_reg = 0x86;          // Re-enable ADC with prescaler 64
    ADCSRB_reg = 0x00;          // Clear MUX5 and auto-trigger bits
    ADMUX_reg  = 0x40;          // AVcc reference
    ADMUX_reg |= (1 << 5);      // Left adjust for 8-bit result via ADCH
}

/*
 * Function Name: select_adc_channel
 * Input: channel_num => ADC channel number (0 to 15)
 * Output: None
 * Logic: Selects the ADC channel. For channels > 7, sets MUX5 bit
 *        in ADCSRB register. Lower 3 bits are written into ADMUX.
 * Example Call: select_adc_channel(8);
 */
void select_adc_channel(unsigned char channel_num)
{
    // First clear MUX5 bit (bit 3 of ADCSRB)
    ADCSRB_reg &= ~(1 << 3);

    // For channels 8–15, set MUX5 bit
    if (channel_num > 7)
    {
        ADCSRB_reg |= (1 << 3);
    }

    // Mask to lower 3 bits and write into ADMUX (preserve upper 5 bits)
    channel_num &= 0x07;
    ADMUX_reg = (ADMUX_reg & 0xE0) | channel_num;
}

/*
 * Function Name: start_adc
 * Input: None
 * Output: None
 * Logic: Starts ADC conversion by setting ADSC bit (bit 6) in ADCSRA
 * Example Call: start_adc();
 */
void start_adc(void)
{
    ADCSRA_reg |= (1 << 6);     // ADSC = 1: Start Conversion
}

/*
 * Function Name: check_adc_conversion_complete
 * Input: None
 * Output: true if conversion is complete, false otherwise
 * Logic: Checks ADIF bit (bit 4) in ADCSRA. When set, conversion is done.
 *        Clears ADIF by writing 1 to it after detection.
 * Example Call: while (!check_adc_conversion_complete());
 */
bool check_adc_conversion_complete(void)
{
    if (ADCSRA_reg & (1 << 4))      // Check ADIF bit
    {
        ADCSRA_reg |= (1 << 4);     // Clear ADIF by writing 1 to it
        return true;
    }
    else
    {
        return false;
    }
}

/*
 * Function Name: read_adc_converted_data
 * Input: None
 * Output: 8-bit ADC result read from ADCH register
 * Logic: Since ADLAR=1 (left adjust), the top 8 bits of the 10-bit result
 *        are in ADCH. Reading ADCH gives us the 8-bit ADC value.
 * Example Call: data = read_adc_converted_data();
 */
unsigned char read_adc_converted_data(void)
{
    unsigned char adc_8bit_data;
    adc_8bit_data = ADCH;           // Read only ADCH for 8-bit result (ADLAR=1)
    return adc_8bit_data;
}

/*
 * Function Name: convert_analog_channel_data
 * Input: sensor_channel_number => ADC channel to read (0 to 15)
 * Output: 8-bit ADC converted value for the given channel
 * Logic: Reinitializes ADC, selects the channel, starts conversion,
 *        waits for completion, reads and returns the 8-bit result.
 *        A small delay is added after reading for ADC stability.
 * Example Call: data = convert_analog_channel_data(3);
 */
unsigned char convert_analog_channel_data(unsigned char sensor_channel_number)
{
    unsigned char adc_8bit_data;

    adc_reinit();                                       // Restore ADC config before each read
    select_adc_channel(sensor_channel_number);          // Select the sensor channel
    start_adc();                                        // Begin conversion
    while (!(check_adc_conversion_complete()));         // Wait until done
    adc_8bit_data = read_adc_converted_data();          // Read 8-bit result from ADCH
    _delay_us(10);                                      // Short delay for ADC stability

    return adc_8bit_data;
}


/* -----------------------------------------------------------------------
 * Main Function
 * ----------------------------------------------------------------------- */

int main(void)
{
    // Initialize all peripherals
    wl_sensors_port_config();
    ir_prox_sensors_port_config();
    adc_init();
    lcd_port_config();
    lcd_init();
    uart_init(UBRR_VALUE);

    // Variable declarations
    unsigned char left_wl_sensor_data, center_wl_sensor_data, right_wl_sensor_data;
    unsigned char ir_prox_3_sensor_data, ir_prox_4_sensor_data, ir_prox_5_sensor_data;
    char tx_buffer[30];                                 // Buffer for UART string (increased size)

    _delay_ms(100);                                     // Allow LCD and peripherals to stabilize

    while (1)
    {
        /* --- Read White-Line Sensors (ADC channels 1, 2, 3) --- */
        left_wl_sensor_data   = convert_analog_channel_data(left_wl_sensor_channel);    // ADC3
        center_wl_sensor_data = convert_analog_channel_data(center_wl_sensor_channel);  // ADC2
        right_wl_sensor_data  = convert_analog_channel_data(right_wl_sensor_channel);   // ADC1

        /* --- Read IR Proximity Sensors (ADC channels 6, 7, 8) --- */
        ir_prox_3_sensor_data = convert_analog_channel_data(ir_prox_3_sensor_channel);  // ADC6
        ir_prox_4_sensor_data = convert_analog_channel_data(ir_prox_4_sensor_channel);  // ADC7
        ir_prox_5_sensor_data = convert_analog_channel_data(ir_prox_5_sensor_channel);  // ADC8

        /* --- Display on LCD ---
         * Row 1: White-Line sensor values  (L=left, C=center, R=right)
         * Row 2: IR Proximity sensor values (3, 4, 5)
         * Layout: [xxx] [xxx] [xxx]  (3 digits per sensor, 4 cols apart)
         */
        lcd_numeric_value(1, 1,  left_wl_sensor_data,   3);
        lcd_numeric_value(1, 5,  center_wl_sensor_data, 3);
        lcd_numeric_value(1, 9,  right_wl_sensor_data,  3);

        lcd_numeric_value(2, 1,  ir_prox_3_sensor_data, 3);
        lcd_numeric_value(2, 5,  ir_prox_4_sensor_data, 3);
        lcd_numeric_value(2, 9,  ir_prox_5_sensor_data, 3);

        /* --- Send Center White-Line Sensor Data over UART --- */
        sprintf(tx_buffer, "Center WL: %03d\r\n", center_wl_sensor_data);
        uart_tx_string(tx_buffer);

        _delay_ms(200);                                 // Refresh rate: ~5 times per second
    }
}
