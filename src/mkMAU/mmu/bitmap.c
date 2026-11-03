#include"../../../include/lib/dbt.h"
#include"../../../include/driverHeaders/uart.h"
#include<stdint.h>
#include<stddef.h>

uint8_t min_mem_aligned(uint64_t* start , uint64_t* end , uint32_t required_bytes , struct hardware_info* out_info); 
/*
This function will return the size of the bitmap ! 
*/
uint32_t make_bitmap( uint64_t ram_size_in_bytes , struct hardware_info * out_info  , uintptr_t* bitmap   ) 
{
    /*
      Step -- > 1 Calculate max pages ! 
    */
    uint32_t max_pages = ram_size_in_bytes/out_info->min_mem_allocation ; 

     /*
      Step -- > 2 
      One page is of size min_mem_allocation , hence we have number max number of pages , every page will be represented by one bit , so we will do need max number of pages bits , divide that by 8 and we get number of bytes needed to represent those pages ! 
    */

    uint32_t number_of_bytes_required = (max_pages+7)/8 ;  // To make it a multiple of 8  ! 

    /*
      Step -- > 3 
      now we just have to find that range whose size is number_of_bytes_required and is free  ! 
      and on top of that starting address and ending address of that range must be a multiple of min_mem_allocation ! so that we get aligned pages for our bitmap ! 
      We have rsv regions sorted inside the out_info , we have kernel end and start address also , we will make a merged array first including rsv regions and kernel range , than we will find those ranges whose size is greter than or equal to number of bytes required and are aligned by min_mem_allocation ! 
    */

    // Making the array first , inserting the kernel ragion at correct place inside that array ! 

       extern char _kernel_start[];
    extern char _kernel_end[]; 


    
    
      uintptr_t kernel_start = (uintptr_t)_kernel_start ;
    uintptr_t kernel_end = (uintptr_t)_kernel_end ;


        struct reserved_region merged_array[out_info->rsv_count+1] ; // Including kernel also ! 

    // We will just compare the start addresses only and insert the kernel !
    int i = 0 ; 
    int j = 0 ; 
    int kernel_inserted = 0 ; 

    while(i < out_info->rsv_count)
    {
        if(out_info->rsv_regions[i].start > kernel_start && !kernel_inserted)
        {
            merged_array[j].start = kernel_start ; 
            merged_array[j].end = kernel_end ; 
            j++ ; 

            merged_array[j].start = out_info->rsv_regions[i].start ; 
            merged_array[j].end = out_info->rsv_regions[i].end ; 

            kernel_inserted = 1 ; 
        }
        else
        {
            merged_array[j].start = out_info->rsv_regions[i].start ; 
            merged_array[j].end = out_info->rsv_regions[i].end ; 
        }
        i++ ; 
        j++ ; 
    }

    // If the kernel comes after all the reserved regions ( or there are no reserved regions ) than insert it at the end ! 
    if(!kernel_inserted)
    {
        merged_array[j].start = kernel_start ; 
        merged_array[j].end = kernel_end ; 
        j++ ; 
    }

    // Now merged_array has j entries , sorted by start address ! 
    // Now we have to find free bytes which are aligned also !


    // Method -- > we will make a start pointer which points towards the start of that range and an end pointer which points to end of the range ! we will update them continously until find free range !

    i = 0 ; 
    uint64_t  start = out_info->ram_base_address ; 
    uint64_t  end = merged_array[0].start ; 
    uint64_t  ram_end = out_info->ram_base_address + ram_size_in_bytes ; 
    uint32_t  merged_count = j ;    // number of entries inside merged_array ! 

    while(1)
    {
          // end > start is checked first because unsigned subtraction can underflow ! 
          if(end > start && end-start >= number_of_bytes_required)
          {
               uint8_t flag =  min_mem_aligned(&start , &end  , number_of_bytes_required , out_info) ; 
               if(flag)
               {
                *bitmap = start ; 
                break;
               }
          } 

          // This gap did not work , if it was the last gap than nothing is found ! 
          if(i >= merged_count)
          {
              return -1 ; 
          }

          // Moving to the next gap , start never goes backwards because regions can overlap ! 
          if(merged_array[i].end > start)
          {
              start = merged_array[i].end ; 
          }

          if(i+1 < merged_count)
          {
              end = merged_array[i+1].start ; 
          }
          else
          {
              end = ram_end ;     // last gap goes till the end of ram ! 
          }
          i++ ; 
    }


  uart_puts(" \n Value of BitMAP is \n") ; 
  uart_puthex(*bitmap); 

  uart_puts(" \n Size of BitMAP is \n") ; 
  uart_puthex(end-start) ; 

  return end-start ; 
    

}


uint8_t min_mem_aligned(uint64_t* start , uint64_t* end , uint32_t required_bytes , struct hardware_info* out_info)
{
         /*
          Here we need to check or extract that range which is equal to required bytes and is aligned with min_mem_allocation value ! 
         */

         if(*start%out_info->min_mem_allocation == 0 )
         {
            // That means start is already aligned 
            *end = *start + required_bytes ;  // Because required bytes is calculated according to min_mem_allocation hence end will be already assigned to min_mem_allocation ! 
            return 1 ; // Aligned flag turns true ! 
         }
         else
         {
            // Otherwise we need to align the bytes within that range ! 
            // Rounding start up to the next multiple of min_mem_allocation ! 
            uint64_t  aligned_start = ((*start + out_info->min_mem_allocation - 1)/out_info->min_mem_allocation)*out_info->min_mem_allocation ; 

            // After rounding up the range must still fit inside the gap ! 
            // aligned_start < *end is checked first because unsigned subtraction can underflow ! 
            if(aligned_start < *end && (*end - aligned_start) >= required_bytes)
            {
                *start = aligned_start ; 
                *end = aligned_start + required_bytes ; 
                return 1 ; // Aligned flag turns true ! 
            }

            return 0 ; // Gap is too small after aligning , flag stays false ! 
         }
}