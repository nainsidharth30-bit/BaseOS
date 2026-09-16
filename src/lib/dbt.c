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
void extract_ssd_irq_line_data(uint8_t *base_address, size_t len_to_read,uint32_t address_cells, uint32_t size_cells, struct hardware_info *out_info) ;
 void traverse_totalsize (uintptr_t dbt_tree_ptr ,struct hardware_info *out_info);
void dtb_structure_blocks_parser (uintptr_t dtb_tree_ptr , struct hardware_info *out_info);
  void discard_dbt(uintptr_t dbt_tree_ptr , uintptr_t dbt_size);
  void   traverse_off_mem_rsvmap(uintptr_t,struct hardware_info *out_info);
void extract_ram_info(uint8_t* base_address , size_t len_to_read , uint32_t address_cells , uint32_t size_cells , struct hardware_info* out_info);
void extract_ssd_mmio_size(uint8_t *base_address, size_t len_to_read,uint32_t address_cells, uint32_t size_cells, struct hardware_info *out_info,char compatibility[30]);

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
         if(!dbt_tree_ptr)
         {
            return -1 ;
         } 

        
        traverse_off_mem_rsvmap(dbt_tree_ptr,out_info);
       dtb_structure_blocks_parser(dbt_tree_ptr,out_info);
        array_sort(out_info->rsv_regions , sizeof(struct reserved_region) ,out_info->rsv_count ,compare_start_address);
       traverse_totalsize(dbt_tree_ptr,out_info);
     


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


