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

     /*
      IN Case of SSD Controller --> First 8 bits tells if it is a read (1) or write(0) operation ! 
     
     */
  
   uint8_t   operation_data_array[20] ; 
   uint8_t setup_flag ; 
     
};





#endif