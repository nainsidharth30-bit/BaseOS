#ifndef ENABLE_IRQ_H
#define ENABLE_IRQ_H
#include"../lib/dbt.h"
#include<stdint.h>
#include"ic_bpt.h"

void enable_irq_peripheral(struct hardware_info* out_info , uint16_t* peripheral_index_array ,uint8_t peripheral_index_array_count ,uint16_t* ic_index_array ,uint8_t ic_index_array_count );


#endif