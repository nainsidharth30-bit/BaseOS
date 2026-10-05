#ifndef VIRTIO_H
#define VIRTIO_H
#include<stdint.h>
#include<stddef.h>


int virtio_ssd_driver(uintptr_t sector_address , uintptr_t buffer_base , uintptr_t buffer_size , uintptr_t mmio_base , uintptr_t mmio_size);



#endif