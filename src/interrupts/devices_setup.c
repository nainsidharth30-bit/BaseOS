
#include"../../include/interrupts/ic_bpt.h"
#include"../../include/lib/memcpy.h"
#include"../../include/lib/setzero.h"
#include"../../include/driverHeaders/uart.h"
#include"../../include/driverHeaders/track_stack.h"
#include"../../include/lib/dbt.h"
#include<stdint.h>

void devices_setup(struct hardware_info* out_info , uint16_t * peripheral_index_array , uint8_t peripheral_index_array_count , uint16_t* ic_index_array , uint8_t ic_index_array_count )
{
      for (int i=0 ; i<peripheral_index_array_count ; i++)
      {
          for(int j =0 ; j<ic_index_array_count ; j++)
          {
              if(out_info->devices[peripheral_index_array[i]].interrupt_controller_phandle == out_info->devices[ic_index_array[j]].phandle  )
              {
                 out_info->devices[peripheral_index_array[i]].ic_code = out_info->devices[ic_index_array[j]].ic_code ;
                 out_info->devices[peripheral_index_array[i]].interrupt_controller_phandle =(uint64_t) &out_info->devices[ic_index_array[j]] ;
                 break;
              }
          }
      }   
      
      uart_puts("\n Device Setup is succesfuoll ! ");
}