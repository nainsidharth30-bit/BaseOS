#include "../../include/driverHeaders/uart.h"

__attribute__((weak))

int extracting_dbt_info(uintptr_t dbt_tree_ptr , struct hardware_info *out_info )
{
    uart_puts("Hello , I am weak dtb ");
    return 1 ;
}