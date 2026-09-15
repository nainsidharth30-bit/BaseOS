#ifndef DBT_H
#define DBT_H
#ifndef MAX_RESERVED_REGIONS
#define MAX_RESERVED_REGIONS 16  /* Maximum number of scattered reserved regions */
#endif
#include<stdint.h>

struct reserved_region {
    uint64_t start;
    uint64_t end;
};

struct hardware_info {
    uint64_t ram_base_address;
    uint64_t ram_size;
  
    
    /* Array to hold scattered reserved regions */
    struct reserved_region rsv_regions[MAX_RESERVED_REGIONS];
    int rsv_count; /* Number of valid entries found */
     uintptr_t ssd_controller_base_address ;
     uint64_t ssd_size ;
     uint32_t ssd_blocks_size ;
     uint32_t ssd_irq ;

     };

  extern struct  hardware_info g_hw;

  int extracting_dbt_info(uintptr_t dbt_tree_ptr , struct hardware_info *out_info ) ;

#endif