#include<stdint.h>
#include "../../include/lib/dbt.h"
#include "../../include/lib/quicksort.h"
#include "../../include/mkMAU/mkMAU.h"
#include<string.h>
#include "../../include/driverHeaders/uart.h"
#include "../../include/lib/alignbyte.h"



#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009
 void traverse_totalsize (uintptr_t dbt_tree_ptr ,struct hardware_info *out_info);
void traverse_off_dt_struct(uintptr_t dbt_tree_ptr, struct hardware_info *out_info);
void traverse_reserved_regions_in_off_dt_struct (uintptr_t dbt_tree_ptr , struct hardware_info *out_info);
  void discard_dbt(uintptr_t dbt_tree_ptr , uintptr_t dbt_size);
  void traverse_off_mem_rsvmap(uintptr_t dbt_tree_ptr, struct hardware_info *out_info);


int compare_start_address(const void *a, const void *b)
{
    const struct mkmau_node *node_a = (const struct mkmau_node *)a;
    const struct mkmau_node *node_b = (const struct mkmau_node *)b;

    if (node_a->base_range < node_b->base_range) return -1;
    if (node_a->base_range > node_b->base_range) return 1;
    return 0;
}










 char* parsing_node_name(uint8_t** one_byte_tracker_pointer)
{
    char *node_name = (char *)(*one_byte_tracker_pointer) ; 
    while(**one_byte_tracker_pointer!='\0')
    {
        (*one_byte_tracker_pointer)++;
    }
     (*one_byte_tracker_pointer)++;
   
     /* PLACE 1: Pad past variable-length node name */
     byte_alignment((void **)one_byte_tracker_pointer);

     return node_name ;
}

     

