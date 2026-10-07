#include "../../include/mkMAU/mkMAU.h"
#include<stddef.h>
#include<stdint.h>
#include "../../include/driverHeaders/uart.h"

uint32_t mkmau_merge_array_rsv_regions(struct reserved_region temporary_array[] ,
                                       int temporary_array_size ,
                                       struct reserved_region merged_array[] ) ;
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

/* 
This function tries to merged those resevred regions which are either partially overlapping , or are adjacent to each other or are fully embeddeed inside each other ! 
*/uint32_t mkmau_merge_array_rsv_regions(struct reserved_region temporary_array[] ,
                                       int temporary_array_size ,
                                       struct reserved_region merged_array[])
{
        uint32_t iterator = 0 ;
        uint32_t merged_array_count = 0 ;

        if(temporary_array_size == 0)
        {
                return 0 ;
        }

        /* Seed the output with the first region */
        merged_array[0] = temporary_array[0] ;

        for(iterator = 1 ; iterator < temporary_array_size ; iterator++)
        {
                struct reserved_region node = temporary_array[iterator] ;
                struct reserved_region* last = &merged_array[merged_array_count] ;

                /* If node starts at or before the end of the last merged region,
                   they overlap or are adjacent — extend the merged region. */
                if(node.start <= last->end)
                {
                        if(node.end > last->end)
                        {
                                last->end = node.end ;
                        }
                }
                else
                {
                        /* Real gap — start a new merged region */
                        merged_array_count++ ;
                        merged_array[merged_array_count] = node ;
                }
        }

        return merged_array_count ;
}

    
