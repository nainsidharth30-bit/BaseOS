#include"../../../include/mkMAU/mmu/root_pte.h"
#include"../../../include/lib/dbt.h"
#include"../../../include/lib/setzero.h"
#include"../../../include/mkMAU/mkmau_utils.h"
#include"../../../include/driverHeaders/uart.h"

#include<stdint.h>
#include<stddef.h>


static struct root_pte_entry* root_pte_array = NULL ; 
static uint32_t root_pte_count = 0 ; 

struct root_pte_entry* root_pte_array_setup(struct hardware_info* out_info , uintptr_t bitmap , uint32_t size_bitmap ) 
{
    uint32_t max_processes = out_info->max_processes ; 

    uint64_t bytes_needed = sizeof(struct root_pte_entry)*max_processes ; 
    uint32_t pages_needed = 0 ; 

    if(bytes_needed%out_info->min_mem_allocation==0)
    {
        pages_needed = bytes_needed/out_info->min_mem_allocation;
    }
    else
    {
        pages_needed = bytes_needed/out_info->min_mem_allocation+1;
    }

    uint16_t next_pages = 0 ; 

    uint32_t first_page = free_contiguous_pages_wrapper(&next_pages , pages_needed , bitmap , size_bitmap ) ; 

    root_pte_array = (struct root_pte_entry*)(out_info->ram_base_address + ((uint64_t)first_page)*out_info->min_mem_allocation) ; 

    return root_pte_array ;
}
void root_pte_set_kernel(struct hardware_info* out_info , uintptr_t bitmap , uint32_t size_bitmap )
{
    uint32_t page_index = allocate_page(bitmap,size_bitmap , 1 , 0) ; 
    uint64_t addr       = out_info->ram_base_address + ((uint64_t)page_index)*out_info->min_mem_allocation ; 

    set_zero((void*)addr , out_info->min_mem_allocation) ; 

    root_pte_array[root_pte_count].page_index = page_index ; 
    root_pte_array[root_pte_count].pid = 0 ; 

    root_pte_count++ ; 

    /*
     Now a root page table entry is allocated for kernel ! 
     PID of kernel is always zero ! 
    */
}