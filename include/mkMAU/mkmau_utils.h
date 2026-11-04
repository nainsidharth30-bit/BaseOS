#ifndef MKMAU_UTILS_H
#define MKMAU_UTILS_H

#include<stdint.h>
#include<stddef.h>
int compare_start_address(const void *a, const void *b) ; 
struct __attribute__((packed)) flattened_device_tree_header {
    uint32_t magic;            /* Offset 0x00 */
    uint32_t totalsize;        /* Offset 0x04 */
    uint32_t off_dt_struct;    /* Offset 0x08 */
    uint32_t off_dt_strings;   /* Offset 0x0C */
    uint32_t off_mem_rsvmap;   /* Offset 0x10 */
    uint32_t version;          /* Offset 0x14 */
    uint32_t last_comp_version;/* Offset 0x18 */
    uint32_t boot_cpuid_phys;  /* Offset 0x1C */
    uint32_t size_dt_strings;  /* Offset 0x20 */
    uint32_t size_dt_struct;   /* Offset 0x24 */
};

typedef enum{
    node_free = 1,
    node_in_use =0
}node_flag_t;


uint32_t allocate_page(uintptr_t bitmap , uint32_t size_bitmap , uint8_t bit_set_flag , uint32_t start_page ) ; 

uint32_t free_contiguous_pages_wrapper(uint16_t* next_pages  , uint32_t required_pages , uintptr_t bitmap , uint32_t size_bitmap ) ; 


/*
It is only used when we have to sort the rsv regions array ! 
*/
struct mkmau_node{
    uint64_t base_range ;
    uint64_t end_range;
};


#endif