#include<stdint.h>
#include "../../include/interrupts/ic_bpt.h"
#include "../../include/interrupts/arm_cortex_a15_gic.h"
#include"../../include/driverHeaders/uart.h"
int arm_cortex_a15_gic(struct ic_bpt * bpt)
{
    uart_puts("\n I am arm cortex gic \n");
    return 1 ;
}