
#include "../../include/lib/dbt.h"
#include<stdint.h>

void reserved_region_ranges_size(struct hardware_info* out_info , uint64_t * size_of_reserved_regions )
{
    *size_of_reserved_regions =0;
     
    for(int i=0 ; i<out_info->rsv_count ; i++)
    {
      
         *size_of_reserved_regions=*size_of_reserved_regions+(out_info->rsv_regions[i].end - out_info->rsv_regions[i].start);

    }
}