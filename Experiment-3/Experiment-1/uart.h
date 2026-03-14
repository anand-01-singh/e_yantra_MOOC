/*
 * uart.h
 * Created: 10/07/2018 10:47:03
 * Author: e-Yantra Team
 */

#ifndef UART_H_
#define UART_H_

// Initializes UART with baud rate, 8-bit data, no parity, 1 stop bit, TX+RX enabled
// Pass UBRR_VALUE macro as argument (defined in firebird_simulation.h)
void uart_init(unsigned int ubbr_value);

// Transmits a single character over UART (waits for Data Register Empty before writing)
void uart_tx(char data);

// Transmits a null-terminated string over UART character by character
void uart_tx_string(char *data);

#endif /* UART_H_ */
