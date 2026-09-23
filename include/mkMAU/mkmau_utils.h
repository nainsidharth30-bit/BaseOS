#ifndef MKMAU_UTILS_H

#define MKMAU_UTILS_H
#include<stdint.h>

void reserved_region_ranges_size(struct hardware_info* out_info , uint64_t * size_of_reserved_regions ) ;
uint32_t mkmau_merge_array_rsv_regions(struct hardware_info* out_info , struct reserved_region merged_array[] ) ;
struct mkmau_node memory_allocator(uintptr_t bytes_to_allocate) ;

#endif