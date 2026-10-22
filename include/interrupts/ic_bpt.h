#ifndef IC_BPT_H
#define IC_BPT_H
#include<stdint.h>
#define MAX_PAIRS 6
#define MAX_CELLS 5 

struct ic_bpt{
    /* This array stores the mmio base and mmio size pairs in the order (mmio base , mmio size ), 
    Also which bank the pairs belongs to is answed by the order , like if gicd comes first in gic than first (mmio base , mmio size ) pair corresponsed to gicd
    */
    uint64_t mmio_base_mmio_size_pair_array[MAX_PAIRS] ; 
    uint32_t total_number_of_elemnets ; // Its is total number of elemnets not pairs 
    uint32_t irq_cells[MAX_CELLS];
    uint32_t number_of_irq_cells ; 
    uint32_t operation_code ; // 0 --> Return  , 1 -- > Enable the IRQ Line 
    uint32_t priority_value ; 
} ; 

#endif