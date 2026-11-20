#include"../../../include/mkMAU/mmu/set_root_page_table_mmu.h"
#include"../../../include/lib/dbt.h"
#include"../../../include/lib/setzero.h"
#include"../../../include/driverHeaders/uart.h"
#include"../../../include/mkMAU/mmu/root_pte.h"
#include"../../../include/mkMAU/mkmau_utils.h"
#include<stdint.h>


uint32_t set_pid( struct root_pte_entry * root_pte_array , uint32_t root_pte_array_count);
uint8_t check_if_pid_alive( struct root_pte_entry * root_pte_array , uint32_t root_pte_array_count );


/*
Before loading any extension , any application , its process id and root page table entry must be assigned first and that is what this function do ! 
Set stage returns the updated root pte array count ! 
*/

uint32_t set_stage(uintptr_t bitmap , uint32_t size_bitmap , struct hardware_info* out_info , struct root_pte_entry * root_pte_array , uint32_t root_pte_array_count   )
{
   /*
    Step --> 1 
    Assign it the process id ! 
   */

   uint32_t temp_pid = set_pid(root_pte_array,root_pte_array_count) ; 

   /*
   Step --> 2 
   Find a free Page which will be used as its root page table entry !
   */

     uint32_t page_index =    allocate_page(bitmap,size_bitmap,1,0) ; 



     /*
     Step --> 3 
     Put these two cpmbinations inside root_pte_array and than that extension or app can be loaded easily ! 
     */

    root_pte_array[root_pte_array_count].pid = temp_pid ; 
    root_pte_array[root_pte_array_count].page_index = page_index ; 

    root_pte_array_count++ ; 

    return root_pte_array_count ; 

}
uint32_t set_pid( struct root_pte_entry * root_pte_array , uint32_t root_pte_array_count)
{
     extern uint32_t last_assigned_pid ;  
     
     last_assigned_pid++ ; 

     if(last_assigned_pid==0)
     {
        last_assigned_pid++;
     }

   if(check_if_pid_alive(root_pte_array,root_pte_array_count))
   {
          return set_pid(root_pte_array,root_pte_array_count) ; 
   }
   else
   {
    // Free and Valid PID ! 
    return last_assigned_pid ; 
   }

}

uint8_t check_if_pid_alive( struct root_pte_entry * root_pte_array , uint32_t root_pte_array_count )
{
    extern uint32_t last_assigned_pid ; 

    for(uint32_t i =0 ; i<root_pte_array_count ; i++)
    {
        
        if(last_assigned_pid == root_pte_array[i].pid)
        {
            return 1 ; 
        }
    }

    return 0 ; 
}