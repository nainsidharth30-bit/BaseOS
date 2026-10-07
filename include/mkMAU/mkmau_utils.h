#ifndef MKMAU_UTILS_H

#define MKMAU_UTILS_H
#include<stdint.h>

void reserved_region_ranges_size(struct hardware_info* out_info , uint64_t * size_of_reserved_regions ) ;

/* 
This function tries to merged those resevred regions which are either partially overlapping , or are adjacent to each other or are fully embeddeed inside each other ! 
*/
uint32_t mkmau_merge_array_rsv_regions(struct reserved_region temporary_array[] , int temporary_array_size  , struct reserved_region merged_array[] ) ;



struct mkmau_node memory_allocator(uintptr_t bytes_to_allocate) ;

#endif