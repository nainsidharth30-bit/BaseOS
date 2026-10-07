#ifndef QUEUE_ADDRESS_H
#define QUEUE_ADDRESS_H
#include "stdint.h"

void extract_virtio_queue_address(uintptr_t mmio_base , uintptr_t mmio_size);

#endif