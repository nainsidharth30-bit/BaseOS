 #include<stdint.h>
 #include "../../include/lib/alignbyte.h"
 #include "../../include/driverHeaders/uart.h"
 void byte_alignment(void **aligning_ptr  )
 {
  uintptr_t current_address = (uintptr_t)(*aligning_ptr);
  
    uintptr_t next_multiple_address_of_four = (current_address+3) & ~3 ;
   
    *aligning_ptr =(void*) next_multiple_address_of_four ;

 }