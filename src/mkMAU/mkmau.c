#include <stdint.h>
#include "../../include/driverHeaders/uart.h"
#include "../../include/mkMAU/mkMAU.h"
#include "../../include/lib/dbt.h"
#include "../../include/mkMAU/mobilemkMAU.h"
#include "../../include/mkMAU/mkmau_utils.h"
#include "../../include/driverHeaders/track_stack.h"

#include"stddef.h"
#define MMAU_SIZE 4096

 struct mkmau_node *memory_tracker_array = NULL;
 int memory_tracker_array_size ;

 extern  void array_node_shift_end (struct mkmau_node* memory_tracker_array , size_t shift_units , size_t from_which_node ,size_t array_size );
 int mkMAU_initial_tracker_array_populating(uintptr_t kernel_end_address , struct hardware_info *out_info , struct mkmau_node* memory_tracker_array ,uintptr_t end_range_tracker_array  );
 void mkMAU_find_free_range_for_range_nodes_array(uintptr_t kernel_end_address ,uint32_t merge_array_count , struct reserved_region *merged_array , uintptr_t number_of_maximum_nodes ,uintptr_t* free_range_start_address,uintptr_t* free_range_end_address, struct hardware_info *out_info ) ;
   uint32_t check_that_range_with_kernel (uintptr_t start_address , uintptr_t end_address , uintptr_t needed_size_for_array , uintptr_t kernel_start , uintptr_t kernel_end ,uintptr_t * free_range_end_address , uintptr_t* free_range_start_address);

void mkMAU_main(struct hardware_info * out_info , uintptr_t x1_register)  
{
    uart_puts("I am in mkmau_main");
       extern char _kernel_start[];
    extern char _kernel_end[];     
    
    uintptr_t kernel_start = (uintptr_t)_kernel_start ;
    uintptr_t kernel_end = (uintptr_t)_kernel_end ;
    size_t kernel_size = kernel_end - kernel_start;
     uint64_t size_of_reserved_regions = 0 ;  
      reserved_region_ranges_size(out_info ,&size_of_reserved_regions);
      uint64_t size_of_available_ram = out_info->ram_size - ((uint64_t)kernel_size+(size_of_reserved_regions+1)); // we include kernel as reserved region but not the dtb tree region 
   uart_puts("\n size of free memory \n");
   uart_puthex((uintptr_t)size_of_available_ram);
    uart_puts("\n \n");
      uintptr_t total_number_of_possible_nodes = size_of_available_ram/MMAU_SIZE ;
       uintptr_t free_range_start_address ;
       uintptr_t free_range_end_address ;

    
//        size_t jk = 0;
//        while(jk<out_info->rsv_count)
//        {
          
//           uart_puts(" \n  Here is a reserved region \n");
//           /* To print the start address of the reserved region */
// uart_puthex((uintptr_t)out_info->rsv_regions[jk].start);
//     uart_puts(" ------");
// /* To print the end address of the reserved region */
// uart_puthex((uintptr_t)out_info->rsv_regions[jk].end);

//         jk++;
//        }

       struct reserved_region merged_array[(out_info->rsv_count)] ;
       uart_puts("hello");
       track_stack();
    uint32_t merge_array_count =    mkmau_merge_array_rsv_regions(out_info,merged_array);
    uart_puthex(merge_array_count);


      mkMAU_find_free_range_for_range_nodes_array(kernel_start,merge_array_count,merged_array,total_number_of_possible_nodes,&free_range_start_address,&free_range_end_address,out_info);


//       uart_puts("Start address ------->");
//       uart_puthex(free_range_start_address);
//       uart_puts("  end address -->>");
//       uart_puthex(free_range_end_address);
      

       memory_tracker_array = (struct mkmau_node*)free_range_start_address;
       uart_puts("\n Free range start address printing==");
       uart_puthex(free_range_start_address);
       uart_puts("\n Free range end address ==  ");
       uart_puthex(free_range_end_address);
       uart_puts("\n Hello Shinchan  1");
   memory_tracker_array_size = mkMAU_initial_tracker_array_populating(kernel_end,out_info,memory_tracker_array,free_range_end_address);

//    int k =0 ;
//    while(k<memory_tracker_array_size)
//    {
//     uart_puts("\n start=== ");
//     uart_puthex(memory_tracker_array[k].base_range);
//     uart_puts("\n end=== ");
//     uart_puthex(memory_tracker_array[k].end_range);
//     uart_puts("\n ");
//     k++;
//    }

//    uart_puts("\n---------------- ------->\n");
//      uart_puthex(memory_tracker_array_size);
//       uart_puts("\n---------------- ------->\n");

}

