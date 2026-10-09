#include "../../include/driverHeaders/uart.h"
#include<stdint.h>
#include "../../include/lib/dbt.h"
#include "../../include/mkMAU/mkMAU.h"
#include "../../include/driverHeaders/generic_struct_ssd.h"

struct hardware_info g_hw = {0} ;

/* Stores the maximum Queue size for the SSD controller Queue*/
 uint32_t max_queue_size = -1 ; 
/* stores the address of the Queue used by SSD controller */
 uintptr_t ssd_queue_address = 0 ;
 
 /* Stores the Address of the buffer which is used by SSD controller for DMA */
 void * kernel_buffer_address = 0 ;

 /* Gives the sector size in SSD , Size of kernel buffer is equal to SSD Sector size */
 uint32_t ssd_sector_size = 0 ;


    /* Pointer function to the present SSD driver , we are bining it to correct SSD driver on run time ! */
   int (*ssd_driver)(struct ssd_request_bpt* bpt)=0;




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
  }
  else
  {
    uart_puts("\n Its a BSP , extracting BSP info now !\n");
  }
      

    uart_puts("\n Now we are entering in mkMAU main ! \n");






       /*  
   ARCHITECTURAL DECISION ----->
         The memory which is used by the kernel ( like reserved regions , buffers , SSD Queue , Kernel itself) 
         must be completely invisible to the extensions so that they cannot acccessa any such memory ! 
         More importantly as I am making the BaseOS for all RISC based chips and it does hardweare discovery on runtime hence many things are being built on runtime to prevent memory overhead 
         For example , our memory_tracker_array stores mkMAU nodes which stores range of the region and a flag whether it is free or not , to track memory ! 
         I am not building it on compile time or static time because that will remain as a fixed length ! 
         Rule ---> if extension is asking memory than it must ask for atleast 4kb of memory in every request ! 
         Chips with lot of Ram needs a bigger such array and chips with very smaller ram needs smaller array , hence I did one thing , I calculate the available ram by 
         Total RAM - (kernel_static_size + kernel_buffer+SSD_QUEUE +...) , than I divide this value to sizeof(mkmau_node) , this way we get maximum number of nodes needed to track whole ram , 
         than I made a memory_trtacker_array pointer of type mkmau_node and using that to track memory ! 
         As all these things are happening on runtime , hence I am managing every byte of memory by myself inside the mkMAU_main function ! 
   */
      
       mkMAU_main(&g_hw, x1_register) ;
      
    
    
}