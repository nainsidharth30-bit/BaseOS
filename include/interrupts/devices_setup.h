#ifndef DEVICES_SETUP_H
#define DEVICES_SETUP_H

#include"../lib/dbt.h"
#include<stdint.h>
#include"ic_bpt.h"

struct index_arrays {
   uint16_t idx_array[20];
   uint8_t  count ; 
} ; 

void devices_setup(struct hardware_info* out_info , struct index_arrays * arr  );

#endif