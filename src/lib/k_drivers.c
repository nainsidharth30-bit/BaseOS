
#include <stdint.h>
#include "../../include/lib/dbt.h"
#include "../../include/lib/setzero.h"
#include "../../include/mkMAU/mkmau_utils.h"
#include "../../include/lib/memcpy.h"
#include "../../include/driverHeaders/uart.h"
#include "../../include/driverHeaders/peripheral_struct.h"
#include "../../include/lib/k_drivers.h"
#include<stddef.h>

#define K_DRIVER_ARRAY_SIZE 5 

uint8_t allocate_memory_for_queue_and_buffer(struct peripheral_request_bpt* bpt , struct hardware_info* out_info , uintptr_t bitmap , uint32_t size_bitmap ) ;

void set_k_drivers(uint16_t* ssd_index_array , uint8_t ssd_index_array_count , struct hardware_info* out_info , uintptr_t bitmap , uint32_t size_bitmap)
{
   extern struct k_driver_entry k_driver_array[K_DRIVER_ARRAY_SIZE] ;
   extern uint8_t k_driver_array_count ;

      struct device_entry* device ; 

      struct peripheral_request_bpt bpt ; 
      set_zero(&bpt , sizeof(struct peripheral_request_bpt)) ; 
      
      for(int i=0 ; i<ssd_index_array_count;i++)
      {
           device = &out_info->devices[ssd_index_array[i]] ; 
            bpt.operation_data_array[0] = 0 ; // Getting the Buffer Size   ; 
            device->peripheral_code(&bpt) ; 
            bpt.operation_data_array[0] = 1 ; // Getting Queue size this time !
            device->peripheral_code(&bpt) ; 

            // Now allocating space for the Queue and Buffer based on their sizes ! 

            if(!allocate_memory_for_queue_and_buffer(&bpt , out_info , bitmap , size_bitmap))
            {
               uart_puts("\n No memory for the queue and buffer of the ssd k-driver ! ") ; 
               return ; 
            }

            // Now we abslutely can fill the k_driver_array ! 

            k_driver_array[k_driver_array_count].buffer_base_address = bpt.buffer_addresses_array[0] ; 
            k_driver_array[k_driver_array_count].queue_base_address = bpt.queues_address_array[0] ; 
            k_driver_array[k_driver_array_count].buffer_size = bpt.size_of_buffer ; 
            k_driver_array[k_driver_array_count].queue_size = bpt.size_of_queue ; 
            k_driver_array[k_driver_array_count].uniq_id = device->uniq_id ; 

            k_driver_array_count++ ; 
      }
}


// Returns 1 when the queue and the buffer got their memory , 0 when there is no memory left ! 
uint8_t allocate_memory_for_queue_and_buffer(struct peripheral_request_bpt* bpt , struct hardware_info* out_info , uintptr_t bitmap , uint32_t size_bitmap )
{
   uint32_t size_needed = bpt->size_of_buffer + bpt->size_of_queue ; 

   // Now we have to allocate enough pages for this ! 

   uint32_t pages_needed = (size_needed + out_info->min_mem_allocation - 1) / out_info->min_mem_allocation ; 

   uint16_t next_pages = 0 ; // It is number of next pages from the first page ! 

   uint32_t first_page = free_contiguous_pages_wrapper(&next_pages , pages_needed , bitmap , size_bitmap) ; 

   if(first_page == 0xFFFFFFFF)
   {
      return 0 ; 
   }

   bpt->buffer_addresses_array[0] = out_info->ram_base_address + (uint64_t)first_page * out_info->min_mem_allocation ; 
   bpt->size_of_baa = 1 ; 

   bpt->queues_address_array[0] = out_info->ram_base_address + (uint64_t)first_page * out_info->min_mem_allocation + bpt->size_of_buffer ; 
   bpt->size_of_qaa = 1 ; 

   return 1 ; 
}
