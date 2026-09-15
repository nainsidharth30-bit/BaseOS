#include "../../include/mkMAU/mkMAU.h"
#include<stddef.h>
#include<stdint.h>

extern void array_node_shift_end(struct mkmau_node* memory_tracker_array, size_t shift_units, size_t from_which_node, size_t array_size);
 struct mkmau_node break_the_block_update_memory_tracker_array(int iterator  , uintptr_t bytes_to_allocate , int* tracker_array_size) ;

struct mkmau_node memory_allocator(uintptr_t bytes_to_allocate)
{
    extern struct mkmau_node * memory_tracker_array ;
    extern int memory_tracker_array_size ;
    

    int iterator = 0 ;
    while(iterator<memory_tracker_array_size)
    {
        struct mkmau_node current_node = memory_tracker_array[iterator] ;
        if(current_node.if_free==1 && current_node.end_range-current_node.base_range >= bytes_to_allocate)
        {
            if(current_node.end_range-current_node.base_range >= bytes_to_allocate)
            {
                   memory_tracker_array[iterator].if_free=0;
                   return memory_tracker_array[iterator] ;
            }

     struct mkmau_node node = break_the_block_update_memory_tracker_array(iterator,bytes_to_allocate,&memory_tracker_array_size);
     return node ;

        }

        iterator++;
    }


}

    struct mkmau_node break_the_block_update_memory_tracker_array(int iterator  , uintptr_t bytes_to_allocate , int* tracker_array_size)
    {
               extern struct mkmau_node * memory_tracker_array ;
    extern int memory_tracker_array_size ;
    
    uint64_t start_address_first_block = memory_tracker_array[iterator].base_range ;
    uint64_t end_address_first_block = start_address_first_block+bytes_to_allocate ;

    uint64_t start_address_second_block = end_address_first_block;
    uint64_t end_address_second_block = memory_tracker_array[iterator].end_range ;

    struct mkmau_node node_1  ;
    struct mkmau_node node_2 ;
    node_1.base_range=start_address_first_block;
    node_1.end_range=end_address_first_block;
    node_1.if_free=0;

    
    node_2.base_range=start_address_second_block;
    node_2.end_range=end_address_second_block;
    node_2.if_free=1;

    memory_tracker_array[iterator]=node_1;
    array_node_shift_end(memory_tracker_array,1,iterator,*tracker_array_size);
    memory_tracker_array[iterator+1]=node_2;

  *tracker_array_size = *tracker_array_size+1 ;

  return node_1;
    
    }


    void reserved_region_ranges_size(struct hardware_info* out_info , uint64_t * size_of_reserved_regions )
{
    *size_of_reserved_regions =0;
     
    for(int i=0 ; i<out_info->rsv_count ; i++)
    {
      
         *size_of_reserved_regions=*size_of_reserved_regions+(out_info->rsv_regions[i].end - out_info->rsv_regions[i].start);

    }
}

    
