#ifndef K_DRIVERS_H
#define K_DRIVERS_H


#include "dbt.h"
#include<stdint.h>


void set_k_drivers(uint16_t* ssd_index_array , uint8_t ssd_index_array_count , struct hardware_info* out_info , uintptr_t bitmap , uint32_t size_bitmap) ; 


struct k_driver_entry{
   uint64_t queue_base_address  ; 
   uint32_t queue_size ; 
   uint64_t buffer_base_address ; 
   uint32_t buffer_size ; 

   uint32_t uniq_id ; 
} ;



#endif