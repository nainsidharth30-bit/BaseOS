#include <stdint.h>
#include "../../include/lib/fetch_ssd_drivers_ext.h"
#include "../../include/lib/dbt.h"
#include "../../include/lib/setzero.h"
#include "../../include/mkMAU/mkmau_utils.h"
#include "../../include/lib/memcpy.h"
#include "../../include/driverHeaders/uart.h"

#define DELIMITER           0xFFFFFFFFu     // -1 in the dtb , ends one group and starts the next ! 
#define END_OF_BLOCK        0xFFFFFFFEu     // -2 in the dtb , the end of the whole property ! 
#define MAX_META_EXT_BYTES  0x100000        // 1 MB , stops a garbage size from asking for all of RAM ! 

#define META_EXT_ARRAY_SIZE 15 


/*
This array stores the three pairs which makes one single dataset information for one single meta ext array ! 
first slot stores the base address of that meta ext array , next one store size of that meta ext array and next one stores the uniq id of the ssd controller , so now its a pair of 3 actuyally ! 
*/

struct meta_ext_array_entry meta_ext_array_matrix[META_EXT_ARRAY_SIZE] ; 

uint8_t meta_ext_array_matrix_size =0 ;

uint8_t allocate_memory_for_queue_and_buffer(struct peripheral_request_bpt* bpt , struct hardware_info* out_info , uintptr_t bitmap , uint32_t size_bitmap ) ;

void allocate_space_for_meta_ext_array(uint64_t size_needed , struct hardware_info* out_info , uintptr_t bitmap , uint32_t size_bitmap , uint32_t uniq_id ) ; 

/* ---- helpers split out of the main loop ---- */
int     find_sector_index_for_uniq_id(struct hardware_info* out_info , int delimiter_count) ; 
uint8_t setup_queue_and_buffer(struct device_entry* device , struct peripheral_request_bpt* bpt , struct hardware_info* out_info , uintptr_t bitmap , uint32_t size_bitmap ) ; 
void    read_sector_into_buffer(struct device_entry* device , struct peripheral_request_bpt* bpt , uint64_t sector ) ; 
void    read_and_copy_meta_ext_array(struct device_entry* device , struct peripheral_request_bpt* bpt , uint64_t base_sector , uint64_t array_size , uint8_t* dest ) ; 
void    fetch_meta_ext_for_device(struct device_entry* device , struct peripheral_request_bpt* bpt , struct hardware_info* out_info , uintptr_t bitmap , uint32_t size_bitmap ) ; 





void fetch_ssd_drivers_extensions(struct hardware_info * out_info , uint16_t* ssd_index_array , uint8_t ssd_index_array_count , uintptr_t bitmap , uint32_t size_bitmap )
{

   uart_puts("\n Hello I am fetching SSD DRIVERS , k-drivers\n") ; 

  /*
  Will be used by everyone ! 
  */
   struct peripheral_request_bpt bpt ; 
   set_zero(&bpt , sizeof(struct peripheral_request_bpt)) ; 

   /*
   Iterating over all the ssd device entry ! 
   */
   for(int jk = 0 ; jk < ssd_index_array_count ; jk++)
   {
      struct device_entry* device = &out_info->devices[ssd_index_array[jk]] ; 
      fetch_meta_ext_for_device(device , &bpt , out_info , bitmap , size_bitmap) ; 
   }
}



/* 
Walks the mapped_sector_array by counting DELIMITER entries until delimiter_count is reached , 
and returns the index of the first sector in the matching block ! 
*/
int find_sector_index_for_uniq_id(struct hardware_info* out_info , int delimiter_count)
{
   int delimter_check_count = 0 ; 
   int sector_index = -1 ; 

   for(int j = 0 ; delimter_check_count != delimiter_count ; j++)
   {
      if(out_info->ssd_controller_extensions_mapped_sector_array[j] == DELIMITER)
      {
         delimter_check_count++ ; 
      }
      sector_index = j + 1 ; 
   }

   return sector_index ; 
}



