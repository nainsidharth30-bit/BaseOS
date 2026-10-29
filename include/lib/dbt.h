#ifndef DBT_H
#define DBT_H
#ifndef MAX_RESERVED_REGIONS
#define MAX_RESERVED_REGIONS 16  /* Maximum number of scattered reserved regions */
#endif
#include<stdint.h>
#include"../driverHeaders/peripheral_struct.h"
#include"../interrupts/ic_bpt.h"
#define MAX_INTERRUPT_CELLS 5
#define MAX_DEVICES 32
#define MAX_IC_DEVICES 10
struct reserved_region {
    uint64_t start;
    uint64_t end;
};


 struct device_entry{
    uint32_t phandle ;
    char compatible[100];
    uint64_t interrupt_controller_phandle;
      uint64_t mmio_base_size_array[6]; 
      uint8_t mmio_base_size_array_count ; 
    uint32_t irq_cells[MAX_INTERRUPT_CELLS];
    uint32_t irq_cells_count;
    uint32_t irq_priority ; 
    uint32_t queue_num ; 
    uint32_t uniq_id ; 
    int (*ic_code)(struct ic_bpt* bpt);
     int (*peripheral_code)(struct peripheral_request_bpt* bpt);
 };

 struct ic_compatible_entry {
    const char *compatible;
    int (*ic_code)(struct ic_bpt *);
};

struct ssd_compatible_entry{
   const char* compatible ; 
   int (*peripheral_code)(struct peripheral_request_bpt* bpt);
};



struct hardware_info {
    uint64_t ram_base_address;
    uint64_t ram_size;

    
    /* Array to hold scattered reserved regions */
    struct reserved_region rsv_regions[MAX_RESERVED_REGIONS];
    int rsv_count; /* Number of valid entries found */

    struct device_entry devices[MAX_DEVICES];
    uint32_t device_count ;   // it works more like a index rather than size , for size do device_count+1 

    uint32_t mem_sec ;  // 0 --> None  , 1 --> MPU , 2--> MMU ! 

    uint32_t ssd_controller_extensions_uniq_id_array[10] ; 
    uint32_t ssd_controller_extensions_mapped_sector_array[25] ; 

     };

  extern struct  hardware_info g_hw;

  int extracting_dbt_info(uintptr_t dbt_tree_ptr , struct hardware_info *out_info ) ;

#endif