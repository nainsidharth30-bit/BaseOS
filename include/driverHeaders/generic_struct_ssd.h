#ifndef SSD_REQUEST_H
#define SSD_REQUEST_H

#include <stdint.h>


struct ssd_request_bpt {
    uintptr_t mmio_base;           
    uintptr_t mmio_size;          
    uintptr_t sector_address;      
    uintptr_t ram_buffer_address;  
    uintptr_t queue_base;         
    uintptr_t queue_size;         
};

/* Return codes of virtio_ssd_driver() */
#define SSD_OK            0   /* sector is now in ram_buffer_address        */
#define SSD_ERR_INIT     -1   /* controller handshake failed                */
#define SSD_ERR_QUEUE    -2   /* queue memory too small/misaligned, or the  */
                              /* controller supports too small a queue      */
#define SSD_ERR_TIMEOUT  -3   /* controller never answered                  */
#define SSD_ERR_IO       -4   /* controller answered with an error status   */

#endif