#ifndef MKMAU_H
#define MKMAU_H
#include <stdint.h>
#include "../lib/dbt.h"
#include <stddef.h>

#define MMAU_PAGE_SIZE 4096ULL

  


void mkMAU_main(struct hardware_info * out_info , uintptr_t x1_register );


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

struct mkmau_node{
    uint64_t base_range ;
    uint64_t end_range;
    node_flag_t  if_free ;
    size_t owner_pid ;
};




#endif