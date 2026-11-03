#ifndef POPULATE_BITMAP_H
#define POPULATE_BITMAP_H
#include"../../lib/dbt.h"
#include<stdint.h>
#include<stddef.h>

void populate_bitmap(uintptr_t bitmap , uint32_t bitmap_size  , uint64_t* array_of_page_numbers  , uint32_t size_of_array_of_page_numbers  , uint8_t free_or_occupied )  ; 

#endif