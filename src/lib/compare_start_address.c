#include<stdint.h>

#include "../../include/lib/compare_start_address.h"

#include "../../include/mkMAU/mkMAU.h"

#include "../../include/driverHeaders/uart.h"




int compare_start_address(const void *a, const void *b)
{
    const struct mkmau_node *node_a = (const struct mkmau_node *)a;
    const struct mkmau_node *node_b = (const struct mkmau_node *)b;

    if (node_a->base_range < node_b->base_range) return -1;
    if (node_a->base_range > node_b->base_range) return 1;
    return 0;
}