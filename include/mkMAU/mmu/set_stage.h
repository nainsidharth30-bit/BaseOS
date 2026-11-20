#ifndef SET_STAGE
#define SET_STAGE

#include<stdint.h>
#include"../../lib/dbt.h"
#include"root_pte.h"


uint32_t set_stage(uintptr_t bitmap , uint32_t size_bitmap , struct hardware_info* out_info , struct root_pte_entry * root_pte_array , uint32_t root_pte_array_count  ) ; 


#endif