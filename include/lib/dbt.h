#ifndef DBT_H
#define DBT_H
#ifndef MAX_RESERVED_REGIONS
#define MAX_RESERVED_REGIONS 16  /* Maximum number of scattered reserved regions */
#endif
#include<stdint.h>
#define MAX_INTERRUPT_CELLS 5
#define MAX_DEVICES 32
struct reserved_region {
    uint64_t start;
    uint64_t end;
};

 struct device_entry{
    uint32_t phandle ;
    char compatible[70];
        uint64_t mmio_base_address;
    uint64_t mmio_size ;
        uint32_t irq_cells[MAX_INTERRUPT_CELLS];
    uint32_t irq_cells_count;
    uint32_t interrupt_controller_phandle;

 };

struct hardware_info {
    uint64_t ram_base_address;
    uint64_t ram_size;

    
    /* Array to hold scattered reserved regions */
    struct reserved_region rsv_regions[MAX_RESERVED_REGIONS];
    int rsv_count; /* Number of valid entries found */

    struct device_entry devices[MAX_DEVICES];
    uint32_t device_count;


     };

  extern struct  hardware_info g_hw;

  int extracting_dbt_info(uintptr_t dbt_tree_ptr , struct hardware_info *out_info ) ;

#endif