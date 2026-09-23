#include<stdint.h>
#include "../../include/lib/dbt.h"
#include "../../include/lib/quicksort.h"
#include "../../include/mkMAU/mkMAU.h"
#include<string.h>
#include "../../include/driverHeaders/uart.h"
#include "../../include/lib/alignbyte.h"
#include "../../include/driverHeaders/track_stack.h"



#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009

void extract_reserved_regions_from_dt_struct(uint8_t* base_address ,size_t len_to_read , struct hardware_info* out_info,uint32_t address_cells ,uint32_t size_cells);
void extract_ssd_irq_line_data(uint8_t *base_address, size_t len_to_read,uint32_t interrupt_cells,  struct hardware_info *out_info , struct device_entry* device_node);
void extract_ssd_gic_phandle_data(uint8_t* base_address  , struct device_entry* device_node);
 void traverse_totalsize (uintptr_t dbt_tree_ptr ,struct hardware_info *out_info);
void dtb_structure_blocks_parser (uintptr_t dtb_tree_ptr , struct hardware_info *out_info);
  void discard_dbt(uintptr_t dbt_tree_ptr , uintptr_t dbt_size);
  void   traverse_off_mem_rsvmap(uintptr_t dbt_tree_ptr,struct hardware_info *out_info);
void extract_ram_info(uint8_t* base_address , size_t len_to_read , uint32_t address_cells , uint32_t size_cells , struct hardware_info* out_info);
void extract_ssd_mmio_size(struct device_entry* device_node,uint8_t *base_address, size_t len_to_read,uint32_t address_cells, uint32_t size_cells, struct hardware_info *out_info,char* compatibility);

