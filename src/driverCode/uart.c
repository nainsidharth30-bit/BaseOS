#include "../../include/driverHeaders/uart.h"
#include<stdint.h>
// UART Data Register
#define UART0_DR ((volatile unsigned int *)0x09000000)

// 0x09000000 ---> Address in QEMU for uart register 

// 1. Sending  a single character to hardware
void uart_putc(char c) {
    // Write character byte directly to the hardware register
    *UART0_DR = (unsigned int)c;
}

// 2. Send a null-terminated string
void uart_puts(const char *str) {
    while (*str) {
        // If we hit a newline character, issue a carriage return for clean line alignment
        if (*str == '\n') {
            uart_putc('\r');
        }
        uart_putc(*str++);
    }
}

#include "../../include/driverHeaders/uart.h"

// Convert and print a 64-bit number as a hexadecimal string
void uart_puthex(uintptr_t val) {
    uart_puts("0x"); // Print hex prefix

    // A 64-bit number has 16 hex digits (4 bits each = 64 bits)
    for (int i = 60; i >= 0; i -= 4) {
        uint8_t nibble = (val >> i) & 0xF; // Extract 4 bits at a time
        
        if (nibble < 10) {
            uart_putc('0' + nibble);        // Convert 0-9 to ASCII '0'-'9'
        } else {
            uart_putc('A' + (nibble - 10)); // Convert 10-15 to ASCII 'A'-'F'
        }
    }
}




