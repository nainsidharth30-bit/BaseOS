#include "../../include/driverHeaders/uart.h"
#include<stdint.h>
#include "../../include/lib/dbt.h"
#include "../../include/mkMAU/mkMAU.h"
#include "../../include/driverHeaders/peripheral_struct.h"
#include "../../include/interrupts/ic_bpt.h"
#include"../../include/interrupts/enable_irq.h"
#include"../../include/interrupts/devices_setup.h"
#include"../lib/memcpy.h"
#include"../lib/setzero.h"


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
Now calling ic_code from peripheral_devices(SSD Controller) to turn their irq line on ! 
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


*/






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