int extracting_dbt_info(uintptr_t dbt_tree_ptr , struct hardware_info *out_info )
{
        //  if(!dbt_tree_ptr)
        //  {
        //     return -1 ;
        //  } 

        
        traverse_off_dt_struct(dbt_tree_ptr , out_info);
        traverse_off_mem_rsvmap(dbt_tree_ptr,out_info);
         traverse_reserved_regions_in_off_dt_struct ( dbt_tree_ptr , out_info);
         array_sort(out_info->rsv_regions , sizeof(struct reserved_region) ,out_info->rsv_count ,compare_start_address);
        traverse_totalsize(dbt_tree_ptr,out_info);
     


}

  void traverse_off_dt_struct(uintptr_t dbt_tree_ptr ,struct hardware_info *out_info)
  {      
    uart_puts("\n  traversing dbt dtruct \n");
    struct flattened_device_tree_header *fdt = (struct flattened_device_tree_header*)dbt_tree_ptr ;  
           uint32_t offset_for_off_dt_struct  =  __builtin_bswap32((fdt->off_dt_struct) ) ;
             uint8_t* one_byte_tracker_pointer = (uint8_t *)dbt_tree_ptr+offset_for_off_dt_struct ;
             uint32_t offset_for_dt_string = __builtin_bswap32(fdt->off_dt_strings);
             uint8_t is_memory_flag = 0 ;
             
           while(1)
           {
               uint32_t bigE_token = *(uint32_t*)one_byte_tracker_pointer ;
               uint32_t real_token = __builtin_bswap32(bigE_token);
                 one_byte_tracker_pointer = one_byte_tracker_pointer+4;
               if(real_token == FDT_BEGIN_NODE)
               {
                 uart_puts("true fdt begin");
              char* node_name =        parsing_node_name(&one_byte_tracker_pointer)     ;


              if (node_name[0] == 'm' && 
    node_name[1] == 'e' && 
    node_name[2] == 'm' && 
    node_name[3] == 'o' && 
    node_name[4] == 'r' && 
    node_name[5] == 'y')
{
                   is_memory_flag =1 ;
                  
}


               } 
               else if(real_token==FDT_PROP)
               {

                uint32_t len = __builtin_bswap32(*(uint32_t*)one_byte_tracker_pointer);
                one_byte_tracker_pointer=one_byte_tracker_pointer+4 ;
                uint32_t nameoff = __builtin_bswap32(*(uint32_t*)one_byte_tracker_pointer);
                one_byte_tracker_pointer=one_byte_tracker_pointer+4 ;
                if(is_memory_flag)
                {
                  

                  
                 const char* prop_name = (char*)((uint8_t*)dbt_tree_ptr+nameoff+offset_for_dt_string);
if (prop_name[0] == 'r' && prop_name[1] == 'e' && prop_name[2] == 'g' && prop_name[3] == '\0')
{  
    uint32_t *reg_data = (uint32_t*)one_byte_tracker_pointer;
    
    // Read 64-bit Base Address (cells 0 and 1)
    uint64_t base_high_bits = __builtin_bswap32(reg_data[0]);
    uint64_t base_low_bits = __builtin_bswap32(reg_data[1]);
    uint64_t ram_base = (base_high_bits << 32) | base_low_bits;

    // Read 64-bit Size (cells 2 and 3)
    uint64_t size_high_bits = __builtin_bswap32(reg_data[2]);
    uint64_t size_low_bits = __builtin_bswap32(reg_data[3]);
    uint64_t ram_size = (size_high_bits << 32) | size_low_bits;

    out_info->ram_base_address = ram_base;
    out_info->ram_size = ram_size;

    break;
}

                

                }

                /* PLACE 2: Skip property payload length AND align to next token */
              
                one_byte_tracker_pointer = one_byte_tracker_pointer + len;
                byte_alignment((void **)&one_byte_tracker_pointer);
                

               }


           }

           

  }

  void traverse_off_mem_rsvmap(uintptr_t dbt_tree_ptr, struct hardware_info *out_info)
  {      
    
    // Resereved regions it will give are 4 because other remaining 3 are part of dt struct offset as fdt nodes !
    
    struct flattened_device_tree_header *fdt = (struct flattened_device_tree_header*)dbt_tree_ptr ;  
          uart_puts("\n We are finding reserved regions \n");
         uint32_t offset_for_off_mem_rsvmap = __builtin_bswap32(fdt->off_mem_rsvmap);
         uint8_t* reserved_region_array_tracker = (uint8_t*)(dbt_tree_ptr+offset_for_off_mem_rsvmap);
         int total_numbers_of_rserved_regions = 0;
         int iterator =0 ;
         while(1)
         {
          
          uint32_t first_half_address = __builtin_bswap32(*(uint32_t*)reserved_region_array_tracker);
          
          reserved_region_array_tracker=reserved_region_array_tracker+4;
           
          uint32_t second_half_address =  __builtin_bswap32(*(uint32_t*)reserved_region_array_tracker);
          
          reserved_region_array_tracker=reserved_region_array_tracker+4;
         
          uint64_t reserved_region_address = ((uint64_t)first_half_address << 32) | second_half_address;

          uint32_t first_half_size =  __builtin_bswap32(*(uint32_t*)reserved_region_array_tracker);
          
          reserved_region_array_tracker=reserved_region_array_tracker+4;
           
          uint32_t second_half_size =  __builtin_bswap32(*(uint32_t*)reserved_region_array_tracker);
           
          reserved_region_array_tracker=reserved_region_array_tracker+4;
           
          uint64_t reserved_region_size = ((uint64_t)first_half_size << 32) | second_half_size;

          uint64_t reserved_region_end_address = reserved_region_address+reserved_region_size;

      // uart_puts("\nstarting address------>");
      // uart_puthex(reserved_region_address);
      //   uart_puts("\nending address------>");
      //   uart_puthex(reserved_region_end_address);

          if(reserved_region_address==0 && reserved_region_size==0)
          {
            uart_puts("\n all regions found , total are \n");
            uart_puthex(out_info->rsv_count);
            break;
          }

          if(total_numbers_of_rserved_regions<MAX_RESERVED_REGIONS)
          {
            out_info->rsv_regions[total_numbers_of_rserved_regions].start=reserved_region_address;
            out_info->rsv_regions[total_numbers_of_rserved_regions].end=reserved_region_end_address;
            total_numbers_of_rserved_regions++;
          }
          out_info->rsv_count=total_numbers_of_rserved_regions;
//           uart_puts("\nTotal off_mem_rsvmap regions found: ");
// uart_puthex(out_info->rsv_count);

     
         }
         
  }
void  traverse_totalsize (uintptr_t dbt_tree_ptr ,struct hardware_info *out_info)
  {
      struct flattened_device_tree_header *fdt = (struct flattened_device_tree_header*)dbt_tree_ptr ; 
      uint32_t total_size = __builtin_bswap32(fdt->totalsize) ;
      discard_dbt(dbt_tree_ptr,total_size);
      
  }