int compare_start_address(const void *a, const void *b)
{
    const struct mkmau_node *node_a = (const struct mkmau_node *)a;
    const struct mkmau_node *node_b = (const struct mkmau_node *)b;

    if (node_a->base_range < node_b->base_range) return -1;
    if (node_a->base_range > node_b->base_range) return 1;
    return 0;
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


          if(reserved_region_address==0 && reserved_region_size==0)
          {
      
            break;
          }

          if(total_numbers_of_rserved_regions<MAX_RESERVED_REGIONS)
          {
            out_info->rsv_regions[total_numbers_of_rserved_regions].start=reserved_region_address;
            out_info->rsv_regions[total_numbers_of_rserved_regions].end=reserved_region_end_address;
            total_numbers_of_rserved_regions++;
          }
          out_info->rsv_count=total_numbers_of_rserved_regions;

     
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

      struct device_entry device_node ;
      struct device_entry * device_node_ptr=&device_node;

              uint32_t address_cells = 2;
        uint32_t size_cells = 2; // we use default value 

        uint32_t  interrupt_cells = 0 ;
      

       int reserved_regions_depth = -1;
       int depth =0 ;




       //------------------SSD CONTROLLERS ARRAY ----------//
       static const char *ssd_compatibles[] = {
    /* --- Virtual / Generic --- */
    "virtio,mmio",              /* QEMU, KVM, Firecracker */
    "generic-ahci",             /* Fallback for any standard AHCI */

    /* --- NVMe (PCIe SSDs) --- */
    "nvme",                     /* Standard PCIe NVMe */
    "apple,nvme-ans2",          /* Apple Silicon */
    "qcom,ufshc",               /* Qualcomm UFS (often used as boot storage) */

    /* --- UFS (Universal Flash Storage) --- */
    "jedec,ufs-1.1",            /* UFS 1.1 standard */
    "jedec,ufs-2.0",            /* UFS 2.0+ standard (mobile/embedded) */
    "qcom,ufshc",               /* Qualcomm UFS host */

    /* --- SATA / AHCI (Embedded & Server) --- */
    "snps,dwc-ahci",            /* Synopsys DesignWare (most common IP) */
    "snps,spear-ahci",          /* ST Spear */
    "marvell,armada-3700-ahci", /* Marvell Armada */
    "marvell,armada-8k-ahci",   /* Marvell Armada 8K */
    "brcm,sata3-ahci",          /* Broadcom SATA3 */
    "hisilicon,hisi-ahci",      /* HiSilicon */
    "cavium,octeon-7130-ahci",  /* Cavium Octeon */

    /* --- SD/MMC / eMMC (Most common on small RISC chips) --- */
    "sdhci",                    /* Standard SD Host Controller Interface */
    "snps,dw-mshc",             /* Synopsys DesignWare Mobile Storage */
    "arm,pl180",                /* ARM PrimeCell (older) */
    "mediatek,mt8173-mmc",      /* MediaTek */

    NULL
};

//---------------- SSD CONTROLLERS ARRAY OVER --------//
      

       //  -------- FLAG SECTION  ---------------------//
 
         int resreved_memory_dt_struct_flag =0 ;  

         int memory_node_flag = 0;
         int ssd_node_flag =0;

         uint8_t device_flag = 0;
         uint8_t compatible_flag =0;





       //----------- FLAG SECTION OVER ---------------//

       //---------------- Buffers ----------------------//
         uint8_t *reg_buffer = NULL;
          size_t reg_len = 0;

             uint8_t *irq_line_buffer = NULL;
             size_t irq_line_len = 0;

             uint8_t * interrupt_parent_buffer = NULL;
             size_t interrupt_parent_len = 0;

           
             char compatible_buffer[12] = "";
             



       //----------------------------------Buffers Over --------//    

    

       while(one_byte_tracker_pointer < (uint8_t *)base_of_structure_block + __builtin_bswap32(fdt->size_dt_struct))
       {

        uint32_t token = __builtin_bswap32(*(uint32_t *)one_byte_tracker_pointer);
        one_byte_tracker_pointer=one_byte_tracker_pointer+4;

        if(token==FDT_BEGIN_NODE)
        {
          
          address_cells=2;
          compatible_flag=0;
          device_flag=0;
          size_cells=2;
          memory_node_flag = 0 ;
          ssd_node_flag=0;
          reg_buffer=NULL;
          reg_len=0;
          irq_line_buffer=NULL;
          irq_line_len=0;
          compatible_buffer[0] = '\0'; 


          depth++;
           
          const char* node_name = one_byte_tracker_pointer ;

          if(node_name[0]=='r'&&node_name[1]=='e'&&node_name[2]=='s'&&node_name[3]=='e'&&node_name[4]=='r'&&node_name[5]=='v'&&node_name[6]=='e'&&node_name[7]=='d'&&node_name[8]=='-'&&node_name[9]=='m'&&node_name[10]=='e'&&node_name[11]=='m'&&node_name[12]=='o'&&node_name[13]=='r'&&node_name[14]=='y'&&node_name[15]=='\0')
          {
                 resreved_memory_dt_struct_flag=1 ;
                 
                 reserved_regions_depth=depth;
                 
          }
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

    
    if (prop_name[0]=='#' && prop_name[1]=='i' && prop_name[2]=='n' && prop_name[3]=='t' && prop_name[4]=='e' && prop_name[5]=='r' && prop_name[6]=='r' && prop_name[7]=='u' && prop_name[8]=='p' && prop_name[9]=='t' && prop_name[10]=='-' && prop_name[11]=='c' && prop_name[12]=='e' && prop_name[13]=='l' && prop_name[14]=='l' &&prop_name[15]=='s' && prop_name[16]=='\0')
{
 
    interrupt_cells = __builtin_bswap32(*(uint32_t *)one_byte_tracker_pointer);
}



        if(prop_name[0]=='d'&&prop_name[1]=='e'&&prop_name[2]=='v'&&prop_name[3]=='i'&&prop_name[4]=='c'&&prop_name[5]=='e'&&prop_name[6]=='_'&&prop_name[7]=='t'&&prop_name[8]=='y'&&prop_name[9]=='p'&&prop_name[10]=='e'&&prop_name[11]=='\0')
    {
      device_flag=1;
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
      compatible_flag=1;
                     for(int ssd_compatible_iterator = 0 ; ssd_compatibles[ssd_compatible_iterator]!=NULL;ssd_compatible_iterator++)
            {
                const char* ssd_controller_name = ssd_compatibles[ssd_compatible_iterator];

                uint8_t match_found_flag = 0 ;
                uint8_t per_char_iterator = 0 ;

                while(ssd_controller_name[per_char_iterator]==one_byte_tracker_pointer[per_char_iterator])
                {
                   per_char_iterator++ ;
                   if(ssd_controller_name[per_char_iterator]=='\0' && one_byte_tracker_pointer[per_char_iterator]=='\0')
                   {
                    match_found_flag=1;
                   }
                }

                if(match_found_flag)
                {
                   ssd_node_flag=1;
                   
                   uint8_t i = 0 ;
                   while(ssd_controller_name[i]!='\0')
                   {
                    compatible_buffer[i]=ssd_controller_name[i];
                    i++;
                   }
                    compatible_buffer[i]='\0';
                   break;
                }
                

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
        track_stack();
        extract_ssd_mmio_size(device_node_ptr,reg_buffer,reg_len,address_cells,size_cells,out_info,compatible_buffer);
      }

      if(ssd_node_flag&&irq_line_buffer)
      {
       
        extract_ssd_irq_line_data(irq_line_buffer,irq_line_len,interrupt_cells,out_info,device_node_ptr);
      }

      if(ssd_node_flag&&interrupt_parent_buffer)
      {
        
        extract_ssd_gic_phandle_data(interrupt_parent_buffer,device_node_ptr);
      
      }

            if (resreved_memory_dt_struct_flag&&reg_buffer )
      {
        // Means we got some reserved regions ! 
   
        extract_reserved_regions_from_dt_struct(reg_buffer, reg_len,out_info,address_cells,size_cells);
      }

          // I am the end of the node , Reset every variable 
          memory_node_flag = 0 ;
          ssd_node_flag=0;
          compatible_flag=0;
          device_flag=0;
          reg_buffer=NULL;
          reg_len=0;
          irq_line_buffer=NULL;
          irq_line_len=0;
          
          compatible_buffer[0] = '\0'; 

          if (reserved_regions_depth != -1 && depth == reserved_regions_depth) {
    resreved_memory_dt_struct_flag = 0;
    reserved_regions_depth = -1;
                              }
depth--;
         
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

void extract_ssd_mmio_size(struct device_entry* device_node,uint8_t *base_address, size_t len_to_read,uint32_t address_cells, uint32_t size_cells, struct hardware_info *out_info ,char* compatibility)
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

         


                    device_node->mmio_base_address = mmio_base;
                     device_node->mmio_size = mmio_size;

                     int i=0;
                     while(compatibility[i]!='\0')
                     {
                      device_node->compatible[i]=compatibility[i];
                      i++;
                     }
                     device_node->compatible[i]='\0';
  

            
                     
                  

}

void extract_ssd_irq_line_data(uint8_t *base_address, size_t len_to_read,uint32_t interrupt_cells,  struct hardware_info *out_info , struct device_entry* device_node)
{
       
       device_node->irq_cells_count=interrupt_cells;

       uint32_t * tracker_pointer = (uint32_t*)base_address ;
        int i=0 ;
       while((uint8_t*)tracker_pointer<base_address+len_to_read)
       {
        device_node->irq_cells[i]=__builtin_bswap32(*tracker_pointer);
        tracker_pointer=tracker_pointer+1;
        i++;
       }

       

}

void extract_ssd_gic_phandle_data(uint8_t* base_address  , struct device_entry* device_node)
{
  uint32_t * reader = (uint32_t*)base_address;
  device_node->interrupt_controller_phandle=__builtin_bswap32(*reader );
}



void extract_reserved_regions_from_dt_struct(uint8_t* base_address ,size_t len_to_read , struct hardware_info* out_info,uint32_t address_cells ,uint32_t size_cells )
{


  uint32_t * reader = (uint32_t*)base_address;
     uint32_t cells_in_one_region = address_cells+size_cells ;
     uint32_t total_cells = len_to_read/4 ;// one cell is of 4 bytes 

     uint32_t total_number_of_rsv_regions = total_cells/cells_in_one_region ;

     int i = 0 ;

     while(i<total_number_of_rsv_regions)
     {

      uint64_t start_address =0 ;
      uint64_t end_address=0 ;
      uint64_t size=0 ;

      for(int j=0 ; j<address_cells ; j++)
      {
            uint32_t cell_read = __builtin_bswap32(*reader);
            start_address = (start_address<<32)|cell_read ;
            reader=reader+1 ;
      }

      for(int k=0 ;k<size_cells ;k++)
      {
          uint32_t cell_read = __builtin_bswap32(*reader);
          size = (size<<32)|cell_read ;
          reader=reader+1;
      }
      end_address = start_address+size ;

      
      out_info->rsv_regions[out_info->rsv_count].start=start_address;
       out_info->rsv_regions[out_info->rsv_count].end=end_address;
       out_info->rsv_count=out_info->rsv_count+1 ;

       i++;

     }

}