void mkMAU_find_free_range_for_range_nodes_array(uintptr_t kernel_start_address ,uint32_t merge_array_count , struct reserved_region *merged_array , uintptr_t number_of_maximum_nodes ,uintptr_t* free_range_start_address,uintptr_t* free_range_end_address , struct hardware_info *out_info  )
{
       uintptr_t start_address = out_info->ram_base_address;
   extern char _kernel_end[];
   uintptr_t kernel_end_address = (uintptr_t)_kernel_end ;
      uintptr_t  end_address ;
             size_t node_size = sizeof(struct mkmau_node);
       uintptr_t needed_size_for_array = node_size*number_of_maximum_nodes ;
       
       uint32_t iterator = 0 ;
       
       while(iterator<=merge_array_count)
       {
        end_address = merged_array[iterator].start ;
          if(start_address<=merged_array[iterator].start)
          {
            if(start_address==merged_array[iterator].start)
            {
                start_address=merged_array[iterator].end;
                iterator++;
                continue;
            }
              

      uint32_t correct_region_flag =  check_that_range_with_kernel(start_address , end_address , needed_size_for_array ,kernel_start_address,kernel_end_address,free_range_end_address,free_range_start_address);

      if(correct_region_flag)
      {
        break;
      }

      start_address = merged_array[iterator].end ;
      

          }
 
          iterator++ ;

       }

       start_address =merged_array[merge_array_count-1].end ;
       end_address=out_info->ram_base_address+out_info->ram_size ;
       

              check_that_range_with_kernel(start_address , end_address , needed_size_for_array ,kernel_start_address,kernel_end_address,free_range_end_address,free_range_start_address);

              uart_puts("\n I am printing the range where our memory_tracker array is resides \n");
              uart_puthex(*free_range_start_address);
              uart_puts("\n \n");
              uart_puthex(*free_range_end_address);
                uart_puts("\n needed size for array  \n");
                uart_puthex(needed_size_for_array);
              
 
}


  uint32_t check_that_range_with_kernel (uintptr_t start_address , uintptr_t end_address , uintptr_t needed_size_for_array , uintptr_t kernel_start , uintptr_t kernel_end ,uintptr_t * free_range_end_address , uintptr_t* free_range_start_address)
  {
         if(end_address-start_address>=needed_size_for_array)
         {

            if(end_address<kernel_start || start_address>kernel_end)
            {
                *free_range_start_address=start_address;
                *free_range_end_address=start_address+needed_size_for_array ;
                return 1 ;
            }

               if(start_address<kernel_start && kernel_end<end_address )
               {
                      if(kernel_start-start_address>=needed_size_for_array)
                      {
                        * free_range_end_address=kernel_start;
                        *free_range_start_address=start_address;
                        return 1 ;
                      } 
                      else if(end_address-kernel_end>=needed_size_for_array)
                       {
                          *free_range_end_address=end_address;
                          *free_range_start_address=kernel_end;
                          return 1 ;
                       }
                       else {
                        return 0;
                       }
               }

           else if( kernel_start<start_address&&kernel_end>start_address&&kernel_end<end_address )
            {
                start_address = kernel_end ;
            }
           else if(start_address<kernel_start&&kernel_start<end_address&&kernel_end>end_address )
            {
                end_address=kernel_start;
            }
           else if(start_address==kernel_start && kernel_end<end_address)
            {
                start_address=kernel_end ;
            }
          else  if(start_address<kernel_start && kernel_end==end_address )
            {
                end_address=kernel_start ;
            }

            if(end_address-start_address>=needed_size_for_array)
            {
                *free_range_end_address = start_address+needed_size_for_array;
                *free_range_start_address=start_address ;
                return 1;
            }
         }

         return 0 ;
  }

 int mkMAU_initial_tracker_array_populating(uintptr_t kernel_end_address , struct hardware_info *out_info , struct mkmau_node* memory_tracker_array ,uintptr_t end_range_tracker_array  )
 {    uart_puts("\n Hello Shinchan");
  uart_puts("\n I am printing memory_tracker_array==");
  uart_puthex((uintptr_t)memory_tracker_array);
    int tracker_array_size= -1 ;
    extern char _kernel_start[];
    uintptr_t kernel_start_address = (uintptr_t)_kernel_start;
    struct reserved_region merged_array[out_info->rsv_count] ;
    uint32_t merged_array_size = mkmau_merge_array_rsv_regions(out_info,merged_array);

    int iterator = 0 ;
             uintptr_t start_address = out_info->ram_base_address;
         uintptr_t end_address ;

         uart_puts(" \n Size tracker_array_size==");
         uart_puthex(tracker_array_size);

    while(iterator<=merged_array_size)
    {

        end_address=merged_array[iterator].start ;

        if(end_address==start_address)
        {
            uart_puts("\n start=end");
        }

        if(end_address>start_address)
        {
             uart_puts("\n end>start");
              if(kernel_start_address==start_address&&kernel_end_address==end_address)
              {
                iterator++;
                continue;
              } 
              else if(kernel_start_address==start_address&&kernel_end_address<end_address)
              { uart_puts("\n I was right ");
                 uart_puthex(tracker_array_size);
                tracker_array_size++;
                         uart_puts(" \n Size tracker_array_size==");
         uart_puthex(tracker_array_size);
                 struct mkmau_node node ;
                 node.base_range= kernel_end_address;
                 node.end_range=end_address;
                memory_tracker_array[tracker_array_size].base_range = kernel_end_address;
    memory_tracker_array[tracker_array_size].end_range  = end_address;
                         uart_puts(" \n Size tracker_array_size==");
         uart_puthex(tracker_array_size);
              } 
              else if(kernel_end_address==end_address&&kernel_start_address>start_address)
              {
                 tracker_array_size++;
                 struct mkmau_node node ;
                 node.base_range=start_address;
                 node.end_range=kernel_start_address;
                 memory_tracker_array[tracker_array_size]=node;
              }
              else if(start_address<kernel_start_address&&end_address>kernel_end_address){
                struct mkmau_node node_1;
                struct mkmau_node node_2 ;

                node_1.base_range=start_address;
                node_1.end_range=kernel_start_address;
                tracker_array_size++;
                memory_tracker_array[tracker_array_size]=node_1;
                node_2.base_range=kernel_end_address;
                node_2.end_range=end_address;
               tracker_array_size++;
                memory_tracker_array[tracker_array_size]=node_2;
              }
              else
              {
                uart_puts("\n We founf a totally free region \n");
                struct mkmau_node node ;
                uart_puts("\nssssss===");
                uart_puthex(start_address);
                uart_puts("\neeeee===");
                uart_puthex(end_address);
                node.base_range=start_address;
                node.end_range=end_address;
                tracker_array_size++;
   memory_tracker_array[tracker_array_size].base_range = node.base_range;
    memory_tracker_array[tracker_array_size].end_range  = node.end_range;
              }

        }
        start_address=merged_array[iterator].end;

        iterator++;
    }



    
   int i =0 ;
   while(i<=tracker_array_size)
   {
      if(memory_tracker_array[i].base_range==(uintptr_t)memory_tracker_array && memory_tracker_array[i].end_range==end_range_tracker_array)
      {
        array_node_shift_end(memory_tracker_array,1,i-1,tracker_array_size);
        tracker_array_size--;
        break;
      }
      else if(memory_tracker_array[i].base_range==(uintptr_t)memory_tracker_array && memory_tracker_array[i].end_range>end_range_tracker_array)
      {
            memory_tracker_array[i].base_range=end_range_tracker_array;  
            break;
      }
      else if(memory_tracker_array[i].end_range==end_range_tracker_array && memory_tracker_array[i].base_range<(uintptr_t)memory_tracker_array)
      {
         memory_tracker_array[i].end_range=(uintptr_t)memory_tracker_array;
         break;
      }
      else if (memory_tracker_array[i].base_range<(uintptr_t)memory_tracker_array && memory_tracker_array[i].end_range>end_range_tracker_array)
      {
           
            tracker_array_size++;
            memory_tracker_array[tracker_array_size].base_range=end_range_tracker_array;
            memory_tracker_array[tracker_array_size].end_range=memory_tracker_array[i].end_range ;

             memory_tracker_array[i].end_range=(uintptr_t)memory_tracker_array;
             break;
      }
    i++;
   }


   int j = 0;
while (j <= tracker_array_size)
{
    uart_puts("\n Free region start --> ");
    uart_puthex(memory_tracker_array[j].base_range);
    uart_puts("\n  end --> ");
    uart_puthex(memory_tracker_array[j].end_range);
    uart_puts(" \n  size --> ");
    uart_puthex(memory_tracker_array[j].end_range - memory_tracker_array[j].base_range);
    j++;
}


     


 }

 void array_node_shift_end (struct mkmau_node* memory_tracker_array , size_t shift_units , size_t from_which_node ,size_t array_size )

 {
  if (array_size <= 1 || from_which_node >= array_size - 1) {

        return;

    }

       size_t copy_iterator = array_size-1;

      while (copy_iterator!=from_which_node )

      {

             struct mkmau_node node ;

            node = memory_tracker_array[copy_iterator];

            memory_tracker_array[copy_iterator+1]=node;

            copy_iterator--;


      }

 } 