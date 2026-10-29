#ifndef META_EXT_H
#define META_EXT_H
#include<stdint.h>
#include<stddef.h>

 struct meta_ext{

    const char extension_name[64] ; 
    /*
    The value extension type tells about the main work or say type of extension like if the extension is a virtual machine or a driver ! 
     1 -- > Driver , 2 -- > Virtual Machine and so on , we can decide that thing later as we proceed further in our work ! 
    */
    uint32_t extension_type ; 

    /* Tells if the extension should be loaded automatically at boot or not ! 0 --> no , 1 --> Yes !*/

    uint8_t load_at_boot_extension ; 

    /*
    Gives the sector number where the extension is actually present ! 
    */

    uint64_t sector_number ; 
    /* Sector offset for placement of extension inside the sector ! */
    uint32_t byte_offset ; 

    /* Size of extension in bytes !*/

    uint32_t size_of_extension ; 

    /*
    The controller it belongs to !
    */

    uint32_t uniq_id  ; 

    /* 
 we can add more properties later based on requirement !
*/


 } ; 



#endif