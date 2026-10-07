#include <stdint.h>
#include "../../include/driverHeaders/uart.h"
#include "../../include/mkMAU/mkMAU.h"
#include "../../include/lib/dbt.h"
#include "../../include/mkMAU/mobilemkMAU.h"
#include "../../include/mkMAU/mkmau_utils.h"
#include "../../include/driverHeaders/track_stack.h"
#include "../../include/driverHeaders/find_ssd_device.h"
#include "../../include/lib/quicksort.h"
#include "../../include/lib/compare_start_address.h"
 #include "../../include/lib/alignbyte.h"

#include"stddef.h"
#define MMAU_SIZE 4096

 struct mkmau_node *memory_tracker_array = NULL;
 int memory_tracker_array_size ;

 

 extern  void array_node_shift_end (struct mkmau_node* memory_tracker_array , size_t shift_units , size_t from_which_node ,size_t array_size );

 int mkMAU_find_free_range_for_range_nodes_array(uintptr_t kernel_start_address ,uint32_t merge_array_count , struct reserved_region *merged_array , uintptr_t number_of_bytes,uintptr_t* free_range_start_address,uintptr_t* free_range_end_address , struct hardware_info *out_info ,uint8_t byte_align ) ;








void mkMAU_main(struct hardware_info * out_info , uintptr_t x1_register)  
{
    uart_puts("I am in mkmau_main");
       extern char _kernel_start[];
    extern char _kernel_end[];  
    extern uint32_t max_queue_size ;  
    extern uint32_t ssd_sector_size; 
    
    uintptr_t kernel_start = (uintptr_t)_kernel_start ;
    uintptr_t kernel_end = (uintptr_t)_kernel_end ;
    size_t kernel_size = kernel_end - kernel_start;
     uint64_t size_of_reserved_regions = 0 ;  
      reserved_region_ranges_size(out_info ,&size_of_reserved_regions);
         uart_puts("\n size of Resreved Regions memory \n");
   uart_puthex((uintptr_t)size_of_reserved_regions);
    uart_puts("\n \n");
       uart_puts("\n size of SSd Queue \n");
   uart_puthex((uintptr_t)max_queue_size*26);
    uart_puts("\n \n");
       uart_puts("\n size ssd sector size  \n");
   uart_puthex((uintptr_t)ssd_sector_size);
    uart_puts("\n \n");
      uint64_t size_of_available_ram = out_info->ram_size - ((uint64_t)kernel_size+(size_of_reserved_regions)+(uint32_t)max_queue_size*26 + (uint32_t)ssd_sector_size); // we include kernel as reserved region but not the dtb tree region 
   uart_puts("\n size of free memory \n");
   uart_puthex((uintptr_t)size_of_available_ram);
    uart_puts("\n \n");

      uart_puts("\n size of kernel \n");
   uart_puthex((uintptr_t)kernel_size);
    uart_puts("\n \n");
      uintptr_t total_number_of_possible_nodes = size_of_available_ram/MMAU_SIZE ;


       uintptr_t free_range_start_address ;
       uintptr_t free_range_end_address ;
       


       uintptr_t free_range_for_ssd_queue ;
       uintptr_t free_range_for_kernel_buffer ;

    
 // merged_array is an array used for all inuse memory which either is reserved region and kernel occupied spaces ! 

       struct reserved_region merged_array[(out_info->rsv_count+1+1+1)] ; // we are adding kernel buffer , ssd queue and kernel itself also ! 
        


       uart_puts("hello");

       track_stack();

       struct reserved_region temporaray_array[(out_info->rsv_count+1)] ;

              int i=0 ;
       while(i<out_info->rsv_count)
       {
          uint64_t start = out_info->rsv_regions[i].start;
            uint64_t end = out_info->rsv_regions[i].end;
            temporaray_array[i].start=start;
            temporaray_array[i].end=end;
            i++;
       }
       temporaray_array[i].start=kernel_start;
       temporaray_array[i].end=kernel_end;

       array_sort(temporaray_array,sizeof(struct reserved_region),out_info->rsv_count+1,compare_start_address);



    uint32_t merge_array_count =    mkmau_merge_array_rsv_regions(temporaray_array,out_info->rsv_count+1,merged_array);
    uart_puthex(merge_array_count);


  merge_array_count =  mkMAU_find_free_range_for_range_nodes_array(kernel_start,merge_array_count,merged_array,total_number_of_possible_nodes*sizeof(struct mkmau_node),&free_range_start_address,&free_range_end_address,out_info,0);


    

       memory_tracker_array = (struct mkmau_node*)free_range_start_address;



       array_sort(merged_array,sizeof(struct reserved_region),merge_array_count+1,compare_start_address);
       int j =0 ;
       while(j<merge_array_count+1)
       {
         temporaray_array[j].start=merged_array[j].start ;
         temporaray_array[j].end=merged_array[j].end ;
         j++;
       }

       merge_array_count = mkmau_merge_array_rsv_regions(temporaray_array,merge_array_count+1,merged_array);

       merge_array_count = mkMAU_find_free_range_for_range_nodes_array(kernel_start,merge_array_count,merged_array,ssd_sector_size+4,&free_range_start_address
      ,&free_range_end_address,out_info,0);

            extern void * kernel_buffer_address ;

      kernel_buffer_address = (void *) free_range_start_address;

       array_sort(merged_array,sizeof(struct reserved_region),merge_array_count+1,compare_start_address);

             int jk =0 ;
       while(jk<merge_array_count+1)
       {
         temporaray_array[jk].start=merged_array[jk].start ;
         temporaray_array[jk].end=merged_array[jk].end ;
         jk++;
       }


        merge_array_count = mkmau_merge_array_rsv_regions(temporaray_array,merge_array_count+1,merged_array);


          merge_array_count = mkMAU_find_free_range_for_range_nodes_array(kernel_start,merge_array_count,merged_array,max_queue_size,&free_range_start_address
      ,&free_range_end_address,out_info,1);

      extern uintptr_t ssd_queue_address ;
      ssd_queue_address = free_range_start_address ;

      
             uart_puts("\n queue address start==");
       uart_puthex(free_range_start_address);
       uart_puts("\n queue address end ==  ");
       uart_puthex(free_range_end_address);
       uart_puts("\n Hello Shinchan  1");


         










   //          while(merge_array_count!=-1)
   //  {
   //    uart_puts("\n  Region Start");
   //    uart_puthex(merged_array[merge_array_count].start);
   //    uart_puts("----- end address");
   //     uart_puthex(merged_array[merge_array_count].end);
   //     uart_puts("\n");
   //     merge_array_count--;
   //  }




          


discover_ssd_device(out_info);

     

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



 int mkMAU_find_free_range_for_range_nodes_array(uintptr_t kernel_start_address ,uint32_t merge_array_count , struct reserved_region *merged_array , uintptr_t number_of_bytes ,uintptr_t* free_range_start_address,uintptr_t* free_range_end_address , struct hardware_info *out_info ,uint8_t byte_align  )
{
       uintptr_t start_address = out_info->ram_base_address;
      uintptr_t  end_address ;
             
      
       
       uint32_t iterator = 0 ;
       
       while(iterator<=merge_array_count)
       {
        end_address = merged_array[iterator].start ;
        
        if(start_address==end_address)
        {
         start_address=merged_array[iterator].end;
         iterator++;
         continue;
        }
        else if(start_address<end_address)
        {
             if(end_address-start_address>=number_of_bytes)
             {
               *free_range_start_address = start_address ;
               if( byte_align)
               {
                   byte_alignemnt_by_4096((void *)free_range_start_address);

                   if(*free_range_start_address+number_of_bytes<=end_address)
                   {
                                    *free_range_end_address=*free_range_start_address+ number_of_bytes;
               merge_array_count++;
               merged_array[merge_array_count].start= *free_range_start_address ;
               merged_array[merge_array_count].end = *free_range_start_address+ number_of_bytes;
               return merge_array_count;
                   }
                   else
                   {  // write the corrrect start_address assignment here 
                         start_address = merged_array[iterator].end;
                     iterator++;
                     continue;
                   }

               }
               *free_range_end_address=*free_range_start_address+ number_of_bytes;
               merge_array_count++;
               merged_array[merge_array_count].start= *free_range_start_address ;
               merged_array[merge_array_count].end = *free_range_start_address+ number_of_bytes;
               return merge_array_count;
             }
        }
  

       }

 
              
 
}
