#include <stdint.h>
#include "../../include/driverHeaders/uart.h"
#include "../../include/mkMAU/mkMAU.h"
#include "../../include/lib/dbt.h"
 #include "../../include/mkMAU/mobilemkMAU.h"
 #include "../../include/mkMAU/mkmau_utils.h"
 #include"stddef.h"
 #define MMAU_SIZE 4096

  void array_node_shift_end (struct mkmau_node* memory_tracker_array , size_t shift_units , size_t from_which_node ,size_t array_size );
 void mkMAU_initial_tracker_array_populating(uintptr_t kernel_end_address , struct hardware_info *out_info , struct mkmau_node* memory_tracker_array  );
 void mkMAU_find_free_range_for_range_nodes_array(uintptr_t kernel_end_address , struct hardware_info *out_info , uintptr_t number_of_maximum_nodes ,uintptr_t* free_range_start_address,uintptr_t* free_range_end_address ) ;

void mkMAU_main(struct hardware_info * out_info , uintptr_t x1_register)  
{
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
      uintptr_t total_number_of_possible_nodes = size_of_available_ram/MMAU_SIZE ;
       uintptr_t free_range_start_address ;
       uintptr_t free_range_end_address ;

    
       size_t jk = 0;
       while(jk<out_info->rsv_count)
       {
          
          uart_puts(" \n  Here is a reserved region \n");
          /* To print the start address of the reserved region */
uart_puthex((uintptr_t)out_info->rsv_regions[jk].start);
    uart_puts(" ------");
/* To print the end address of the reserved region */
uart_puthex((uintptr_t)out_info->rsv_regions[jk].end);

        jk++;
       }



//       mkMAU_find_free_range_for_range_nodes_array(kernel_end,out_info,total_number_of_possible_nodes,&free_range_start_address,&free_range_end_address);


// //       uart_puts("Start address ------->");
// //       uart_puthex(free_range_start_address);
// //       uart_puts("  end address -->>");
// //       uart_puthex(free_range_end_address);
      

//        struct mkmau_node* memory_tracker_array = (struct mkmau_node*)free_range_start_address ;
//        mkMAU_initial_tracker_array_populating(kernel_end,out_info,memory_tracker_array);
     



}

