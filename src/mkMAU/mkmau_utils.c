#include<stdint.h>
#include"../../include/mkMAU/mkmau_utils.h"



int compare_start_address(const void *a, const void *b)
{
    const struct mkmau_node *node_a = (const struct mkmau_node *)a;
    const struct mkmau_node *node_b = (const struct mkmau_node *)b;

    if (node_a->base_range < node_b->base_range) return -1;
    if (node_a->base_range > node_b->base_range) return 1;
    return 0;
}
/*
This function always allocate one single Page only , it starts searching from start_page ! 
and if bit_set_flag is 1 , before returning the page number , it makes that occupied also ! 
*/

uint32_t allocate_page(uintptr_t bitmap , uint32_t size_bitmap , uint8_t bit_set_flag , uint32_t start_page )
{
   uint8_t* bytes = (uint8_t*)bitmap ; 

   for(uint32_t page = start_page ; page < size_bitmap*8 ; page++)
   {
      // Byte value 0 means all 8 pages of this byte are occupied , skipping the whole byte ! 
      if(bytes[page/8] == 0)
      {
          page = page | 7 ;     // Jumping to the last page of this byte , the loop does page++ after it ! 
          continue ; 
      }

      if((bytes[page/8] >> (page%8)) & 1)
      {
          // 1 means free ! 
          if(bit_set_flag) 
          {
               bytes[page/8] &= (uint8_t)~(1 << (page%8)) ; 
          }
          return page ; 
      }
   }

   return 0xFFFFFFFF ; // No memory is free  ! 
}

/*
This function wraps the allocate page function and gives multiple contiguous free pages ! 
Returns Base Page Number 
*/

uint32_t free_contiguous_pages_wrapper(uint16_t* next_pages  , uint32_t required_pages , uintptr_t bitmap , uint32_t size_bitmap )
{


           uint8_t* bytes = (uint8_t*)bitmap ;     // We will use it when we need to set bits on bitmap !



       uint32_t search_from = 0 ; 
       *next_pages = 0 ; 

       if(required_pages == 0)
       {
           return 0xFFFFFFFF ; 
       }

       while(1)
       {
           // Getting the first page index with bit_set_flag as false ! 
           uint32_t first_page_index = allocate_page(bitmap , size_bitmap , 0 , search_from) ; 
           if(first_page_index == 0xFFFFFFFF)
           {
               return 0xFFFFFFFF ;      // No free page left at all ! 
           }

           uint32_t pages = 1 ; 

           // Checking that the next pages are free , exactly one after another ! 
           while(pages != required_pages)
           {
               uint32_t get_page = allocate_page(bitmap , size_bitmap , 0 , first_page_index+pages) ; 
               if(get_page != first_page_index+pages)
               {
                   break ;      // Continuity broken ! 
               }
               pages++ ; 
           }

           if(pages == required_pages)
           {
                            // Enough continuous pages , marking all of them occupied now ! 
               for(uint32_t p = first_page_index ; p < first_page_index+required_pages ; p++)
               {
                   bytes[p/8] &= (uint8_t)~(1 << (p%8)) ; 
               }

               *next_pages = required_pages ; 
               return first_page_index ; 
           }

           // The next search starts after the page where continuity broke , not from the beginning ! 
           search_from = first_page_index+pages ; 
       }
}