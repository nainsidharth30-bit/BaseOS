#include<stdint.h>
#include "../../include/interrupts/ic_bpt.h"
#include "../../include/interrupts/arm_pl190_vic.h"
#include"../../include/driverHeaders/uart.h"
int arm_pl190_vic(struct ic_bpt * bpt)
{
     uart_puts("\n I am arm cortex vic \n");
    return 1;
}