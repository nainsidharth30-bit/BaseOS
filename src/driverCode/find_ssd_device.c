#include "../../include/lib/dbt.h"
#include "../../include/driverHeaders/uart.h"
#include "../../include/driverHeaders/device_indexes.h"
#include "../../include/driverHeaders/ssd_driver.h"
#include "../../include/string_utility.h"
#include "../../include/driverHeaders/virtIO.h"
#include<stdint.h>
#include<string.h>



// void discover_ssd_device(struct hardware_info* out_info) ;

void dump_hex(uintptr_t address, uint32_t length);


/* This function is used for */
void discover_ssd_device(struct hardware_info *out_info)
{
    struct device_entry *ssd = &out_info->devices[SSD_DEVICE_INDEX];

   
    if (ssd->compatible[0] == '\0') {
        return;
    }

    if (str_eq(ssd->compatible, "virtio,mmio")) {

      extern   uint32_t max_queue_size; 

   extern  uintptr_t ssd_queue_address ;
 
    extern    void * kernel_buffer_address  ;
    extern   uint32_t ssd_sector_size ; 

        struct ssd_request_bpt bpt = {0};

        bpt.mmio_base          = ssd->mmio_base_address;
        bpt.mmio_size          = ssd->mmio_size;
        bpt.queue_base         = ssd_queue_address;  
        bpt.queue_size         = max_queue_size ; 
        bpt.ram_buffer_address = (uintptr_t)kernel_buffer_address;
        bpt.sector_address     = 1024;               

        virtio_ssd_driver(&bpt);

        dump_hex((uintptr_t)kernel_buffer_address ,ssd_sector_size );
    
    }
}


void dump_hex(uintptr_t address, uint32_t length)
{
    const volatile uint8_t *ptr = (const volatile uint8_t *)address;

    for (uint32_t i = 0; i < length; i++) {
        /* Every 16 bytes, print an address label and start a new line */
        if ((i & 0x0F) == 0) {
            uart_puts("\n");
            uart_puthex(address + i);
            uart_puts(": ");
        }

        uint8_t byte = ptr[i];

        /* high nibble */
        uart_putc("0123456789ABCDEF"[(byte >> 4) & 0x0F]);
        /* low nibble */
        uart_putc("0123456789ABCDEF"[byte & 0x0F]);
        uart_putc(' ');
    }

    uart_puts("\n");
}
   






