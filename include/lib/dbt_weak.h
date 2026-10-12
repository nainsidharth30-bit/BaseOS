#ifndef DBT_WEAK_H
#define DBT_WEAK_H

#include<stdint.h>
#include "../../include/lib/dbt.h"


int extracting_dbt_info(uintptr_t dbt_tree_ptr , struct hardware_info *out_info );

#endif