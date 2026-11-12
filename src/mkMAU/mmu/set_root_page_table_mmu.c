
#include"../../../include/mkMAU/mmu/set_root_page_table_mmu.h"
#include<stdint.h>

void set_root_page_table_mmu(uintptr_t address) 
{
    // Give the cpu the address of the root page table ! 
    __asm__ volatile("msr ttbr0_el1 , %0" : : "r"((uint64_t)address)) ; 

    __asm__ volatile("dsb sy") ; 
    __asm__ volatile("isb") ; 

    // Throw away anything the cpu remembered about the old root ! 
    __asm__ volatile("tlbi vmalle1") ; 
    __asm__ volatile("dsb sy") ; 
    __asm__ volatile("isb") ; 
}