void dtb_structure_blocks_parser (uintptr_t dtb_tree_ptr , struct hardware_info *out_info)
{
       struct flattened_device_tree_header *fdt = (struct flattened_device_tree_header *)dtb_tree_ptr ;
       
       uintptr_t base_of_structure_block = __builtin_bswap32(fdt->off_dt_struct)+dtb_tree_ptr;
      
       uintptr_t base_of_string_block = __builtin_bswap32(fdt->off_dt_strings)+dtb_tree_ptr ;

      uint8_t *one_byte_tracker_pointer = (uint8_t *)base_of_structure_block;

              uint32_t address_cells = 2;
        uint32_t size_cells = 2; // we use default value 

      

       //  -------- FLAG SECTION  ---------------------//
 
           

         int memory_node_flag = 0;
         int ssd_node_flag =0;


       //----------- FLAG SECTION OVER ---------------//

       //---------------- Buffers ----------------------//
         uint8_t *reg_buffer = NULL;
          size_t reg_len = 0;

             uint8_t *irq_line_buffer = NULL;
             size_t irq_line_len = 0;

             uint8_t * interrupt_parent_buffer = NULL;
             size_t interrupt_parent_len = 0;

             char compatible_buffer[30] = '\0';



       //----------------------------------Buffers Over --------//    

       while(one_byte_tracker_pointer < (uint8_t *)base_of_structure_block + __builtin_bswap32(fdt->size_dt_struct))
       {

        uint32_t token = __builtin_bswap32(*(uint32_t *)one_byte_tracker_pointer);
        one_byte_tracker_pointer=one_byte_tracker_pointer+4;

        if(token==FDT_BEGIN_NODE)
        {
          address_cells=2;
          size_cells=2;
          memory_node_flag = 0 ;
          ssd_node_flag=0;
          reg_buffer=NULL;
          reg_len=0;
          irq_line_buffer=NULL;
          irq_line_len=0;
          interrupt_parent_buffer=NULL;
          interrupt_parent_len=0;
          compatible_buffer[30] = '\0'; 
              while (*one_byte_tracker_pointer != '\0') 
              {
                one_byte_tracker_pointer++;
              }
    one_byte_tracker_pointer++;
    byte_alignment((void **)&one_byte_tracker_pointer);
        }

      else  if(token == FDT_PROP)
        {

    uint32_t len = __builtin_bswap32(*(uint32_t *)one_byte_tracker_pointer );
    one_byte_tracker_pointer=one_byte_tracker_pointer+4;
    uint32_t nameoff = __builtin_bswap32(*(uint32_t *)one_byte_tracker_pointer );
    one_byte_tracker_pointer=one_byte_tracker_pointer+4;

    const char* prop_name = (const char *)(base_of_string_block + nameoff);

    if(prop_name[0]=='#'&&prop_name[1]=='a'&&prop_name[2]=='d'&&prop_name[3]=='d'&&prop_name[4]=='r'&&prop_name[5]=='e'&&prop_name[6]=='s'&&prop_name[7]=='s'&&prop_name[8]=='-'&&prop_name[9]=='c'&&prop_name[10]=='e'&&prop_name[11]=='l'&&prop_name[12]=='l'&&prop_name[13]=='s'&&prop_name[14]=='\0')
    {
        address_cells = __builtin_bswap32(*(uint32_t*)one_byte_tracker_pointer);
        
    }

        if(prop_name[0]=='#'&&prop_name[1]=='s'&&prop_name[2]=='i'&&prop_name[3]=='z'&&prop_name[4]=='e'&&prop_name[5]=='-'&&prop_name[6]=='c'&&prop_name[7]=='e'&&prop_name[8]=='l'&&prop_name[9]=='l'&&prop_name[10]=='s'&&prop_name[11]=='\0')
    {
               size_cells = __builtin_bswap32(*(uint32_t*)one_byte_tracker_pointer);
        
    }

        if(prop_name[0]=='d'&&prop_name[1]=='e'&&prop_name[2]=='v'&&prop_name[3]=='i'&&prop_name[4]=='c'&&prop_name[5]=='e'&&prop_name[6]=='_'&&prop_name[7]=='t'&&prop_name[8]=='y'&&prop_name[9]=='p'&&prop_name[10]=='e'&&prop_name[11]=='\0')
    {
             if(len==7)
             {
              if(one_byte_tracker_pointer[0]=='m'&&one_byte_tracker_pointer[1]=='e'&&one_byte_tracker_pointer[2]=='m'&&one_byte_tracker_pointer[3]=='o'&&one_byte_tracker_pointer[4]=='r'&&one_byte_tracker_pointer[5]=='y'&&one_byte_tracker_pointer[6]=='\0')
              {
                  memory_node_flag = 1;
              }

             }
    }

        if(prop_name[0]=='c'&&prop_name[1]=='o'&&prop_name[2]=='m'&&prop_name[3]=='p'&&prop_name[4]=='a'&&prop_name[5]=='t'&&prop_name[6]=='i'&&prop_name[7]=='b'&&prop_name[8]=='l'&&prop_name[9]=='e'&&prop_name[10]=='\0')
    {
           if (len == 12 &&
        one_byte_tracker_pointer[0]=='v' &&one_byte_tracker_pointer[1]=='i' &&one_byte_tracker_pointer[2]=='r' &&one_byte_tracker_pointer[3]=='t' &&one_byte_tracker_pointer[4]=='i' && one_byte_tracker_pointer[5]=='o' &&one_byte_tracker_pointer[6]==',' &&one_byte_tracker_pointer[7]=='m' &&one_byte_tracker_pointer[8]=='m' &&one_byte_tracker_pointer[9]=='i' && one_byte_tracker_pointer[10]=='o' &&one_byte_tracker_pointer[11]=='\0')
    {
      compatible_buffer[30] = 'virtio,mmio';
        ssd_node_flag = 1;
    }
    }
      if(prop_name[0]=='r'&&prop_name[1]=='e'&&prop_name[2]=='g'&&prop_name[3]=='\0')
      {

             reg_buffer = one_byte_tracker_pointer ;
             reg_len = len ; 

      }
      if(prop_name[0]=='i'&&prop_name[1]=='n'&&prop_name[2]=='t'&&prop_name[3]=='e'&&prop_name[4]=='r'&&prop_name[5]=='r'&&prop_name[6]=='u'&&prop_name[7]=='p'&&prop_name[8]=='t'&&prop_name[9]=='s'&&prop_name[10]=='\0')
      {
            irq_line_buffer=one_byte_tracker_pointer;
            irq_line_len=len;
      }

      if(prop_name[0]=='i'&&prop_name[1]=='n'&&prop_name[2]=='t'&&prop_name[3]=='e'&&prop_name[4]=='r'&&prop_name[5]=='r'&&prop_name[6]=='u'&&prop_name[7]=='p'&&prop_name[8]=='t'&&prop_name[9]=='-'&&prop_name[10]=='p'&&prop_name[11]=='a'&&prop_name[12]=='r'&&prop_name[13]=='e'&&prop_name[14]=='n'&&prop_name[15]=='t'&&prop_name[16]=='\0')
      {
         interrupt_parent_buffer=one_byte_tracker_pointer;
         interrupt_parent_len=len;
      }

      



    one_byte_tracker_pointer = one_byte_tracker_pointer+len;
     byte_alignment((void **)&one_byte_tracker_pointer);


        }
        else if(token==FDT_NOP)
        {
          // Do nothing , I have no info
        }
        else if(token == FDT_END_NODE)
        {


          if(memory_node_flag && reg_buffer)
      {
        extract_ram_info(reg_buffer,reg_len,address_cells,size_cells,out_info);
      }

      if(ssd_node_flag && reg_buffer)
      {
        extract_ssd_mmio_size(reg_buffer,reg_len,address_cells,size_cells,out_info,compatible_buffer);
      }

      if(ssd_node_flag&&irq_line_buffer)
      {
        // Call a function
      }

      if(ssd_node_flag&&interrupt_parent_buffer)
      {
        //call a function
      }

          // I am the end of the node , Reset every variable 
          memory_node_flag = 0 ;
          ssd_node_flag=0;
          reg_buffer=NULL;
          reg_len=0;
          irq_line_buffer=NULL;
          irq_line_len=0;
          interrupt_parent_buffer=NULL;
          interrupt_parent_len=0;
          compatible_buffer[30] = '\0';
         
        }
        else
        {
          // I am the end of the whole DTB tree 
          break;
        }
               

       }
      

}

