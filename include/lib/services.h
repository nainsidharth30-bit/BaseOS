#ifndef KERNEL_H
#define KERNEL_H
#include "../driverHeaders/uart.h"

struct kernel_services{
  
    void (*uart_puts)(const char *str);

};


#endif