#include<stdint.h>
#include "uart.h"
 uintptr_t get_sp(uintptr_t sp) ;
void track_stack()
{
    extern char _stack_bottom[];
    extern char _stack_top[];

    uintptr_t stack_top = (uintptr_t)_stack_top ;
    uintptr_t stack_bottom = (uintptr_t)_stack_bottom;

    uintptr_t sp  = 0;
    sp=get_sp(sp);

    uart_puts("\n===== STACK STATUS =====\n");
    uart_puts("top      = "); uart_puthex(stack_top);    uart_puts("\n");
    uart_puts("bottom   = "); uart_puthex(stack_bottom); uart_puts("\n");
    uart_puts("sp       = "); uart_puthex(sp);     uart_puts("\n");
    uart_puts("size     = "); uart_puthex(stack_top - stack_bottom); uart_puts("\n");
    uart_puts("used     = "); uart_puthex(stack_top - sp);     uart_puts("\n");
    uart_puts("free     = "); uart_puthex(sp - stack_bottom);  uart_puts("\n");
    uart_puts("========================\n");
    
}

 uintptr_t get_sp(uintptr_t sp)
{
       # if defined(__aarch64__)
       {
         __asm__ volatile ("mov %0, sp" : "=r"(sp));
       }  

       #elif defined(__arm__)
    {
    __asm__ volatile ("mov %0, sp" : "=r"(sp));
       }

       #elif defined(__riscv)
    {
        __asm__ volatile ("mv %0, sp" : "=r"(sp));
    }

    #elif defined(__mips__)
   {
     __asm__ volatile ("move %0, $sp" : "=r"(sp));
    }

    #endif

    return sp;

}