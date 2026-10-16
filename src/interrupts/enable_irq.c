#include"../../include/interrupts/enable_irq.h"
#include"../../include/interrupts/ic_bpt.h"
#include"../../include/lib/memcpy.h"
#include"../../include/lib/setzero.h"
#include"../../include/driverHeaders/uart.h"
#include"../../include/driverHeaders/track_stack.h"
#include<stdint.h>


void enable_irq_peripheral(struct hardware_info* out_info , uint16_t* peripheral_index_array ,uint8_t peripheral_index_array_count ,uint16_t* ic_index_array ,uint8_t ic_index_array_count )
{  uart_puts("I am in !\n");
      track_stack();
       struct ic_bpt bpt ={0};

      for (int i =0 ; i<peripheral_index_array_count ; i++)
      { 
            uart_puts("I am in !\n");
            memcopy(bpt.irq_cells , out_info->devices[peripheral_index_array[i]].irq_cells , sizeof(out_info->devices[peripheral_index_array[i]].irq_cells));
            bpt.number_of_irq_cells = out_info->devices[peripheral_index_array[i]].irq_cells_count ; 

            for(int j =0 ; j<ic_index_array_count;j++)
            {
                if(out_info->devices[peripheral_index_array[i]].interrupt_controller_phandle == out_info->devices[ic_index_array[j]].phandle)
                {
                     memcopy(bpt.mmio_base_mmio_size_pair_array, out_info->devices[ic_index_array[j]].mmio_base_size_array ,sizeof(out_info->devices[ic_index_array[j]].mmio_base_size_array));
                     bpt.total_number_of_elemnets = out_info->devices[ic_index_array[j]].mmio_base_size_array_count;
                     break;
                }
            }
            

            out_info->devices[peripheral_index_array[i]].ic_code(&bpt);
            set_zero(&bpt , sizeof(struct ic_bpt));
            
      }
}