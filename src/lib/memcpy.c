
#include<stdint.h>

    void memcopy(void * destination , const void* source , uint32_t total_bytes)
    {
        uint8_t* tracker_pointer_destination = ( uint8_t*)destination ;
        uint8_t* tracker_pointer_source = ( uint8_t*)source;

        uint8_t i=0;
        while(i<total_bytes)
        {
            tracker_pointer_destination[i] = tracker_pointer_source[i];
            i++;
        }

    }