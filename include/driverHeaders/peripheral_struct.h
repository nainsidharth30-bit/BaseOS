#ifndef PERIPHERAL_STRUCT_H
#define PERIPHERAL_STRUCT_H

#include <stdint.h>


struct peripheral_request_bpt {
     uintptr_t mmio_base_size_pair_array[6] ;  // mbspa
     uint32_t number_of_elements_mbspa ; 
     uintptr_t queues_address_array[5] ; //qaa
     uint32_t size_of_qaa ; 
     uintptr_t buffer_addresses_array[5] ; //baa
     uint32_t size_of_baa ; 
  
     uint64_t operation_data_array[20] ; 
     
};





#endif