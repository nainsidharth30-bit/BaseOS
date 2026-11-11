#include<stdint.h>
#include<stddef.h>

#include"../../../include/lib/dbt.h"
#include"../../../include/lib/setzero.h"
#include"../../../include/driverHeaders/uart.h"
#include"../../../include/mkMAU/mmu/pte_populate.h"
#include"../../../include/mkMAU/mkmau_utils.h"

/* ----------------------------- forward declarations ----------------------------- */

uint8_t valid_bit_check(uint64_t* reg);

void populate_pte_slot(uint64_t* reg, uintptr_t ram_base, uint32_t min_mem_alloaction, uintptr_t bitmap, uint32_t size_bitmap);

void populate_leaf_pte_slot(uint64_t* reg, uint64_t page_attributes_register);

void level_1_pte(uint64_t* root_pte, uint64_t virtual_address, uint64_t page_attributes_register, uintptr_t ram_base, uint32_t min_mem_alloaction, uintptr_t bitmap, uint32_t size_bitmap);

void level_2_pte(uint64_t* slot_address_level_1, uint64_t virtual_address, uint64_t page_attributes_register, uintptr_t ram_base, uint32_t min_mem_alloaction, uintptr_t bitmap, uint32_t size_bitmap);

void level_3_pte(uint64_t* slot_address_level_2, uint64_t virtual_address, uint64_t page_attributes_register);

/* ----------------------------- entry point ----------------------------- */
void pte_population( uintptr_t ram_base, uint64_t root_page_table_index, uint64_t virtual_address, uint64_t page_attributes_register, uint32_t min_mem_alloaction, uintptr_t bitmap, uint32_t size_bitmap )
{
    /*
     The bitmap counts pages from the start of RAM, so the address of the root
     table is ram_base + index * page_size, not just index * page_size.
    */
    uint64_t* zeroth_slot_root_pte = (uint64_t*)(ram_base + root_page_table_index * min_mem_alloaction);

    /*
     Step --> 1
     Check the valid bit of the root slot. If invalid, allocate a page for the
     level 1 table and install its address into the root slot.
    */
    if(!valid_bit_check(zeroth_slot_root_pte))
    {
        populate_pte_slot(zeroth_slot_root_pte , ram_base , min_mem_alloaction , bitmap , size_bitmap);
    }

    /*
     Step --> 2
     Traverse into the level 1 table and continue the walk.
    */
    level_1_pte(zeroth_slot_root_pte, virtual_address, page_attributes_register, ram_base, min_mem_alloaction, bitmap, size_bitmap);
}

/* ----------------------------- helpers ----------------------------- */

uint8_t valid_bit_check(uint64_t* reg)
{
    if((*reg<<63)&(1ULL<<63))
    {
        // Bit is Valid !
        return 1 ;
    }
    return 0 ; // not valid , fill the entry !
}

/*
  Installs a table descriptor into the given slot :
    - allocates one 4KB page for the next-level table
    - zeroes that page so every slot inside it starts invalid
    - writes its physical address ( ram_base + index * page_size ) into bits 12 to 47
    - sets valid bit (0) and type bit (1)
*/
void populate_pte_slot(uint64_t* reg , uintptr_t ram_base , uint32_t min_mem_alloaction , uintptr_t bitmap , uint32_t size_bitmap)
{
    uint32_t page_index = allocate_page(bitmap , size_bitmap , 1 , 0) ;
    uint64_t address    = ram_base + ((uint64_t)page_index) * min_mem_alloaction ;

    // Zero the newly allocated table page. Every slot inside must start invalid.
    set_zero((void*)address , min_mem_alloaction);

    // Install the table descriptor: valid + type + address in one direct write.
    // The slot is guaranteed invalid (caller checked), so overwrite is safe.
    *reg = (address & 0x0000FFFFFFFFF000ULL) | (1ULL << 0) | (1ULL << 1);
}

