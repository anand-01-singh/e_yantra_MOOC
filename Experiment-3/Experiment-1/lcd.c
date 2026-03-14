/*
 * lcd.c
 * Created: 10/07/2018 10:47:03
 * Author: e-Yantra Team
 *
 * Fixes applied:
 *  1. lcd_wr_command() and lcd_wr_char() — data port write now uses
 *     safe bit-masking that does NOT corrupt RS/RW/EN control pins
 *     when data and control pins share the same PORT (e.g., PORTC on ATmega2560)
 *  2. lcd_numeric_value() — fixed flag variable declared as unsigned char (was global)
 *  3. All global intermediate variables (temp, unit, tens, etc.) kept as-is but
 *     made static to avoid linker conflicts
 *  4. lcd_wr_char() RS pin comment corrected (was wrongly labelled "Command")
 *  5. lcd_string() column increment fixed to use column++ consistently
 */

#include "firebird_simulation.h"
#include <util/delay.h>
#include "lcd.h"

#define sbit( reg, bit )    reg |=  ( 1 << bit )   // Set a bit in a register
#define cbit( reg, bit )    reg &= ~( 1 << bit )   // Clear a bit in a register

// Static intermediate variables used inside lcd_numeric_value()
static unsigned int temp;
static unsigned int unit;
static unsigned int tens;
static unsigned int hundred;
static unsigned int thousand;
static unsigned int million;


/*
 * Function Name: lcd_port_config
 * Input: None
 * Output: None
 * Logic: Configures LCD data and control pins as output, sets them all to logic 0
 * Example Call: lcd_port_config();
 */
void lcd_port_config(void)
{
    // Set LCD control pins (RS, RW, EN) as output
    lcd_control_ddr_reg |= ( (1 << RS_pin) | (1 << RW_pin) | (1 << EN_pin) );

    // Set LCD data pins (DB4–DB7) as output
    lcd_data_ddr_reg    |= ( (1 << DB4_pin) | (1 << DB5_pin) | (1 << DB6_pin) | (1 << DB7_pin) );

    // Clear LCD control pins to logic 0
    lcd_control_port_reg &= ~( (1 << RS_pin) | (1 << RW_pin) | (1 << EN_pin) );

    // Clear LCD data pins to logic 0
    lcd_data_port_reg    &= ~( (1 << DB4_pin) | (1 << DB5_pin) | (1 << DB6_pin) | (1 << DB7_pin) );
}


/*
 * Function Name: lcd_write_nibble
 * Input: nibble => upper 4 bits to send (already shifted to DB7..DB4 positions)
 *        rs     => 0 for command, 1 for data
 * Output: None
 * Logic: Sends one nibble to LCD safely without disturbing control pins.
 *        Clears only the DB4–DB7 bits in the data port, then ORs in the nibble.
 *        This prevents corrupting RS/RW/EN pins when they share the same PORT.
 * Example Call: lcd_write_nibble(0x40, 0);
 */
static void lcd_write_nibble(unsigned char nibble, unsigned char rs)
{
    // Safely clear only DB4–DB7 bits (leave RS/RW/EN and other bits untouched)
    lcd_data_port_reg &= ~( (1 << DB4_pin) | (1 << DB5_pin) | (1 << DB6_pin) | (1 << DB7_pin) );

    // Write nibble bits onto DB4–DB7 pins
    // nibble is expected already in the upper 4 bits (bits 7:4)
    if (nibble & 0x80) sbit(lcd_data_port_reg, DB7_pin);
    if (nibble & 0x40) sbit(lcd_data_port_reg, DB6_pin);
    if (nibble & 0x20) sbit(lcd_data_port_reg, DB5_pin);
    if (nibble & 0x10) sbit(lcd_data_port_reg, DB4_pin);

    // Set RS based on command (0) or data (1)
    if (rs)
        sbit(lcd_control_port_reg, RS_pin);
    else
        cbit(lcd_control_port_reg, RS_pin);

    cbit(lcd_control_port_reg, RW_pin);             // RW = 0: Writing to LCD

    // Pulse EN pin to latch data
    sbit(lcd_control_port_reg, EN_pin);
    _delay_ms(2);
    cbit(lcd_control_port_reg, EN_pin);
    _delay_ms(2);
}


/*
 * Function Name: lcd_set_4bit
 * Input: None
 * Output: None
 * Logic: Initializes LCD into 4-bit mode using the standard 3-step
 *        initialization sequence as per HD44780 datasheet.
 * Example Call: lcd_set_4bit();
 */
void lcd_set_4bit(void)
{
    _delay_ms(15);                                  // Wait for LCD power-on

    // Step 1: Send 0x3 three times (8-bit mode reset sequence)
    lcd_write_nibble(0x30, 0);
    _delay_ms(5);

    lcd_write_nibble(0x30, 0);
    _delay_ms(1);

    lcd_write_nibble(0x30, 0);
    _delay_ms(1);

    // Step 2: Switch to 4-bit mode
    lcd_write_nibble(0x20, 0);
    _delay_ms(1);
}


/*
 * Function Name: lcd_wr_command
 * Input: cmd => 8-bit command byte for LCD
 * Output: None
 * Logic: Sends a command to LCD in two 4-bit nibbles (upper nibble first).
 *        RS=0 indicates command mode.
 * Example Call: lcd_wr_command(0x01);  // Clear display
 */
void lcd_wr_command(unsigned char cmd)
{
    lcd_write_nibble(cmd & 0xF0, 0);           // Send upper nibble, RS=0 (command)
    lcd_write_nibble((cmd << 4) & 0xF0, 0);   // Send lower nibble, RS=0 (command)
}


