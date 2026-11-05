#include<stdint.h>
#include<stddef.h>

#include"../../../include/lib/dbt.h"
#include"../../../include/driverHeaders/uart.h"
#include"../../../include/mkMAU/mmu/pte_populate.h"
#include"../../../include/mkMAU/mkmau_utils.h"

uint8_t valid_bit_check(uint64_t* reg  ) ; 
void populate_root_pte_slot_zero(uint64_t* reg , uint32_t min_mem_alloaction , uintptr_t bitmap , uint32_t size_bitmap); 
// void level_1_pte(uint64_t* root_pte ) ; 

void pte_population(uintptr_t ram_base , uintptr_t ram_size , uint64_t root_page_table_index , uint64_t real_page_need_mapped_index , uint64_t virtual_address , struct page_table_entry_attributes page_attributes , uint32_t min_mem_alloaction , uintptr_t bitmap , uint32_t size_bitmap )
{

        uint64_t* zeroth_slot_root_pte = (uint64_t*)(root_page_table_index*min_mem_alloaction) ; 
       

    /*
     Step -- > 1 
     Check the valid bit and only than fill the slot 0 of the root page table entry ! 
    */

       if(!valid_bit_check(zeroth_slot_root_pte))
       {
         populate_root_pte_slot_zero(zeroth_slot_root_pte , min_mem_alloaction , bitmap , size_bitmap ) ; 
          
       }

       /*
         Step 2 -- > Traverse the root page table entry  , extract the address of the page table at level 1  , than find the correct slot in which the given virtual address should lie , check if that slot is valid or not , if not , than fill that slot ! 
       */

    //    level_1_pte(zeroth_slot_root_pte) ; 

         

}

uint8_t valid_bit_check(uint64_t* reg   )
{
    if((*reg<<63)&(1ULL<<63))
    {
        // Bit is Valid ! 
        return 1 ; 
    }
    return 0 ; // not valid , fill the entry ! 
}

void populate_root_pte_slot_zero(uint64_t* reg , uint32_t min_mem_alloaction , uintptr_t bitmap , uint32_t size_bitmap)
{
    // Filling the entry ! 
           (*reg)|=(1ULL) ; 
          uint32_t page_index = allocate_page(bitmap , size_bitmap , 1 , 0) ; 
          uint64_t address = ((uint64_t)page_index)*min_mem_alloaction ; 

        //Setting the level 1 table address
             *reg |= address;

             // Setting the type bit which is present at index 1 

             *reg |= (1ULL << 1);

             // Set address_level_1_pte

}


void level_1_pte(uint64_t* root_pte , uint64_t virtual_address )
{
    uint64_t* level_1_table_address = *root_pte & 0x0000FFFFFFFFF000ULL;  // 0x0000FFFFFFFFF000ULL -- > have 1s from bit 12 to 47 and zero everywhere else ! 
    /*
     Step -- > 1 
     Now we have the level 1 page table address ,  the first thing we do is find the correct slot to put our address , every slot on this level can cover upto 1GB of virtual address space ! 
     Right now our RAM is 512MB so that will work , but RAM in other cases can also be more than 1 GB ! 
    */
}