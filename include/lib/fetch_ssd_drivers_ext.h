#ifndef FETCH_EXT_DRIVERS_H 
#define FETCH_EXT_DRIVERS_H 

#include "dbt.h"


void fetch_ssd_drivers_extensions(struct hardware_info * out_info , uint16_t* ssd_index_array , uint8_t ssd_index_array_count , uintptr_t bitmap , uint32_t size_bitmap   ) ; 

struct meta_ext_array_entry{

uint64_t base_address ; 
uint32_t size  ; 
uint32_t uniq_id  ; 

} ; 


#endif