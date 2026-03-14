/*
 * uart.c
 * Created: 10/07/2018 10:47:03
 * Author: e-Yantra Team
 *
 * Fixes applied:
 *  1. uart_tx() now waits for UDRE (Data Register Empty) flag before writing
 *     to UDR — the original had this check commented out, causing dropped bytes
 *     when sending strings rapidly (e.g., in a fast while loop).
 *  2. uart_tx_string() unchanged — correctly iterates until null terminator.
 */

#include "firebird_simulation.h"
#include <avr/interrupt.h>
#include "uart.h"

#define sbit( reg, bit )    reg |=  ( 1 << bit )   // Set a bit in a register
#define cbit( reg, bit )    reg &= ~( 1 << bit )   // Clear a bit in a register


/*
 * Function Name: uart_init
 * Input: ubbr_value => pre-calculated UBRR baud rate value
 *                      Use the UBRR_VALUE macro from firebird_simulation.h
 * Output: None
 * Logic: Initializes UART with:
 *        - Baud rate set via UBRRH and UBRRL registers
 *        - Asynchronous USART mode (UMSEL1:0 = 00)
 *        - 8-bit character size (UCSZ1:0 = 11, UCSZ2 = 0)
 *        - No parity, 1 stop bit
 *        - TX and RX enabled
 * Example Call: uart_init(UBRR_VALUE);
 */
void uart_init(unsigned int ubbr_value)
{
    // Disable all UART interrupts and TX/RX before configuring
    UCSRB_reg = 0x00;

    // Set baud rate registers (high byte first, then low byte)
    UBRRH_reg = (unsigned char)(ubbr_value >> 8);
    UBRRL_reg = (unsigned char)(ubbr_value);

    // Configure UCSRC: Asynchronous mode, no parity, 1 stop bit, 8-bit data
    UCSRC_reg &= ~( (1 << UMSEL1_bit) | (1 << UMSEL0_bit) );  // Async USART mode
    UCSRC_reg |=  ( (1 << UCSZ1_bit)  | (1 << UCSZ0_bit) );   // 8-bit character size

    // Enable TX and RX; ensure 9-bit mode is disabled (UCSZ2 = 0)
    UCSRB_reg |=  ( (1 << TXEN_bit) | (1 << RXEN_bit) );
    UCSRB_reg &= ~( 1 << UCSZ2_bit );
}


/*
 * Function Name: uart_tx
 * Input: data => single character to transmit
 * Output: None
 * Logic: Waits until the UART Data Register is empty (UDRE flag set),
 *        then writes the character to UDR for transmission.
 *        The wait is REQUIRED to prevent overwriting data before it is sent.
 * Example Call: uart_tx('A');
 */
void uart_tx(char data)
{
    // Wait until UDRE (Data Register Empty) flag is set — safe to write
    while ( (UCSRA_reg & (1 << UDRE_bit)) == 0x00 );

    UDR_reg = data;                             // Write character to UART data register
}


/*
 * Function Name: uart_tx_string
 * Input: *data => pointer to null-terminated string to transmit
 * Output: None
 * Logic: Sends each character of the string one by one using uart_tx()
 *        until the null terminator '\0' is reached.
 * Example Call: uart_tx_string("Hello\r\n");
 */
void uart_tx_string(char *data)
{
    while (*data != '\0')
    {
        uart_tx(*data);
        data++;
    }
}
