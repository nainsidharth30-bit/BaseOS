#ifndef ROOT_PTE_H

#define ROOT_PTE_H

#include <stdint.h>

#include"../../lib/dbt.h"

struct root_pte_entry{
    uint64_t page_index ;  // Starts from zero ! 
   uint32_t pid ; 
} ; 

void root_pte_array_setup(struct hardware_info* out_info , uintptr_t bitmap , uint32_t size_bitmap ) ; 
void root_pte_set_kernel(uintptr_t bitmap , uint32_t size_bitmap ) ; 

#endif