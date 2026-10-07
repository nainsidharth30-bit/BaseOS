 #include<stdint.h>
 #include "../../include/lib/alignbyte.h"
 #include "../../include/driverHeaders/uart.h"
 void byte_alignment(void **aligning_ptr  )
 {
  uintptr_t current_address = (uintptr_t)(*aligning_ptr);
  
    uintptr_t next_multiple_address_of_four = (current_address+3) & ~3 ;
   
    *aligning_ptr =(void*) next_multiple_address_of_four ;

 }

  void byte_alignemnt_by_4096(uintptr_t *aligning_ptr)
{
    uintptr_t current_address = *aligning_ptr;
    uintptr_t next_multiple = (current_address + 4095) & ~(uintptr_t)4095;
    *aligning_ptr = next_multiple;
}