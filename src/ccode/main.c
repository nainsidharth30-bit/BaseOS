#include "../../include/driverHeaders/uart.h"
#include<stdint.h>
#include "../../include/lib/dbt.h"
#include "../../include/mkMAU/mkMAU.h"

struct hardware_info g_hw = {0} ;

void bkernel_main(uintptr_t x0_register , uint64_t x1_register )
{  
   


       
      extracting_dbt_info(x0_register  , &g_hw) ; 
      mkMAU_main(&g_hw, x1_register) ;
      
    
    
}