/*
Asks the k-driver for buffer size ( op[0]=0 ) and queue size ( op[0]=1 ) , 
then allocates one contiguous region that holds both. 
Addresses land back in bpt->buffer_addresses_array[0] and bpt->queues_address_array[0] ! 
Returns 1 on success , 0 when there is no memory left ! 
*/
uint8_t setup_queue_and_buffer(struct device_entry* device , struct peripheral_request_bpt* bpt , struct hardware_info* out_info , uintptr_t bitmap , uint32_t size_bitmap )
{
   bpt->operation_data_array[0] = 0 ;  // we will get the buffer size 
   device->peripheral_code(bpt) ; 
   bpt->operation_data_array[0] = 1 ;  // we will get the queue size 
   device->peripheral_code(bpt) ; 

   return allocate_memory_for_queue_and_buffer(bpt , out_info , bitmap , size_bitmap) ; 
}



/*
Packs the sector number into operation_data_array[1..8] as little endian , 
then calls the k-driver. The read data ends up in bpt->buffer_addresses_array[0] ! 
operation_data_array[0] must already be 2 ( read ) from the caller ! 
*/
void read_sector_into_buffer(struct device_entry* device , struct peripheral_request_bpt* bpt , uint64_t sector )
{
   for(int b = 0 ; b < 8 ; b++)
   {
      bpt->operation_data_array[1 + b] = (uint8_t)((sector >> (8 * b)) & 0xFF) ; 
   }
   device->peripheral_code(bpt) ; 
}



/*
Copies the whole meta ext array from SSD into dest !
The base sector already sits in the buffer , so the first 8 bytes ( the size field ) 
are skipped , then every further sector is read and copied from byte 0 !
*/
void read_and_copy_meta_ext_array(struct device_entry* device , struct peripheral_request_bpt* bpt , uint64_t base_sector , uint64_t array_size , uint8_t* dest )
{
   uint8_t* src = (uint8_t*) bpt->buffer_addresses_array[0] ; 
   uint64_t copied = 0 ; 
   uint64_t sector = base_sector ; 
   uint32_t start = 8 ;   // base sector : the first 8 bytes are the size , skip them ! 

   while(copied < array_size)
   {
      for(uint32_t k = start ; k < bpt->size_of_buffer && copied < array_size ; k++)
      {
         dest[copied] = src[k] ; 
         copied++ ; 
      }

      start = 0 ;   // every next sector is copied from its first byte ! 
      sector++ ; 

      if(copied < array_size)   // more data is left , read the next sector ! 
      {
         read_sector_into_buffer(device , bpt , sector) ; 
      }
   }
}



