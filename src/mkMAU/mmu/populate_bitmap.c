
#include"../../../include/lib/dbt.h"
#include"../../../include/driverHeaders/uart.h"
#include<stdint.h>
#include<stddef.h>


void populate_bitmap(uintptr_t bitmap , uint32_t bitmap_size  , uint64_t* array_of_page_numbers  , uint32_t size_of_array_of_page_numbers  , uint8_t free_or_occupied ) 
{
     /*
     Because we need to update the bitmap regulary , hence we are asking for very specific things inside arguemnts of this function so that we can use this same function for every updation ! 
     free_or_occupied -- > Tells the value to fill in bits , either zero or one  , 1 -- > Free , 0 -- > Occupied ! 

     */

     uint8_t* bitmap_bytes = (uint8_t*)bitmap ; 

     for(uint32_t i=0 ; i<size_of_array_of_page_numbers ; i++)
     {
        uint64_t page_value = array_of_page_numbers[i] ; 
        // Page_value here tells us , which bit number one should change  , 0th bit starts from the bitmap and last bit is at bitmap+size of bitmap ! 

        // First we will find or go to that particular byte which stores that particular bit which represents the particular page_value page 
        uint64_t byte_index = page_value/8 ; 

        // Than we find which bit inside that byte represents the page ! 
        uint8_t  bit_index = page_value%8 ; 

        // This page number is outside the bitmap , we never touch memory outside the bitmap ! 
        if(byte_index >= bitmap_size)
        {
            continue ; 
        }

        if(free_or_occupied)
        {
            bitmap_bytes[byte_index] |= (uint8_t)(1 << bit_index) ;      // 1 -- > Free 
        }
        else
        {
            bitmap_bytes[byte_index] &= (uint8_t)~(1 << bit_index) ;     // 0 -- > Occupied 
        }
     }
}