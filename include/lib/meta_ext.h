#ifndef META_EXT_H
#define META_EXT_H

#include <stdint.h>

struct __attribute__((packed)) meta_ext{

    char     extension_name[64] ;        // ASCII , zero padded 
    uint32_t extension_type ;            // 0 = SSD driver 
    uint8_t  load_at_boot_extension ;    // 1 = load at boot 
    uint64_t sector_number ;             // where the extension's raw bytes start 
    uint32_t byte_offset ;               // always 0 for now 
    uint32_t size_of_extension ;
    uint32_t uniq_id ;                   // which SSD controller this belongs to 

} ; 



#endif