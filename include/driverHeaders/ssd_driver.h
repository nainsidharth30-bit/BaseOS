#ifndef SSD_DRIVER_H
#define SSD_DRIVER_H

#include<stdint.h>

 extern int (*ssd_driver)(uintptr_t sector_address , uintptr_t buffer_base , uintptr_t buffer_size , uintptr_t mmio_base , uintptr_t mmio_size) ;

#endif