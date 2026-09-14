#ifndef QUICKSORT_H
#define QUICKSORT_H

#include<stdint.h>
#include<stddef.h>
void array_sort(void * array_base_address , uintptr_t node_size_bytes ,int array_size , int (*compare_start_address) (const void*a ,const void *b ));

#endif