#ifndef KERNEL_MAP_H
#define KERNEL_MAP_H

#include<stdint.h>
#include<stddef.h>

/*
Attributes of a leaf page table entry ( level 3 , maps one 4 KB page ) .
The entry is 8 bytes = 64 bits , bit 0 is the rightmost bit and bit 63 is the leftmost bit ! 
Every attribute below is stored in its own uint8_t for easy use , a function packs them into the real 64 bit entry ! 
*/

struct page_table_entry_attributes{




    uint8_t memory_type ;        // 3 bits --> bits 2 to 4 
                                 // Purpose : index ( 0 to 7 ) into MAIR_EL1 register which tells the memory kind , our rule : 0 = normal RAM , 1 = device memory ( MMIO ) ! 

    uint8_t user_access ;        // 1 bit  --> bit 6 ( AP[1] ) 
                                 // Purpose : 1 = user ( EL0 ) can access this page , 0 = only kernel can access ! 

    uint8_t read_only ;          // 1 bit  --> bit 7 ( AP[2] ) 
                                 // Purpose : 1 = page is read only , 0 = page is read and write ! 

    uint8_t shareability ;       // 2 bits --> bits 8 to 9 
                                 // Purpose : which cores see the same data , 0 = not shared , 2 = outer shareable , 3 = inner shareable , we use 3 for normal RAM ! 

    uint8_t access_flag ;        // 1 bit  --> bit 10 
                                 // Purpose : must be 1 , if it is 0 the first access to the page faults ! 
                                 // Mapping function sets this automatically to 1 ! 

    uint8_t not_global ;         // 1 bit  --> bit 11 
                                 // Purpose : 1 = mapping belongs to one process ( tied to its ASID ) , 0 = mapping is same for all processes ! 

    /*
      bits 12 to 47 ( 36 bits ) = physical address of the page , not stored in this struct because the mapping function gets it as a separate argument ! 
      bits 48 to 51 ( 4 bits )  = reserved , always 0 ! 
    */

    uint8_t contiguous_hint ;    // 1 bit  --> bit 52 
                                 // Purpose : hint that this entry is part of a group of contiguous entries 

    uint8_t no_execute_kernel ;  // 1 bit  --> bit 53 ( PXN ) 
                                 // Purpose : 1 = kernel ( EL1 ) is not allowed to run code from this page , 0 = allowed ! 

    uint8_t no_execute_user ;    // 1 bit  --> bit 54 ( UXN ) 
                                 // Purpose : 1 = user ( EL0 ) is not allowed to run code from this page , 0 = allowed ! 

    uint8_t software_bits ;      // 4 bits --> bits 55 to 58 
                                 // Purpose : ignored by hardware , free for our own use ( for example shared page , lazy page , copy on write ) ! 

    /*
      bits 59 to 63 ( 5 bits ) = ignored by hardware , we leave them 0 ! 
    */

} ;


void pte_population( uintptr_t ram_base, uint64_t root_page_table_index, uint64_t virtual_address, uint64_t page_attributes_register, uint32_t min_mem_alloaction, uintptr_t bitmap, uint32_t size_bitmap ) ; 


#endif