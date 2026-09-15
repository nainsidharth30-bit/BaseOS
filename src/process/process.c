
#include<stdint.h>
#include<stddef.h>
#include "../../include/driverHeaders/uart.h"
#include "../../include/lib/meta-ext.h"
#include "../../include/mkMAU/mkmau_utils.h"
#include "../../include/mkMAU/mkMAU.h"




void set_inital_stage_for_extensions()
{
     int iterator = 0 ;
     while(iterator<extension_count)
     {

        // STep:1 First , We will see how much size some extenion needs , we allocate that much size to it via memory_tracker_array 

        uintptr_t required_memory = available_extensions_array[iterator].size_in_bytes ;
    
        struct  mkmau_node node  = memory_allocator(required_memory);

        // step:2 Second , now we will start to read binary of the extension inside the reserved memory block provided in node . 
          


        iterator++;
     }
}