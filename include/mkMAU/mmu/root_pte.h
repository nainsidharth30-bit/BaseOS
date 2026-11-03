#ifndef ROOT_PTE_H

#define ROOT_PTE_H

#include <stdint.h>
#include"../mkMAU.h"
#include"../../lib/dbt.h"

struct root_pte_entry{
    uint64_t page_index ;  // Starts from zero ! 
   uint32_t pid ; 
} ; 

uint16_t making_root_pte_array() ; 

#endif