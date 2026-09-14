// uart.h - PL011 UART Function Declarations
#include<stdint.h>
#ifndef UART_H
#define UART_H

// Function prototypes
void uart_putc(char c);
void uart_puts(const char *str);
void uart_puthex(uintptr_t val);

#endif // UART_H