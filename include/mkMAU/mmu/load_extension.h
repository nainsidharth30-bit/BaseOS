#ifndef LOAD_EXTENSION_H
#define LOAD_EXTENSION_H



#include<stdint.h>
#include"../../lib/meta_ext.h"
#include"../../lib/dbt.h"



void load_extension(struct meta_ext* mte , struct root_pte_entry* root_pte_array , uint32_t root_pte_array_count , uintptr_t bitmap , uint32_t size_bitmap , struct hardware_info* out_info ,  uint16_t* ssd_index_array , uint8_t ssd_index_array_count  ) ; 



#endif