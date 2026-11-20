#include"../../../include/lib/meta_ext.h"
#include"../../../include/lib/k_drivers.h"
#include"../../../include/driverHeaders/uart.h"
#include"../../../include/mkMAU/mmu/root_pte.h"
#include"../../../include/mkMAU/mmu/set_stage.h"
#include"../../../include/lib/dbt.h"
#include"../../../include/lib/memcpy.h"
#include"../../../include/lib/setzero.h"
#include"../../../include/driverHeaders/peripheral_struct.h"
#include"../../../include/mkMAU/mkmau_utils.h"

#define K_DRIVER_ARRAY_SIZE 5


void set_sector_in_bpt(struct peripheral_request_bpt* bpt , uint64_t sector) ; 


#include<stdint.h>
#include<stddef.h>

void load_extension(struct meta_ext* mte , struct root_pte_entry* root_pte_array , uint32_t root_pte_array_count , uintptr_t bitmap , uint32_t size_bitmap , struct hardware_info* out_info ,  uint16_t* ssd_index_array , uint8_t ssd_index_array_count  )
{

    /*
    Step --> 1 
    Before loading any extension  , we must set the stage for it ! 
    */

        root_pte_array_count =  set_stage(bitmap , size_bitmap , out_info , root_pte_array , root_pte_array_count) ; 

    /*
    Step --> 2 
    Now , from the mte , we will extract the uniq_id , get the proper device and its connected k-driver , form the peripheral_request_bpt and fetch the extension properly ! 
    */    

    uint32_t uniq_id = mte->uniq_id ; 
    struct device_entry* device = NULL ; 

    for(int i =0 ; i<ssd_index_array_count ; i++)
    {
        if(out_info->devices[ssd_index_array[i]].uniq_id == uniq_id)
        {
              device = &out_info->devices[ssd_index_array[i]] ; 
              break;
        }
    }

    /*
    Step --> 3 
    We got the correct ssd device , now its time to create the peripheral_request bpt 
    */

    struct peripheral_request_bpt bpt ; 
    
    set_zero(&bpt , sizeof(struct peripheral_request_bpt)) ; 

    memcopy(bpt.mmio_base_size_pair_array , device->mmio_base_size_array , sizeof(bpt.mmio_base_size_pair_array)) ; 
    bpt.number_of_elements_mbspa = device->mmio_base_size_array_count ; 

     // Now we will iterate over the k-drivers array and will get the quuee and buffer data for our bpt 

        extern struct k_driver_entry k_driver_array[K_DRIVER_ARRAY_SIZE] ;
   extern uint8_t k_driver_array_count ;

     for(int i =0 ; i<k_driver_array_count ; i++)
     {
        if(k_driver_array[i].uniq_id == device->uniq_id)
        { 
            bpt.size_of_buffer = k_driver_array[i].buffer_size ; 
            bpt.size_of_queue = k_driver_array[i].queue_size ; 
            bpt.size_of_baa =1 ; 
            bpt.size_of_qaa = 1 ;
            bpt.queues_address_array[0] = k_driver_array[i].queue_base_address ; 
            bpt.buffer_addresses_array[0] = k_driver_array[i].buffer_base_address ; 
            break;
        }
     }
    
     bpt.operation_data_array[0] = 2 ; // Means its a read operation ! 
     uint64_t sector = mte->sector_number ; 

     uint32_t total_sectors_to_read = mte->size_of_extension/bpt.size_of_buffer ; // size of buffer is equal to size of one sector !  

     /*
     Now , our extension can may span more than one sector  , so it is our duty or work to read buffers properly and put the thing there ! 
     */

     /*
     Step --> 4 
     Now we have extension size , we must alloocate space or pages for that extension to load that extension inside those pages ! 
     We will do it this way , we use two variables , one tells number of pages allocated till now ( we are not allocating all the pages in one go) , another tells how many bytes we had loaded , so extension_size - bytes_loaded tells how many bytes are still needed to load , we will call the function or k-driver , extract some sector and from buffer copy that in proper page and will allocate more page on the go ! 
     */

     uint32_t number_of_pages_allocated = 0 ; 
    
     uint64_t bytes_loaded = 0 ; 

     uint32_t current_page = -1 ; 

    



       

        total_sectors_to_read-- ; 
     }

   


void set_sector_in_bpt(struct peripheral_request_bpt* bpt , uint64_t sector)
{
   for(int b = 0 ; b < 8 ; b++)
   {
      bpt->operation_data_array[1 + b] = (uint8_t)((sector >> (8 * b)) & 0xFF) ; 
   }
}





