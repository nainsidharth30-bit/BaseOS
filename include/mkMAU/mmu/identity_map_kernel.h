#ifndef IDENTITY_MAP_KERNEL_H
#define IDENTITY_MAP_KERNEL_H 

#include<stdint.h>
#include"../../lib/dbt.h"
#include"root_pte.h"



void identity_map_kernel(struct hardware_info * out_info , uintptr_t bitmap , uint32_t size_bitmap , struct root_pte_entry * root_pte_array) ; 


#endif