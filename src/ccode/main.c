#include "../../include/driverHeaders/uart.h"
#include<stdint.h>
#include "../../include/lib/dbt.h"
#include "../../include/mkMAU/mkMAU.h"
#include "../../include/driverHeaders/generic_struct_ssd.h"

struct hardware_info g_hw = {0} ;

 uint32_t max_queue_size = -1 ; 

 uintptr_t ssd_queue_address = 0 ;
 
 void * kernel_buffer_address = 0 ;
 uint32_t ssd_sector_size = 0 ;

   int (*ssd_driver)(struct ssd_request_bpt* bpt)=0;

void bkernel_main(uintptr_t x0_register , uint64_t x1_register )
{ 

  uart_puts("\n\nZZZ_UNIQUE_MARKER_12345\n\n");

  uart_puts("\n\n");   
  uart_puts("\n I am in main 1  \n");
      
    extern char _kernel_start[];
    extern char _kernel_end[]; 
    
      uintptr_t kernel_start = (uintptr_t)_kernel_start ;
    uintptr_t kernel_end = (uintptr_t)_kernel_end ;

    // uart_puts("\n Kernel Info \n");
    // uart_puts("\n Kernel start == ");
    // uart_puthex(kernel_start);
    // uart_puts("\n");
    // uart_puts("\n kernel end ==  ");
    // uart_puthex(kernel_end);
    // uart_puts("\n");

  uart_puts("\n I am in main 2\n");


   
  // uart_puts(" \n We are extracting DBT Information now \n");
  
      extracting_dbt_info(x0_register  , &g_hw) ; 


uart_puts("\nI am outsiude dtb \n");





  //  uart_puts("\n  Extracted Information   \n");
  //   uart_puts("\n RAM BASE ADDRESS ==  ");
  //   uart_puthex(g_hw.ram_base_address);
  //   uart_puts("\n");
  //    uart_puts("\n  RAM Size ==   \n");
  //    uart_puthex(g_hw.ram_size);
  //     uart_puts("\n   ");
  //      uart_puts("\n Reserved Regions Info  \n");
      //  for(int i=0 ; i<g_hw.rsv_count;i++)
      //  {
      //        uart_puts("\n Start Address ==   ");
      //        uart_puthex(g_hw.rsv_regions[i].start);
      //         uart_puts("\n   ");
      //         uart_puts("\n   End Address ==");
      //         uart_puthex(g_hw.rsv_regions[i].end);
      //         uart_puts("\n  ");

      //  }

        // uart_puts("\n Total Reserved Regions are ==  ");
        // uart_puthex(g_hw.rsv_count);
        //  uart_puts("\n   ");
      
       mkMAU_main(&g_hw, x1_register) ;
      
    
    
}