void extract_ram_info(uint8_t *base_address, size_t len_to_read,uint32_t address_cells, uint32_t size_cells, struct hardware_info *out_info)
{
    uint64_t ram_base = 0;
    uint64_t ram_size = 0;

    uint32_t total_cells = address_cells + size_cells;

    if (total_cells == 0)
    {
        return;
    }

    if (len_to_read < (size_t)(total_cells * 4))
    {
        return;
    }

    if (address_cells > 2)
    {
        address_cells = 2;
    }

    if (size_cells > 2)
    {
        size_cells = 2;
    }

    uint32_t iterator = 0;

             while (iterator < address_cells)
    {
        uint32_t one_cell = __builtin_bswap32(*(uint32_t *)(base_address + iterator * 4));
                ram_base = (ram_base << 32) | one_cell;
                 iterator++;
    }

               iterator = 0;

                  while (iterator < size_cells)
            {
                 uint32_t one_cell = __builtin_bswap32(*(uint32_t *)(base_address + address_cells * 4 + iterator * 4));
        ram_size = (ram_size << 32) | one_cell;
                iterator++;
         }

                    out_info->ram_base_address = ram_base;
                     out_info->ram_size = ram_size;
}

void extract_ssd_mmio_size(uint8_t *base_address, size_t len_to_read,uint32_t address_cells, uint32_t size_cells, struct hardware_info *out_info ,char compatibility[30])
{
      uint64_t mmio_base = 0;
    uint64_t mmio_size = 0;

    uint32_t total_cells = address_cells + size_cells;

    if (total_cells == 0)
    {
        return;
    }

    if (len_to_read < (size_t)(total_cells * 4))
    {
        return;
    }

    if (address_cells > 2)
    {
        address_cells = 2;
    }

    if (size_cells > 2)
    {
        size_cells = 2;
    }

    uint32_t iterator = 0;

             while (iterator < address_cells)
    {
        uint32_t one_cell = __builtin_bswap32(*(uint32_t *)(base_address + iterator * 4));
                mmio_base = (mmio_base << 32) | one_cell;
                 iterator++;
    }

               iterator = 0;

                  while (iterator < size_cells)
            {
                 uint32_t one_cell = __builtin_bswap32(*(uint32_t *)(base_address + address_cells * 4 + iterator * 4));
        mmio_size = (mmio_size << 32) | one_cell;
                iterator++;
         }

         struct device_entry * node;


                    node->mmio_base_address = mmio_base;
                     node->mmio_size = mmio_size;


                     uart_puts("\n MMIO base address \n");
                     uart_puthex(node->mmio_base_address);
                                          uart_puts("\n MMIO size  \n");
                     uart_puthex(node->mmio_size);
                     uart_puts("\n Check if it is correct  \n");

}

void extract_ssd_irq_line_data(uint8_t *base_address, size_t len_to_read,uint32_t address_cells, uint32_t size_cells, struct hardware_info *out_info)
{

}

