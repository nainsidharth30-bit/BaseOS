#include<stdint.h>

#include"../../../include/mkMAU/mmu/mmu_on.h"

void turn_mmu_on()
{
    // Memory types : index 0 = normal memory ( non cacheable , caches stay off ) , index 1 = device memory ! 
    uint64_t mair = 0x44ULL ; 

    // Address size the cpu supports ! 
    uint64_t mmfr0 = 0 ; 
    __asm__ volatile("mrs %0 , id_aa64mmfr0_el1" : "=r"(mmfr0)) ; 

    // Translation settings : 48 bit virtual addresses , 4 KB pages , inner shareable , ttbr1 not used ! 
    uint64_t tcr = 0 ; 
    tcr |= (16ULL << 0) ; 
    tcr |= (3ULL << 12) ; 
    tcr |= (1ULL << 23) ; 
    tcr |= ((mmfr0 & 0xFULL) << 32) ; 

    __asm__ volatile("msr mair_el1 , %0" : : "r"(mair)) ; 
    __asm__ volatile("msr tcr_el1 , %0" : : "r"(tcr)) ; 

    __asm__ volatile("dsb sy") ; 
    __asm__ volatile("isb") ; 

    // Throw away anything the cpu remembered about old translations ! 
    __asm__ volatile("tlbi vmalle1") ; 
    __asm__ volatile("dsb sy") ; 
    __asm__ volatile("isb") ; 

    // Flip the MMU on bit , caches stay off ! 
    uint64_t sctlr = 0 ; 
    __asm__ volatile("mrs %0 , sctlr_el1" : "=r"(sctlr)) ; 
    sctlr |= 1ULL ; 
    __asm__ volatile("msr sctlr_el1 , %0" : : "r"(sctlr)) ; 
    __asm__ volatile("isb") ; 
}