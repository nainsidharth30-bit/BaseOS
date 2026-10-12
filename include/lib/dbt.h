#ifndef DBT_H
#define DBT_H
#ifndef MAX_RESERVED_REGIONS
#define MAX_RESERVED_REGIONS 16  /* Maximum number of scattered reserved regions */
#endif
#include<stdint.h>
#define MAX_INTERRUPT_CELLS 5
#define MAX_DEVICES 32
#define MAX_IC_DEVICES 10
struct reserved_region {
    uint64_t start;
    uint64_t end;
};

    extern  uint16_t ssd_index_array [20];
  extern  uint8_t ssd_index_array_count ; 

  extern  uint16_t ic_index_array[20];
  extern  uint8_t ic_index_array_count ;

 struct device_entry{
    uint32_t phandle ;
    char compatible[30];
        uint64_t mmio_base_address;
    uint64_t mmio_size ;
    uint32_t irq_cells[MAX_INTERRUPT_CELLS];
    uint32_t irq_cells_count;
    /*
    Stores the address of the ic_entry block to which this device is connected ! Exaample --> SSD controller's device_entry node stores the address of the gic controller (or any connected IC) address ! 
    */
    uint64_t interrupt_controller_phandle;  
 };

 /*
 ARCHITECTURAL DECISION -->  Because IC can have more than one mmio base and mmio size ( like gic have gicc and gicd) , hence we will store the mmio base and mmio size in consecutive blocks of this array ! Means at index 0 we have mmio base of IC1 and at index 1 we have mmio size of IC1 ! 
 */

 struct ic_entry {
    uint32_t phandle ; 
     char ic_compatible[30];
     uint64_t ic_mmio_base_size_array[6]; 
     /*
      Stores the number of mmiobase and mmiosize pairs ! 
     */
     uint8_t number_of_mmio_base_size ; 
 };

struct hardware_info {
    uint64_t ram_base_address;
    uint64_t ram_size;

    
    /* Array to hold scattered reserved regions */
    struct reserved_region rsv_regions[MAX_RESERVED_REGIONS];
    int rsv_count; /* Number of valid entries found */

    struct device_entry devices[MAX_DEVICES];
    uint32_t device_count ;   // it works more like a index rather than size , for size do device_count+1 

    struct ic_entry ic_devices[MAX_IC_DEVICES];
    uint8_t number_of_ic_devices ; 


     };

  extern struct  hardware_info g_hw;

  int extracting_dbt_info(uintptr_t dbt_tree_ptr , struct hardware_info *out_info ) ;

#endif