void mkMAU_find_free_range_for_range_nodes_array(uintptr_t kernel_end_address , struct hardware_info *out_info , uintptr_t number_of_maximum_nodes ,uintptr_t* free_range_start_address,uintptr_t* free_range_end_address  )
{
       uintptr_t start_address = out_info->ram_base_address;
       uintptr_t end_address = out_info->rsv_regions[0].start ;
       uart_puts("--> start address of first reserved region \n");
       uart_puthex((uintptr_t)end_address);
        uart_puthex((uintptr_t)start_address);
      size_t node_size = sizeof(struct mkmau_node);
       uintptr_t needed_size_for_array = node_size*number_of_maximum_nodes ;
    size_t i =0 ;
       while(1)
       {

            uintptr_t difference_result = end_address-start_address;
            if(difference_result>=needed_size_for_array)
            {
                  *free_range_start_address=start_address;
                  *free_range_end_address   = start_address + needed_size_for_array;
                  break;
            }
            start_address = out_info->rsv_regions[i].end ;
            end_address= out_info->rsv_regions[i+1].start;
            i++;

       }
}

 void mkMAU_initial_tracker_array_populating(uintptr_t kernel_end_address , struct hardware_info *out_info , struct mkmau_node* memory_tracker_array )
 {     size_t tracker_array_size = 0;
          extern char _kernel_start[];
       uart_puts("we are in \n");
       uart_puts("below is ram base address from dtb \n");
       uart_puthex(out_info->ram_base_address);
       uart_puts("\nbelow is ram size from dtb \n");
       uart_puthex(out_info->ram_size);
       uart_puts("\n below is kernel end address \n");
       uart_puthex(kernel_end_address);
        uart_puts("\n below is kernel start address \n");
       uart_puthex((uintptr_t)_kernel_start);

       struct mkmau_node range_node ;
       range_node.base_range=kernel_end_address;
      
       range_node.end_range= (uint64_t)out_info->ram_base_address + (uint64_t)out_info->ram_size;
       uart_puts("base address in starting--->");
       uart_puthex(range_node.base_range);
       uart_puts("\n");
           uart_puts("end address in starting--->");
       uart_puthex(range_node.end_range);
       uart_puts("\n");


       memory_tracker_array[0] = range_node ;
       tracker_array_size++; 

      size_t i =0 ;
             while(i<out_info->rsv_count)   // to keep track of resereved regions !
             {
              
              uart_puts("In Outer While \n");
           
              size_t tracker_array_iterator = 0;


                  while(tracker_array_iterator<tracker_array_size)
                  {

                     uart_puts(" checking for resereved regions inside the tracker array \n      ");
                       
                     uint64_t start_address = memory_tracker_array[tracker_array_iterator].base_range;
                     uint64_t end_address = memory_tracker_array[tracker_array_iterator].end_range;

                        if(start_address==out_info->rsv_regions[i].start && end_address==out_info->rsv_regions[i].end)
                        {
                            uart_puts("\n I am in both side boundaries \n");
                            tracker_array_iterator++;
                           continue;
                        }


                     if(start_address<=out_info->rsv_regions[i].start && end_address>=out_info->rsv_regions[i].end)
                     {    
                            uart_puts("condition true  \n ");
                            uint64_t first_range_address_start  = start_address;
                            uint64_t first_range_address_end = out_info->rsv_regions[i].start;
 
                         
                           
                            struct mkmau_node node_1 ;
  

                            if(end_address==out_info->rsv_regions[i].end || start_address==out_info->rsv_regions[i].start)
                            {
                                   uart_puts("chachacha \n");
                                    if(end_address==out_info->rsv_regions[i].end)
                                    {
                                          node_1.base_range=start_address;
                                          node_1.end_range=out_info->rsv_regions[i].start;

                                    }
                                    else
                                    {
                                         node_1.base_range=out_info->rsv_regions[i].end;
                                         node_1.end_range=end_address;

                                    }
                                memory_tracker_array[tracker_array_iterator]=node_1;
                                             for(size_t j=0 ; j<tracker_array_size;j++)
             {
              uart_puts(" start-->  ");
              uart_puthex(memory_tracker_array[j].base_range);
                  uart_puts(" end-->  ");
              uart_puthex(memory_tracker_array[j].end_range);
             }
                                          break;

                            }

                             node_1.base_range=first_range_address_start;
                            node_1.end_range=first_range_address_end;
                            
                             uint64_t second_range_address_start = out_info->rsv_regions[i].end;
                            uint64_t second_range_address_end=end_address;

                            struct mkmau_node node_2 ;
                            node_2.base_range=second_range_address_start;
                            node_2.end_range=second_range_address_end;
   

                           array_node_shift_end(memory_tracker_array,1,tracker_array_iterator,tracker_array_size);

                           memory_tracker_array[tracker_array_iterator]=node_1;
                           memory_tracker_array[tracker_array_iterator+1]=node_2;
                           tracker_array_size++;
                                        for(size_t j=0 ; j<tracker_array_size;j++)
             {
              uart_puts(" start-->  ");
              uart_puthex(memory_tracker_array[j].base_range);
                  uart_puts(" end-->  ");
              uart_puthex(memory_tracker_array[j].end_range);
             }
                           uart_puts("full h \n");
                           
                     }

                     if(start_address<out_info->rsv_regions[i].start && end_address<out_info->rsv_regions[i].end && end_address>out_info->rsv_regions[i].start || 
                         start_address>out_info->rsv_regions[i].start&&end_address>out_info->rsv_regions[i].end&&start_address<out_info->rsv_regions[i].end)
                         {
                            uart_puts("I am checking overllaping    ");
                               struct mkmau_node node ; 
                              if(start_address<out_info->rsv_regions[i].start)
                              {
                                   
                                   node.base_range=start_address;
                                   node.end_range=out_info->rsv_regions[i].start;
                              }
                              else
                              {
                                   
                                   node.base_range = out_info->rsv_regions[i].end;
                                   node.end_range=end_address;
                            }
                            memory_tracker_array[tracker_array_iterator]=node;
                                         for(size_t j=0 ; j<tracker_array_size;j++)
             {
              uart_puts(" start-->  ");
              uart_puthex(memory_tracker_array[j].base_range);
                  uart_puts(" end-->  ");
              uart_puthex(memory_tracker_array[j].end_range);
             }
                            break;

                         }
  

                     tracker_array_iterator++ ;

                  }

                 
                 
                 
                i++;
             }

             for(size_t j=0 ; j<tracker_array_size;j++)
             {
              uart_puts(" start-->  ");
              uart_puthex(memory_tracker_array[j].base_range);
                  uart_puts(" end-->  ");
              uart_puthex(memory_tracker_array[j].end_range);
             }
             uart_puts("  Itd done ");


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