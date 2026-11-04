#include "../../include/driverHeaders/uart.h"
#include<stdint.h>
#include "../../include/lib/dbt.h"
#include "../../include/driverHeaders/peripheral_struct.h"
#include "../../include/interrupts/ic_bpt.h"
#include"../../include/interrupts/enable_irq.h"
#include"../../include/interrupts/devices_setup.h"
#include"../../include/mkMAU/mmu/root_pte.h"
#include"../lib/memcpy.h"
#include"../lib/setzero.h"
#include "../../include/mkMAU/mmu/bitmap.h"
#include "../../include/mkMAU/mmu/populate_bitmap.h"
#include"../../include/mkMAU/page_collect.h"


struct hardware_info g_hw = {0} ;





   /* We are populating all the above variuables on runtime while parsing dtb , because these will be used to know how much memory to give to the buffer for DMa and Queue size on runtime */

void bkernel_main(uintptr_t x0_register , uint64_t x1_register )
{ 

  uart_puts("\n\nZZZ_UNIQUE_MARKER_12345\n\n");

  uart_puts("\n\n");   
  uart_puts("\n I am in main 1  \n");
      
    extern char _kernel_start[];
    extern char _kernel_end[]; 


    
    
      uintptr_t kernel_start = (uintptr_t)_kernel_start ;
    uintptr_t kernel_end = (uintptr_t)_kernel_end ;




    uart_puts("\n Kernel Info \n");
    uart_puts("\n Kernel start == ");
    uart_puthex(kernel_start);
    uart_puts("\n");
    uart_puts("\n kernel end ==  ");
    uart_puthex(kernel_end);
    uart_puts("\n");

 


   
  // uart_puts(" \n We are extracting DBT Information now \n");

  uart_puts("\n Checking the magic Value for DTB \n");
  if(__builtin_bswap32(*(uint32_t *)x0_register)==0xd00dfeed)
  {

     uart_puts(" \n It is a dtb , extracting information !  \n");
      extracting_dbt_info(x0_register  , &g_hw) ; 
    uart_puts("\nI am outsiude dtb \n");
    uart_puthex(g_hw.mem_sec);
  }
  else
  {
    uart_puts("\n Its a BSP , extracting BSP info now !\n");
  }


  /* Initial Devices Setup  */

  



  /*
    Step -- > 

    Populate ic_code entry of peripheral devices , doing a lookup and checking if(interrupt-controller-phandle of peripheral == phanlde of interrupt controller) !
    In devices_setup.c ! 
    
  */

      extern uint16_t ssd_index_array[10];
extern uint8_t  ssd_index_array_count;

extern uint16_t ic_index_array[20];
extern uint8_t  ic_index_array_count;

devices_setup(&g_hw , ssd_index_array , ssd_index_array_count , ic_index_array , ic_index_array_count);



/* Step -->       DONE ! */


/* DECISION -- > First , We will turn on the irq line of SSD Controller only because it needs to fetch extensions binary from SSD to RAM ! */


/* Step -- >  
Now calling ic_code from peripheral_devices(SSD Controller) to turn their irq line on  or enabling Interrupt Lines  ! 
*/

 struct ic_bpt ic_bpt_node;
set_zero(&ic_bpt_node, sizeof(struct ic_bpt));
  uart_puts("\n H1 \n");

  struct device_entry* ic_ptr = NULL ;
   uart_puts("\n H2 \n");

 for(int i =0 ; i<ssd_index_array_count ; i++)
 {
  

  ic_ptr =( struct device_entry*)g_hw.devices[ssd_index_array[i]].interrupt_controller_phandle ; 


     memcopy(ic_bpt_node.mmio_base_mmio_size_pair_array , ic_ptr->mmio_base_size_array , sizeof(ic_ptr->mmio_base_size_array) );
     memcopy(ic_bpt_node.irq_cells, g_hw.devices[ssd_index_array[i]].irq_cells , sizeof(g_hw.devices[ssd_index_array[i]].irq_cells) );


     ic_bpt_node.total_number_of_elemnets = ic_ptr->mmio_base_size_array_count ; 
     ic_bpt_node.number_of_irq_cells = g_hw.devices[ssd_index_array[i]].irq_cells_count ; 

     ic_bpt_node.operation_code = 1 ; // We want to enable the irq line ! 
     ic_bpt_node.priority_value = g_hw.devices[ssd_index_array[i]].irq_priority ; 
     

     ic_ptr->ic_code(&ic_bpt_node);
   
     
      set_zero(&ic_bpt_node , sizeof(struct ic_bpt));
     
       ic_ptr=NULL;
   
 }



/* Step --> Done ! */


 

       /*
         Step -- > 
         Check if it is MMU , MPU or None ! 
       */

       if(g_hw.mem_sec==2)
       {
        uart_puts("Its MMU ! DO Paging ");

        /*
          Step -- > 1 
           We need to make a bitmap , whose every indexed bit will represent that indexed 4kb page in RAM ! 
           So we need to accolacte some space for the bitmap ! 
        */

        uintptr_t bitmap =0; 


     uint32_t size_of_bitmap =    make_bitmap(g_hw.ram_size , &g_hw , &bitmap) ; 



       // Filling the whole bitmap before populating it , 0xFF means every bit is 1 , that is every page is free ! 
     uint8_t* bitmap_bytes = (uint8_t*)bitmap ; 
     uint8_t  fill_value = 0xFF ; 

     for(uint32_t b = 0 ; b < size_of_bitmap ; b++)
     {
         bitmap_bytes[b] = fill_value ; 
     }


     /*
       Step --> 2 
       Now we have to populate the Bitmap but before that we need to collect all those pages which covers the reserved regions , the kernel range and the bitmap itself also ! 
     */

     uint32_t base_page =0 ; 

     // Marking the reserved regions first ! 

     for(int i =0 ; i<g_hw.rsv_count ; i++)
     {
   uint32_t total_continous_pages =      page_collect(g_hw.rsv_regions[i].start , g_hw.rsv_regions[i].end ,&base_page , g_hw.min_mem_allocation , g_hw.ram_base_address,g_hw.ram_base_address+g_hw.ram_size ) ; 

      for(uint32_t j =0 ; j<total_continous_pages ; j++)
      {
        uint64_t arr[1] ; 
        arr[0] = base_page+ j ; 
         populate_bitmap(bitmap , size_of_bitmap ,arr , 1 , 0 ) ; 
      }

     }

     // Marking the kernel 

     uint32_t total_continous_pages =  page_collect(kernel_start, kernel_end ,&base_page , g_hw.min_mem_allocation , g_hw.ram_base_address,g_hw.ram_base_address+g_hw.ram_size ) ; 

      for(uint32_t j =0 ; j<total_continous_pages ; j++)
      {
        uint64_t arr[1] ; 
        arr[0] = base_page+ j ; 
         populate_bitmap(bitmap , size_of_bitmap ,arr , 1 , 0 ) ; 
      }

      // Marking the bitmap now 

      total_continous_pages =  page_collect(bitmap, bitmap+size_of_bitmap ,&base_page , g_hw.min_mem_allocation , g_hw.ram_base_address,g_hw.ram_base_address+g_hw.ram_size ) ; 

      for(uint32_t j =0 ; j<total_continous_pages ; j++)
      {
        uint64_t arr[1] ; 
        arr[0] = base_page+ j ; 
         populate_bitmap(bitmap , size_of_bitmap ,arr , 1 , 0 ) ; 
      }



        // making_root_pte_array();

           
       }
       else if(g_hw.mem_sec==1)
       {
        uart_puts("Its MPU ! ") ; 
       }
       else
       {
        uart_puts("Its None ! ") ; 
       }
      
    
    
}