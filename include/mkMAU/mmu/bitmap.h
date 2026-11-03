#ifndef BIT_MAP_H
#define BIT_MAP_H

#include<stdint.h>
#include"../../lib/dbt.h"


uint32_t make_bitmap( uint64_t ram_size_in_bytes , struct hardware_info * out_info  , uintptr_t* bitmap   ) ;


#endif
