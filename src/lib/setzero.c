
#include<stdint.h>

void set_zero(void* base_address , uintptr_t size)
{
    uint8_t *one_byte_tracker_pointer = (uint8_t*)base_address ; 
   uintptr_t i =0 ; 
   while(i<size)
   {
     *one_byte_tracker_pointer = 0 ; 
    i++ ; 
    one_byte_tracker_pointer++;
   }
}