/* ----------------------------- level 1 ----------------------------- */

void level_1_pte(uint64_t* root_pte, uint64_t virtual_address, uint64_t page_attributes_register, uintptr_t ram_base, uint32_t min_mem_alloaction, uintptr_t bitmap, uint32_t size_bitmap)
{
    uint64_t* level_1_table_address = (uint64_t*)(*root_pte & 0x0000FFFFFFFFF000ULL);  // bits 12 to 47

    /*
     Step --> 1
     Every slot on level 1 covers 1 GB of virtual address space.
     Slot number = VA / 1GB , keep only the low 9 bits (512 slots).
    */
    uint32_t slot_index = (virtual_address >> 30) & 0x1FF;

    uint64_t* slot_address = level_1_table_address + slot_index;

    /*
     Step --> 2
     If the slot is not valid, allocate a page for the level 2 table.
    */
    if(!valid_bit_check(slot_address))
    {
        populate_pte_slot(slot_address , ram_base , min_mem_alloaction , bitmap , size_bitmap);
    }

    /*
     Step --> 3
     Descend into level 2.
    */
    level_2_pte(slot_address, virtual_address, page_attributes_register, ram_base, min_mem_alloaction, bitmap, size_bitmap);
}

/* ----------------------------- level 2 ----------------------------- */

void level_2_pte(uint64_t* slot_address_level_1, uint64_t virtual_address, uint64_t page_attributes_register, uintptr_t ram_base, uint32_t min_mem_alloaction, uintptr_t bitmap, uint32_t size_bitmap)
{
    uint64_t* level_2_table_address = (uint64_t*)(*slot_address_level_1 & 0x0000FFFFFFFFF000ULL);  // bits 12 to 47

    /*
     Step --> 1
     Every slot on level 2 covers 2 MB of virtual address space.
     Slot number = VA / 2MB , keep only the low 9 bits (512 slots).
    */
    uint32_t slot_index = (virtual_address >> 21) & 0x1FF;

    uint64_t* slot_address = level_2_table_address + slot_index;

    /*
     Step --> 2
     If the slot is not valid, allocate a page for the level 3 table.
    */
    if(!valid_bit_check(slot_address))
    {
        populate_pte_slot(slot_address , ram_base , min_mem_alloaction , bitmap , size_bitmap);
    }

    /*
     Step --> 3
     Descend into level 3.
    */
    level_3_pte(slot_address, virtual_address, page_attributes_register);
}

/* ----------------------------- level 3 ----------------------------- */

void level_3_pte(uint64_t* slot_address_level_2, uint64_t virtual_address, uint64_t page_attributes_register)
{
    uint64_t* level_3_table_address = (uint64_t*)(*slot_address_level_2 & 0x0000FFFFFFFFF000ULL);  // bits 12 to 47

    /*
     Step --> 1
     Every slot on level 3 covers 4 KB of virtual address space.
     Slot number = VA / 4KB , keep only the low 9 bits (512 slots).
    */
    uint32_t slot_index = (virtual_address >> 12) & 0x1FF;

    uint64_t* slot_address = level_3_table_address + slot_index;

    /*
     Step --> 2
     This is the last level. The slot holds a leaf page descriptor.
     If invalid, fill it with the complete leaf PTE.
    */
    if(!valid_bit_check(slot_address))
    {
        populate_leaf_pte_slot(slot_address, page_attributes_register);
    }

    /*
     Step --> 3
     Done. slot_address now maps virtual_address to the physical page P.
    */
}

/* ----------------------------- leaf PTE builder ----------------------------- */

void populate_leaf_pte_slot(uint64_t* reg, uint64_t page_attributes_register)
{
    /*
     page_attributes_register is a fully formed leaf PTE :
     valid bit, type bit, physical address in bits 12 to 47, and all attribute bits
     are already at their correct positions. Direct copy.
    */
    *reg = page_attributes_register;
}