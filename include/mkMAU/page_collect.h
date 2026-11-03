#ifndef PAGE_COLLECT_H
#define PAGE_COLLECT_H 

#include<stdint.h>
#include<stddef.h>

uint32_t page_collect(uint64_t start , uint64_t end , uint32_t* base_page_number , uint32_t alignment_value , uint64_t ram_base , uint64_t ram_end)   ; 



#endif