#ifndef DEVICES_SETUP_H
#define DEVICES_SETUP_H

#include"../lib/dbt.h"
#include<stdint.h>
#include"ic_bpt.h"

 

void devices_setup(struct hardware_info* out_info , uint16_t * peripheral_index_array , uint8_t peripheral_index_array_count , uint16_t* ic_index_array , uint8_t ic_index_array_count );

#endif