/*
Per-device worker !
Walks the DTB uniq_id array with delimiter tracking , matches the current device's uniq id , 
sets up queue and buffer once , then reads the base sector , allocates the meta ext array , 
and copies all its bytes from SSD ! 
*/
void fetch_meta_ext_for_device(struct device_entry* device , struct peripheral_request_bpt* bpt , struct hardware_info* out_info , uintptr_t bitmap , uint32_t size_bitmap )
{
   uint32_t uniq_id = device->uniq_id ; 
   uart_puthex(uniq_id) ; 

   uint8_t meta_ext_flag = 0 ;  // Tells if the meta_ext array size is fetched 
   uint8_t queue_buffer_space_allocation_flag = 0 ;   // Tells if we got the Queue and Buffer sizes from the k-driver call so that that space can be allocated !

   int delimiter_count = 0 ;  // This count is being used to match the found uniq id to its mapped sectors ! 

   /*
   We get the uniq id of an ssd device entry , now we must iterate over the ssd_controller_extensions_uniq_id_array , get the matching uniq id and using delimter_count we also will get 
   correct mapped sectors which have the extensions data for that SSD ! 
   */
   for(int i = 0 ; out_info->ssd_controller_extensions_uniq_id_array[i] != END_OF_BLOCK ; i++)
   {
      if(out_info->ssd_controller_extensions_uniq_id_array[i] == uniq_id)
      {
         /*
         Uniq id is matched and now we absolutely can get the sectors mapped to it using the delimiter_count ! 
         */
         int sector_index = find_sector_index_for_uniq_id(out_info , delimiter_count) ; 

         /*
         Step --> 1 
         First , we need size of Queue and Buffer , it is by convention that only one Queue and one Buffer will be given to k-driver but we need their sizes afterall and also must allocate that much emmory for them !
         */
         if(!queue_buffer_space_allocation_flag)
         {
            if(!setup_queue_and_buffer(device , bpt , out_info , bitmap , size_bitmap))
            {
               uart_puts("\n No memory for the queue and buffer of the ssd k-driver ! ") ; 
               return ; 
            }
            /*
            Now the addresses of the Buffer and the Queue are now being stored inside the bpt queue and buffers array [0] index ! 
            */
            queue_buffer_space_allocation_flag = 1 ; 
         }

         /*
         Step --> 2
         Now we got the buffer and Queue space , now we will extact the base sector in our buffer , once extracted , than we alloacte space for meta-ext array and repreatedly fill that array with extensions meta data ! 
         */
         if(!meta_ext_flag)
         {
            memcopy(bpt->mmio_base_size_pair_array , device->mmio_base_size_array , sizeof(bpt->mmio_base_size_pair_array)) ; 
            bpt->operation_data_array[0] = 2 ;  // Means its a read operation ! 

            uint64_t base_sector = out_info->ssd_controller_extensions_mapped_sector_array[sector_index] ; 

            read_sector_into_buffer(device , bpt , base_sector) ; 

            /*
            Now our buffer holds the base sector bytes !
            Our next work is to get the first 8 bytes out of that sector and allocate that much space for the meta ext array !
            */
            uint64_t* meta_ext_array_size = (uint64_t*) bpt->buffer_addresses_array[0] ; 

            allocate_space_for_meta_ext_array(*meta_ext_array_size , out_info , bitmap , size_bitmap , uniq_id) ; 

            /*
            Now we have stored every meta ext array with its base location , its size and uniq id of ssd controller , our next work is to 
            fetch all the sectors present (not the base sector and copy all of them in proper meta ext array !) We already have base sector hence we will leave the first 8 bytes which contains size and copy the rest bytes  , no lets just copy those bytes also !
            */
            uint64_t array_size = *meta_ext_array_size ; 
            uint8_t* dest = (uint8_t*) meta_ext_array_matrix[meta_ext_array_matrix_size - 1].base_address ; 

            read_and_copy_meta_ext_array(device , bpt , base_sector , array_size , dest) ; 

            meta_ext_flag = 1 ; 
         }
      }
      else if(out_info->ssd_controller_extensions_uniq_id_array[i] == DELIMITER)
      {
         delimiter_count++ ; 
      }
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



void allocate_space_for_meta_ext_array(uint64_t size_needed , struct hardware_info* out_info , uintptr_t bitmap , uint32_t size_bitmap , uint32_t uniq_id )
{
   uint32_t pages_needed = (size_needed + out_info->min_mem_allocation - 1) / out_info->min_mem_allocation ; 

   uint16_t next_pages = 0 ; // It is number of next pages from the first page ! 

   uint32_t first_page = free_contiguous_pages_wrapper(&next_pages , pages_needed , bitmap , size_bitmap) ; 

   if(first_page == 0xFFFFFFFF)
   {
      return ; 
   }

   struct meta_ext_array_entry node ; 
   node.base_address = out_info->ram_base_address + first_page*out_info->min_mem_allocation ; 
   node.size = size_needed ; 
   node.uniq_id = uniq_id ; 

   meta_ext_array_matrix[meta_ext_array_matrix_size] = node ; 
   meta_ext_array_matrix_size++ ; 
}