/*
 * Function Name: lcd_init
 * Input: None
 * Output: None
 * Logic: Initializes LCD in 4-bit mode with 2 lines, clears display,
 *        sets entry mode, turns on display and cursor.
 * Example Call: lcd_init();
 */
void lcd_init(void)
{
    lcd_set_4bit();
    _delay_ms(5);
    lcd_wr_command(0x28);                       // 4-bit mode, 2 lines, 5x8 dots
    _delay_ms(1);
    lcd_wr_command(0x01);                       // Clear display
    _delay_ms(2);
    lcd_wr_command(0x06);                       // Entry mode: increment, no shift
    _delay_ms(1);
    lcd_wr_command(0x0C);                       // Display ON, cursor OFF, blink OFF
    _delay_ms(1);
    lcd_wr_command(0x80);                       // Set cursor to home position (row1, col1)
    _delay_ms(1);
}


/*
 * Function Name: lcd_home
 * Input: None
 * Output: None
 * Logic: Moves LCD cursor to home position (row 1, column 1)
 * Example Call: lcd_home();
 */
void lcd_home(void)
{
    lcd_wr_command(0x80);
    _delay_ms(2);
}


/*
 * Function Name: lcd_cursor
 * Input: row    => LCD row number (1 or 2)
 *        column => LCD column number (1 to 16)
 * Output: None
 * Logic: Sets LCD cursor to the specified (row, column) position
 *        by computing and sending the correct DDRAM address command.
 *
 *   Row 1 DDRAM address: 0x00 to 0x0F  → command = 0x80 + (column-1)
 *   Row 2 DDRAM address: 0x40 to 0x4F  → command = 0xC0 + (column-1)
 *
 * Example Call: lcd_cursor(2, 5);
 */
void lcd_cursor(char row, char column)
{
    switch (row)
    {
        case 1: lcd_wr_command(0x80 + column - 1); break;
        case 2: lcd_wr_command(0xC0 + column - 1); break;
        case 3: lcd_wr_command(0x94 + column - 1); break;
        case 4: lcd_wr_command(0xD4 + column - 1); break;
        default: break;
    }
    _delay_us(50);
}


/*
 * Function Name: lcd_clear
 * Input: None
 * Output: None
 * Logic: Clears the LCD display by sending the clear display command (0x01)
 * Example Call: lcd_clear();
 */
void lcd_clear(void)
{
    lcd_wr_command(0x01);
    _delay_ms(2);                               // Clear command needs extra time
}


/*
 * Function Name: lcd_wr_char
 * Input: row           => LCD row (1 or 2)
 *        column        => LCD column (1 to 16)
 *        alpha_num_char => character to display
 * Output: None
 * Logic: Positions cursor and writes one character to LCD in 4-bit mode.
 *        RS=1 indicates data mode (character write).
 * Example Call: lcd_wr_char(1, 3, 'A');
 */
void lcd_wr_char(char row, char column, char alpha_num_char)
{
    lcd_cursor(row, column);
    lcd_write_nibble( (unsigned char)(alpha_num_char) & 0xF0,        1);  // Upper nibble, RS=1 (data)
    lcd_write_nibble(((unsigned char)(alpha_num_char) << 4) & 0xF0,  1);  // Lower nibble, RS=1 (data)
}


/*
 * Function Name: lcd_string
 * Input: row    => LCD row (1 or 2)
 *        column => starting column position
 *        *str   => pointer to null-terminated string to display
 * Output: None
 * Logic: Writes each character of the string to LCD starting at (row, column),
 *        advancing column after each character until null terminator is reached.
 * Example Call: lcd_string(1, 1, "Hello!");
 */
void lcd_string(char row, char column, char *str)
{
    while (*str != '\0')
    {
        lcd_wr_char(row, column, *str);
        str++;
        column++;
    }
}


/*
 * Function Name: lcd_numeric_value
 * Input: row    => LCD row (1 or 2)
 *        column => starting column position
 *        val    => integer value to display (supports negative)
 *        digits => number of digits to display (1 to 5)
 * Output: None
 * Logic: Displays an integer value on LCD at (row, column) with the specified
 *        number of digits. Handles negative values by displaying '-' prefix.
 *        Supports up to 5 digits. Shows 'E' if digits > 5.
 * Example Call: lcd_numeric_value(1, 1, 255, 3);
 */
void lcd_numeric_value(char row, char column, int val, int digits)
{
    // Handle negative values
    if (val < 0)
    {
        val = -val;
        lcd_wr_char(row, column, '-');
        column++;
    }

    // Position cursor at target location
    if (row == 0 || column == 0)
        lcd_home();
    else
        lcd_cursor(row, column);

    unsigned char flag = 0;

    if (digits == 5 || flag == 1)
    {
        million = (val / 10000) + 48;
        lcd_wr_char(row, column, (char)million);
        column++;
        flag = 1;
    }

    if (digits == 4 || flag == 1)
    {
        temp = val / 1000;
        thousand = (temp % 10) + 48;
        lcd_wr_char(row, column, (char)thousand);
        column++;
        flag = 1;
    }

    if (digits == 3 || flag == 1)
    {
        temp = val / 100;
        hundred = (temp % 10) + 48;
        lcd_wr_char(row, column, (char)hundred);
        column++;
        flag = 1;
    }

    if (digits == 2 || flag == 1)
    {
        temp = val / 10;
        tens = (temp % 10) + 48;
        lcd_wr_char(row, column, (char)tens);
        column++;
        flag = 1;
    }

    if (digits == 1 || flag == 1)
    {
        unit = (val % 10) + 48;
        lcd_wr_char(row, column, (char)unit);
        column++;
    }

    if (digits > 5)
    {
        lcd_wr_char(row, column, 'E');          // Error: unsupported digit count
    }
}
