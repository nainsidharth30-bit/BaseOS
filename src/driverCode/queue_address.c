#include "../../include/driverHeaders/uart.h"
#include "../../include/driverHeaders/queue_address.h"
void extract_virtio_queue_address(uintptr_t mmio_base , uintptr_t mmio_size)
{
     uintptr_t offset_virt_queue = 0x034 ;
     uintptr_t offset_virtio_sector_size = 0x114 ;
  
    extern  uint32_t  max_queue_size     ;
     max_queue_size     = *(volatile uint32_t *)(mmio_base + offset_virt_queue)*26;

     uart_puts("\n----------------------------------------------------\n");
    uart_puthex(max_queue_size);
     uart_puts("\n----------------------------------------------------\n");

     extern uintptr_t   ssd_sector_size ;
     ssd_sector_size = *(volatile uint32_t *)(mmio_base + offset_virtio_sector_size);
       uart_puts("\n----------------------------------------------------\n");
    uart_puthex(ssd_sector_size);
     uart_puts("\n----------------------------------------------------\n");

   
}