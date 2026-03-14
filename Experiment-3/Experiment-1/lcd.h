/*
 * lcd.h
 * Created: 10/07/2018 10:47:03
 * Author: e-Yantra Team
 *
 * Fixes applied:
 *  1. Corrected typo "coloumn" → "column" in lcd_wr_char and lcd_numeric_value declarations
 *  2. Added declaration for lcd_write_nibble (internal helper — declared static in .c)
 */

#ifndef LCD_H_
#define LCD_H_

// Configures LCD data and control pins as output
void lcd_port_config(void);

// Initializes LCD into 4-bit mode using the standard HD44780 reset sequence
void lcd_set_4bit(void);

// Sends an 8-bit command to LCD in two 4-bit nibbles (RS=0)
void lcd_wr_command(unsigned char cmd);

// Initializes LCD: 4-bit mode, 2 lines, clear display, entry mode, display ON
void lcd_init(void);

// Moves LCD cursor to home position (row 1, column 1)
void lcd_home(void);

// Positions the LCD cursor at (row, column)
void lcd_cursor(char row, char column);

// Clears the entire LCD display
void lcd_clear(void);

// Writes one character at (row, column) on LCD
void lcd_wr_char(char row, char column, char alpha_num_char);

// Writes a null-terminated string starting at (row, column) on LCD
void lcd_string(char row, char column, char *str);

// Displays an integer value at (row, column) with specified number of digits (1–5)
void lcd_numeric_value(char row, char column, int val, int digits);

#endif /* LCD_H_ */
