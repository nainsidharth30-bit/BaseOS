#include "../../include/mkMAU/mkMAU.h"
#include<stddef.h>
#include<stdint.h>
#include "../../include/driverHeaders/uart.h"
uint32_t mkmau_merge_array_rsv_regions(struct hardware_info* out_info , struct reserved_region* merged_array ) ;
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


uint32_t mkmau_merge_array_rsv_regions(struct hardware_info* out_info , struct reserved_region merged_array[] )
{
        uint32_t iterator = 0 ;
        uint32_t merged_array_count = -1 ;

    // uart_puts("I am alive");

        while(iterator<out_info->rsv_count-1)
        {
                //  uart_puthex(iterator);
                //  uart_puts("\n");
            struct reserved_region node_1 = out_info->rsv_regions[iterator] ;
            struct reserved_region node_2 = out_info->rsv_regions[iterator+1];


            //   if(merged_array_count!=-1)
            //   {
            //     uart_puts("ifififif");
            //     if( node_1.end < merged_array[merged_array_count].end || node_1.start>merged_array[merged_array_count].start)
            //     {
            //         // if(node_1.end < merged_array[merged_array_count].end && node_1.start>merged_array[merged_array_count].start)
            //         // {
            //         //     iterator++;
            //         //     continue;
            //         // }
            //         uart_puts("\n Hello I am here , ket see \n");
                   
            //         struct reserved_region another_node ;
            //         if(node_1.end < merged_array[merged_array_count].end&&node_1.start<=merged_array[merged_array_count].start)
            //         {    uart_puts("\n FFFFFFFFFF \n");
            //              another_node.start=node_1.start;
            //              another_node.end=merged_array[merged_array_count].end;
            //         }

            //         if(node_1.start>merged_array[merged_array_count].start&&node_1.end<=merged_array[merged_array_count].end)
            //         {   uart_puts("\n GGGGGGGGGG \n");
            //              another_node.start=merged_array[merged_array_count].start;
            //              another_node.end = node_1.end ;
            //         }

                  
            //         merged_array[merged_array_count]=another_node;
                   
                    


            //     }
            //   }


            if(node_1.end<=node_2.start)  // if regions are placed adjacent or with some gap 
            //
             {    
                // uart_puts("I am pushing something in array");
                 merged_array_count++;
                //  uart_puthex(merged_array_count);
                 merged_array[merged_array_count]=node_1 ;
                //  uart_puts("Done");
            }
            else if(node_1.start<node_2.start&&node_1.end<node_2.end)
            // 
            {   
            //  uart_puts("I am pushimergen array");
                struct reserved_region another_node ;
                another_node.end=node_2.end ;
                another_node.start=node_1.start ;
                merged_array_count++;
                 merged_array[merged_array_count]=another_node ;
               iterator++ ;
            }
            else{
                //  uart_puts("I amyahoooooooooray");
                merged_array_count++;
                 merged_array[merged_array_count]=node_1 ;
                 iterator++;
            }
            // uart_puts("Incrementing");
           iterator++;  
        }




        // Handling the last region 


          // checking if last region is already merged or not 

       if (out_info->rsv_regions[out_info->rsv_count - 1].start <= merged_array[merged_array_count].end &&
    out_info->rsv_regions[out_info->rsv_count - 1].end >= merged_array[merged_array_count].start)
{
    // Overlap. Extend the merged region if the last region goes further.
    if (out_info->rsv_regions[out_info->rsv_count - 1].end > merged_array[merged_array_count].end)
    {
        merged_array[merged_array_count].end = out_info->rsv_regions[out_info->rsv_count - 1].end;
    }
}
else
{
    merged_array_count++;
    merged_array[merged_array_count] = out_info->rsv_regions[out_info->rsv_count - 1];
}


// int i=0 ;
// while(i<=merged_array_count)
// {
    
//     uart_puts("start = ");
// uart_puthex(merged_array[i].start);
// uart_puts("  end = ");
// uart_puthex(merged_array[i].end);
// uart_puts("\n");
//     i++ ;
// }








return merged_array_count ;

}

    
