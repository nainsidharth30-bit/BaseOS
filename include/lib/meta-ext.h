#ifndef META-EXT_H
#define META-EXT_H
#include<stdint.h>
#include<stddef.h>

struct meta_ext {
    const char* name ;
    uint32_t ssd_sectore_address ;
    uint32_t size_in_bytes ;
    
} ;

struct meta_ext available_extensions_array []={
    {
       .name = "first_extension",
        .ssd_sectore_address = 1024,
        .size_in_bytes = 4096,
    },
    {
        .name = "second_extension",
        .ssd_sectore_address = 2048,
        .size_in_bytes = 4096
    }
} ;

const int extension_count = sizeof(available_extensions_array) / sizeof(available_extensions_array[0]);



#endif