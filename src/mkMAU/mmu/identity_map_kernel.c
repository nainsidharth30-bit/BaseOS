#include"../../../include/lib/dbt.h"
#include"../../../include/driverHeaders/uart.h"
#include"../../../include/mkMAU/mmu/root_pte.h"
#include"../../../include/mkMAU/mmu/pte_populate.h"

#include<stdint.h>

void identity_map_kernel(struct hardware_info * out_info , uintptr_t bitmap , uint32_t size_bitmap , struct root_pte_entry * root_pte_array )
{
    /*
    ARCHITECTURAL DECISIONS -->
       Kernel is now the superboss : it identity-maps all of RAM.
       Every physical page becomes reachable at the same VA, so the kernel can
       touch any page, including pages later handed to extensions.

       The protection that remains : extensions still only see their own pages in
       their own root table, so an extension cannot reach kernel memory. The
       reverse (kernel reading extension memory) is now possible by design.
    */

    /*
    Fixed leaf attributes, set once. Only bits 12 to 47 (physical address) change per page.
       bit 0      : valid
       bit 1      : type = 1 (at level 3 this means "this is a page")
       bits 2-4   : memory_type = 0 (normal RAM)
       bit 5      : non_secure = 0
       bit 6      : AP[1] = 0 (kernel only)
       bit 7      : AP[2] = 0 (read/write)
       bits 8-9   : shareability = 3 (inner shareable)
       bit 10     : access_flag = 1 (mandatory)
       bit 11     : not_global = 0 (kernel mapping is global)
    */
    uint64_t page_attributes_register = 0 ;
    page_attributes_register |= (1ULL << 0) ;
    page_attributes_register |= (1ULL << 1) ;
    page_attributes_register |= (0ULL << 2) ;
    page_attributes_register |= (0ULL << 5) ;
    page_attributes_register |= (0ULL << 6) ;
    page_attributes_register |= (0ULL << 7) ;
    page_attributes_register |= (3ULL << 8) ;
    page_attributes_register |= (1ULL << 10) ;
    page_attributes_register |= (0ULL << 11) ;

    /*
    Step --> 1
      Extract the kernel's root page table by matching pid == 0.
    */
    uint32_t root_page_table_index = 0 ;
    int  found_kernel_root    = 0 ;

    for(int i = 0 ; i < out_info->max_processes ; i++)
    {
        if(root_pte_array[i].pid == 0)
        {
            root_page_table_index = root_pte_array[i].page_index ;
            found_kernel_root     = 1 ;
            break;
        }
    }

    if(!found_kernel_root)
    {
        return;
    }

    /*
     Step --> 2
       Identity map every page in RAM. One pass is enough.

     
    */
    uint64_t total_pages = out_info->ram_size / out_info->min_mem_allocation ;

    for(uint64_t page_index = 0 ; page_index < total_pages ; page_index++)
    {
        uint64_t virtual_address = out_info->ram_base_address + page_index * out_info->min_mem_allocation ;

        // Only bits 12 to 47 change per page, everything else stays from the template.
        uint64_t this_attrs = page_attributes_register | (virtual_address & 0x0000FFFFFFFFF000ULL) ;

        pte_population(out_info->ram_base_address , root_page_table_index , virtual_address , this_attrs , out_info->min_mem_allocation , bitmap , size_bitmap) ;
    }


        /*
    Step --> 3 
    Now we will idenity map the mmio of all devices to the kernel ! 
    UART (PL011) is hardcoded for now at 0x09000000 , size 0x1000 , because
    it must be mapped before MMU enable or the first print faults.
    */

    uint64_t device_mmio_attrs = 0 ;
    device_mmio_attrs |= (1ULL << 0) ;    // valid
    device_mmio_attrs |= (1ULL << 1) ;    // type = page
    device_mmio_attrs |= (1ULL << 2) ;    // memory_type = 1 (device)
    device_mmio_attrs |= (0ULL << 6) ;    // AP[1] = kernel only
    device_mmio_attrs |= (0ULL << 7) ;    // AP[2] = read/write
    device_mmio_attrs |= (0ULL << 8) ;    // shareability = 0
    device_mmio_attrs |= (1ULL << 10) ;   // access_flag
    device_mmio_attrs |= (1ULL << 53) ;   // PXN, don't execute from MMIO
    device_mmio_attrs |= (1ULL << 54) ;   // UXN, don't execute from MMIO

    // Hardcoded UART MMIO
    {
        uint64_t uart_base = 0x09000000ULL ;
        uint64_t uart_size = 0x00001000ULL ;

        uint64_t pages_in_region = (uart_size + out_info->min_mem_allocation - 1) / out_info->min_mem_allocation ;

        for(uint64_t p = 0 ; p < pages_in_region ; p++)
        {
            uint64_t virtual_address = uart_base + p * out_info->min_mem_allocation ;
            uint64_t this_attrs = device_mmio_attrs | (virtual_address & 0x0000FFFFFFFFF000ULL) ;

            pte_population(out_info->ram_base_address , root_page_table_index , virtual_address , this_attrs , out_info->min_mem_allocation , bitmap , size_bitmap) ;
        }
    }

    // Device MMIO from DTB
    for(int i = 0 ; i < out_info->device_count ; i++)
    {
        struct device_entry *dev = &out_info->devices[i] ;

        for(int j = 0 ; j + 1 < dev->mmio_base_size_array_count ; j += 2)
        {
            uint64_t region_base = dev->mmio_base_size_array[j] ;
            uint64_t region_size = dev->mmio_base_size_array[j + 1] ;

            uint64_t pages_in_region = (region_size + out_info->min_mem_allocation - 1) / out_info->min_mem_allocation ;

            for(uint64_t p = 0 ; p < pages_in_region ; p++)
            {
                uint64_t virtual_address = region_base + p * out_info->min_mem_allocation ;
                uint64_t this_attrs = device_mmio_attrs | (virtual_address & 0x0000FFFFFFFFF000ULL) ;

                pte_population(out_info->ram_base_address , root_page_table_index , virtual_address , this_attrs , out_info->min_mem_allocation , bitmap , size_bitmap) ;
            }
        }
    }


}