void discard_dbt(uintptr_t dbt_tree_ptr, uintptr_t dbt_size)
{
    uint8_t *zero_filler = (uint8_t *)dbt_tree_ptr;
    for (size_t i = 0; i < dbt_size; i++)
    {
        zero_filler[i] = 0;
    }
}




void traverse_reserved_regions_in_off_dt_struct (uintptr_t dbt_tree_ptr , struct hardware_info *out_info)
{
    struct flattened_device_tree_header *fdt = (struct flattened_device_tree_header *)dbt_tree_ptr;
      uint32_t offset_for_dt_struct = __builtin_bswap32(fdt->off_dt_struct);     
     uint8_t * one_byte_tracker_pointer = (uint8_t*)( dbt_tree_ptr + offset_for_dt_struct);
     uint32_t offset_for_dt_string = __builtin_bswap32(fdt->off_dt_strings);

     size_t resereved_memory_flag = 0 ;

     while (1)
     {

          uint32_t token = __builtin_bswap32(*(uint32_t *)one_byte_tracker_pointer);
          one_byte_tracker_pointer=one_byte_tracker_pointer+4;
 
          if(token == FDT_BEGIN_NODE)
          {

             char* node_name = parsing_node_name(&one_byte_tracker_pointer);

             if(node_name[0]=='r'&&node_name[1]=='e'&&node_name[2]=='s'&&node_name[3]=='e'&&node_name[4]=='r'&&node_name[5]=='v'&&node_name[6]=='e'&&node_name[7]=='d'&&node_name[8]=='-'&&node_name[9]=='m'&&node_name[10]=='e'&&node_name[11]=='m'&&node_name[12]=='o'&&node_name[13]=='r'&&node_name[14]=='y'&&node_name[15]=='\0')
             {
              resereved_memory_flag=1;
             }
             

          }
          else if(token == FDT_PROP)
          {
               uint32_t len = __builtin_bswap32(*(uint32_t*)one_byte_tracker_pointer);
               one_byte_tracker_pointer=one_byte_tracker_pointer+4;
               uint32_t nameoff = __builtin_bswap32(*(uint32_t *)one_byte_tracker_pointer);
               one_byte_tracker_pointer=one_byte_tracker_pointer+4;

               if(resereved_memory_flag && len==16)
               {

                   const char* prop_name = (char*)((uint8_t*)dbt_tree_ptr+nameoff+offset_for_dt_string);
                   if(prop_name[0] == 'r' && prop_name[1] == 'e' && prop_name[2] == 'g' && prop_name[3] == '\0')
                   {
                          
                    uint32_t first_half_address = __builtin_bswap32(*(uint32_t *)one_byte_tracker_pointer);
                    one_byte_tracker_pointer=one_byte_tracker_pointer+4;
                    uint32_t second_half_address = __builtin_bswap32(*(uint32_t *)one_byte_tracker_pointer);
                     one_byte_tracker_pointer=one_byte_tracker_pointer+4;
                     uint64_t start_address_resevred_region = ((uint64_t)first_half_address<<32)|second_half_address;

                     uint32_t first_half_size = __builtin_bswap32(*(uint32_t *)one_byte_tracker_pointer);
                     one_byte_tracker_pointer=one_byte_tracker_pointer+4;
                     uint32_t second_half_size = __builtin_bswap32(*(uint32_t *)one_byte_tracker_pointer);
                     one_byte_tracker_pointer=one_byte_tracker_pointer+4;

                     uint64_t full_size = ((uint64_t)first_half_size<<32) | second_half_size ;

                     uint64_t end_address_resereved_region = start_address_resevred_region+full_size;

                     int current_rsv_array_size = out_info->rsv_count;


                      out_info->rsv_regions[current_rsv_array_size].start=start_address_resevred_region;
                      out_info->rsv_regions[current_rsv_array_size].end=end_address_resereved_region;

                      current_rsv_array_size++;
                      out_info->rsv_count=current_rsv_array_size;
                     

                   }

                   one_byte_tracker_pointer=one_byte_tracker_pointer+len;
                   byte_alignment((void **)&one_byte_tracker_pointer);

               }

          }
          else if(token == FDT_END)
          {
            break;
          }



     }
  
     uart_puts("\n Total Number of Reserved Regions Found are ======> \n");
     uart_puthex(out_info->rsv_count);

    }
