#ifndef AHCI_H
#define AHCI_H
#include<stdint.h>
#include<stddef.h>
#include"generic_struct_ssd.h"


int ahci_driver( struct ssd_request_bpt * bpt);



#endif