#include<stdint.h>
#include<stddef.h>

/*
The work of this function is to calculate the page number  , because our reserved regions , the kernel ranges and if any other thing are not necessarily aligned at min_mem_allocation bytes 
hence we need to find those pages which engulf them and those pages must be min_mem_allocation byte aligned ! 
Because these memory ranges are continous , from start to end , we will return the total number of pages and will update the pointer which stores the base page number 
Page numbers start from ram base , page 0 is the first page of ram ! 
end is the address just after the last byte of the region ! 
*/

uint32_t page_collect(uint64_t start , uint64_t end , uint32_t* base_page_number , uint32_t alignment_value , uint64_t ram_base , uint64_t ram_end)  
{
        // Alignment value is min_mem_allocation value actually ! 

        // Empty region or a region completely outside ram , nothing to collect ! 
        if(end <= start || end <= ram_base || start >= ram_end)
        {
            *base_page_number = 0 ; 
            return 0 ; 
        }

        // Clamping the region inside ram so that unsigned subtraction never wraps around ! 
        if(start < ram_base)
        {
            start = ram_base ; 
        }
        if(end > ram_end)
        {
            end = ram_end ; 
        }

        // Start goes down to the page which contains it , so the first page is ( start - ram_base ) / alignment rounded down ! 
        *base_page_number = (start - ram_base)/alignment_value ; 

        // End goes up to the page boundary after it , so the partly used last page is also included ! 
        uint32_t page_after_last = (end - ram_base + alignment_value - 1)/alignment_value ; 

        // Total pages engulfing the region ! 
        return page_after_last - *base_page_number ; 
}