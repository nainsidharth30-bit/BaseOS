#include<stdint.h>
#include "../../include/lib/dbt.h"
#include "../../include/lib/quicksort.h"
#include "../../include/mkMAU/mkMAU.h"
#include<string.h>
#include "../../include/driverHeaders/uart.h"
#include "../../include/lib/alignbyte.h"
#include "../../include/driverHeaders/track_stack.h"
#include "../../include/driverHeaders/queue_address.h"
#include "../../include/driverHeaders/device_indexes.h"
#include "../../include/lib/compare_start_address.h"
#include "../../include/string_utility.h"
#include "../../include/lib/memcpy.h"


// Each one is a 4 byte big endian number ,  tells the parser what comes next                 

#define FDT_BEGIN_NODE  0x00000001  /* A new node starts here , After this token, the next bytes are the node name ,  a null-terminated string  padded to a 4-byte boundary. Then the node's properties and children follow   */
#define FDT_END_NODE    0x00000002  // End of  the current node 
#define FDT_PROP        0x00000003  /* Here comes a property ,  [FDT_PROP_TOKEN] [uint32 length] [uint32 name_offset] [value bytes...] [padding]
Length ------> How many bytes to read to get the data 
name_offset-------> offset in string block where name of the property lives 
value -------> actual data (length bytes )
padding-------->4 bytes aligned padding ! 
*/
#define FDT_NOP         0x00000004  //Do nothing ,  Skip 
#define FDT_END         0x00000009  // End of the structure block
/*
This function extract those reserved regions which are present inside the structure_block of dtb 
*/
void extract_reserved_regions_from_dt_struct(uint8_t* base_address ,size_t len_to_read , struct hardware_info* out_info,uint32_t address_cells ,uint32_t size_cells);
/*
This function extract irq line data for SSD device 
*/
void extract_ssd_irq_line_data(uint8_t *base_address, size_t len_to_read,uint32_t interrupt_cells,  struct hardware_info *out_info );
/*
This function extract the phandle , means a pointer which points toward the interrupt controller for the SSD device  
*/
void extract_ssd_interrupt_controller_phandle_data(uint8_t* base_address );
/*
To get the total size of the tree 
*/
 void traverse_totalsize (uintptr_t dbt_tree_ptr ,struct hardware_info *out_info);

 /* DTB_STRUCT_BLOCK is a tree structure where we get various information about the hardware like interrupt conbtrollers data , controllers data , RAM data and so on  */
void dtb_structure_blocks_parser (uintptr_t dtb_tree_ptr , struct hardware_info *out_info);

/* FIlls up zeroes in whole dtb
We are discarding dbt when the data we want is extracted , from then it has no use so we free that memory to use again ! 
*/
  void discard_dbt(uintptr_t dbt_tree_ptr , uintptr_t dbt_size);

  /* This function extracts Data From RSV_MAP section , The RSV_MAP section is a flat array , every entry is of 16 bytes , 8 bytes-->start address , 8bytes ----> to size , the end of RSV_MAP section has address and size as 0 which specifies the end of the RSV_MAP section !   */
  void   traverse_off_mem_rsvmap(uintptr_t dbt_tree_ptr,struct hardware_info *out_info);
  /* Extracts the ram base and ram size */
void extract_ram_info(uint8_t* base_address , size_t len_to_read , uint32_t address_cells , uint32_t size_cells , struct hardware_info* out_info);

/* extract ssd mmio base and mmio size */
void extract_ssd_mmio_size(uint8_t *base_address, size_t len_to_read,uint32_t address_cells, uint32_t size_cells, struct hardware_info *out_info,char* compatibility);






//                                                                  CODE STARTS FROM HERE ! 

     
   struct device_entry ssd_device_node ;  // For Storing the SSD Controller information , no matter which model of SSD Controller ! 



  __attribute__((section(".text.dtb_parser")))   // DTB_PARSERS section 

  /* Here , This function traverse the Device tree blob using the FDT_NODE tokens and extract hardware specific Information !*/
int extracting_dbt_info(uintptr_t dbt_tree_ptr , struct hardware_info *out_info )
{
       /*  Extracting Reserved Regions Data from the RSV_MAP section */
       uart_puts("\n Extracting Reserved Regions Data from the RSV_MAP section \n");
        traverse_off_mem_rsvmap(dbt_tree_ptr,out_info);


        /* Parsing the DTB_STRUCT_BLOCK  */
        uart_puts("\n Parsing the DTB_STRUCT_BLOCK  \n ");
       dtb_structure_blocks_parser(dbt_tree_ptr,out_info);


/*
After extracting all the resreved regions , I am sorting all the reserved regions for better working on patterned data 
*/

        array_sort(out_info->rsv_regions , sizeof(struct reserved_region) ,out_info->rsv_count ,compare_start_address);
       traverse_totalsize(dbt_tree_ptr,out_info);


       /* Devices array population Section ! */

         if(ssd_device_node.mmio_size)
         {
           memcopy(&out_info->devices[SSD_DEVICE_INDEX ] , &ssd_device_node , sizeof(struct device_entry));
           out_info->device_count++;
         }


          /* Devices array population Section Over   ! */


          uart_puts("\n ALl Data Extracted \n");
}

  void traverse_off_mem_rsvmap(uintptr_t dbt_tree_ptr, struct hardware_info *out_info)
  {      
    
    
    
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

   
      
      /*describe how to interpret the numbers inside---->how many 32-bit cells are used to represent an address in this node's reg property , it is present on the parent node — the node that contains children with reg properties. Also on the root, which is the default for anything that doesn't inherit*/
       uint32_t address_cells = 2;     
       
       /*
       What it says: how many 32-bit cells are used to represent a size in this node's reg property.
Where it appears: same place as #address-cells — on the parent, not the child.
       */
        uint32_t size_cells = 2; // we use default value 

        /*
        What it says: how many 32-bit cells are used to represent one interrupt in a device's interrupts property.
Where it appears: on the interrupt controller node — the node that will interpret the interrupts. Not on the device.
        */
        uint32_t  interrupt_cells = 0 ;
      

       int reserved_regions_depth = -1;
       int depth =0 ;




       //------------------SSD CONTROLLERS ARRAY ----------//
       /*An Array which stores names of various SSD Controller ! */
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


//----------------- Interrupt controllers compatible Array---//

static const char *interrupt_controller_compatibles[] = {
    "arm,gic-v3",
    "arm,gic-v3-its",
    "arm,gic-400",
    "arm,cortex-a15-gic",
    "arm,cortex-a7-gic",
    "arm,cortex-a9-gic",
    "arm,cortex-a5-gic",
    "arm,pl390",
    "arm,pl192-vic",
    "arm,pl190-vic",
    "arm,versatile-fpga-irq",
    "arm,nvic",
    "arm,armv8m-nvic",
    "riscv,plic0",
    "sifive,plic-1.0.0",
    "riscv,cpu-intc",
    "riscv,aclint-mswi",
    "riscv,aclint-mtimer",
    "riscv,clint0",
    "sifive,clint0",
    "mti,cpu-interrupt-controller",
    "mti,gic",
    "img,pdc",
    "cdns,xtensa-pic",
    "loongson,liointc",
    "loongson,htvec",
    "loongson,htpic",
    NULL
};

//----------------- Interrupt controllers compatible Array Over ---//
      

       //  -------- FLAG SECTION  ---------------------//
           
         int resreved_memory_dt_struct_flag =0 ;  
     
         int memory_node_flag = 0;
          
         int ssd_node_flag =0;
   
         uint8_t device_flag = 0;
     
         uint8_t compatible_flag =0;

        
         uint8_t interrupt_controller_flag = 0 ; 

         uint8_t phandle_flag = 0 ;





       //----------- FLAG SECTION OVER ---------------//

       //---------------- Buffers ----------------------//
         uint8_t *reg_buffer = NULL;
          size_t reg_len = 0;

             uint8_t *irq_line_buffer = NULL;
             size_t irq_line_len = 0;

             uint8_t * interrupt_parent_buffer = NULL;
             size_t interrupt_parent_len = 0;

             

           
             char compatible_buffer[12] = "";

             uint8_t* interrupt_controller_buffer = NULL;
             size_t interrupt_buffer_len = 0 ;
             
            



       //----------------------------------Buffers Over --------//    

       /*
        We have buffers to save found data value and flags which tells that this buffer belongs to that particular thing , 
        we are doing if(buffer & flag) to know that yes this buffer bnelongs to that and do call the correct Function ! 
       */

    

       while(one_byte_tracker_pointer < (uint8_t *)base_of_structure_block + __builtin_bswap32(fdt->size_dt_struct))
       {

        uint32_t token = __builtin_bswap32(*(uint32_t *)one_byte_tracker_pointer);
        one_byte_tracker_pointer=one_byte_tracker_pointer+4;

        if(token==FDT_BEGIN_NODE)
        {

          // We are setting the initial Values of all variables , buffers and flags so that we do not get data from previous iterations ! 
          
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
          phandle_flag=0;

                  interrupt_controller_buffer = NULL;
             interrupt_buffer_len = 0 ;


          depth++;
           
          const char* node_name = one_byte_tracker_pointer ;

               /* NOTE------------>
     The string value of FDT_BEGIN_NODE can tell us that this node will be storing that kind of information , but these string name changes from DTB to DTB , thats why we are not using those names  , but yes the name "reserved-memory" is same in all DTB FDT_BEGIN_NODE for the reserved regions present inside structure block of dtb ! 
     */

          if(str_eq(node_name, "reserved-memory"))
          {
                 resreved_memory_dt_struct_flag=1 ;
                 
                 reserved_regions_depth=depth;
                 
          }

          // Incremnmetring one_byte_tracker_pointer until it reaches the end of string means '\0' so that we can read next 4 bytes for FDT_PROP_NODE
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

    /*
    The properties name here are stored inside string block of dtb at some particular offset for that particular FDT_PROP_NODE and that offset here is nameoff ! 
    */

    /*
    Now our one_byte_tracker_pointer is already pointing towards the stored value or information , len tells us how many bytes to read to get that particular information =                               *   (one_byte_tracker_pointer+len)
    */

    const char* prop_name = (const char *)(base_of_string_block + nameoff);


    /*
    The cells value whether address cells , size cells and interruot cells is always 4 bytes hence we are not using len cause we know len is 4 bytes here ! 
    */

    if(str_eq(prop_name ,"#address-cells"))
    {
        address_cells = __builtin_bswap32(*(uint32_t*)one_byte_tracker_pointer);
        
    }

        if(str_eq(prop_name,"#size-cells"))
    {
               size_cells = __builtin_bswap32(*(uint32_t*)one_byte_tracker_pointer);
        
    }

    
    if (str_eq(prop_name,"#interrupt-cells"))
{
 
    interrupt_cells = __builtin_bswap32(*(uint32_t *)one_byte_tracker_pointer);
}

      /*
      Device type tells us type of device or what device type memory this node stores , ex: memory , cpu , cache ,pci , serial , network
      NOTE---> Device_type is legacy actually , its modern alter native is compatible but it is still in use for above written things ! 
      */


      /*
      What it tells you
Yes or no: is this node an interrupt controller?

If the property is present → the node is an interrupt controller (GIC, PLIC, APIC, whatever).
If not → the node is something else (a device, a bus, memory, etc.)
      */


      if (str_eq(prop_name,"interrupt-controller"))
{
    interrupt_controller_flag = 1;
}

      
  


      if(str_eq(prop_name,"phandle"))
      {
        phandle_flag=1;
           interrupt_controller_buffer=one_byte_tracker_pointer;
           interrupt_buffer_len = len ;
      }


        if(str_eq(prop_name,"device_type"))
    {


      device_flag=1;
             if(len==7)
             {
              if(str_eq(one_byte_tracker_pointer,"memory"))
              {
                  memory_node_flag = 1;
              }

             }
    }


    /*
    Why both device_type and compatible exist
device_type was the original way to classify nodes — a single string from a fixed vocabulary ("memory", "cpu", "cache"). The DTB spec defined it as the way to say "this is a memory node" or "this is a CPU node."

Later, the compatible system replaced it — a list of strings, vendor-prefixed, extensible, self-documenting. compatible = "arm,cortex-a53" tells you who made it and what it is.

device_type is now deprecated for most uses. Modern device trees use compatible for everything. But device_type = "memory" and device_type = "cpu" are still defined by the DTB spec and still appear in real DTBs
    */



        if(str_eq(prop_name,"compatible"))
    {
      compatible_flag=1;

      /*
      Now we are checking if it is compatible for some ssd controller ! 
      */
      
              for(int iterator = 0 ; ssd_compatibles[iterator]!=NULL;iterator++)
              {
                if(str_eq((const char*)one_byte_tracker_pointer,ssd_compatibles[iterator]))
                {  
                   ssd_node_flag=1;
                   int len = 0 ;
                  
                   while(ssd_compatibles[iterator][len]!='\0')
                   {
                    len++;
                   }
                   len++;
                   memcopy(compatible_buffer,ssd_compatibles[iterator],len);
                   break;
                }
              }

              /* Now We are checking if it is compatible for some interrupt controller ! */
   
               for(int iterator=0 ; interrupt_controller_compatibles[iterator]!=NULL ; iterator++)
               {
                if(str_eq((const char*)one_byte_tracker_pointer,interrupt_controller_compatibles[iterator]))
                {
                  interrupt_controller_flag=1;
                  int len =0 ;
                  while(interrupt_controller_compatibles[iterator][len]!='\0')
                  {
                    len++;
                  }
                  len++;
                  memcopy(compatible_buffer,interrupt_controller_compatibles[iterator],len);
                  break;
                }
               }






    }

    /*
    The reg property gives us pair of values (address,size ) which tells us about base address of some device and its size , for example the mmio_base_address of some devices and their size also example any ssd controller , gic controller ,reserved regions  and so on , so it gives pairs of (address,size )
    */


      if(str_eq(prop_name,"reg"))
      {
             reg_buffer = one_byte_tracker_pointer ;
             reg_len = len ; 
      }

      /*
      interrupts tells you which interrupt line a device is wired to at its interrupt controller, and how it's triggered.
      Means tells about the irq line of some device like an ssd controller or a gic controller and more !
      */

      if(str_eq(prop_name,"interrupts"))
      {
       
            irq_line_buffer=one_byte_tracker_pointer;
            irq_line_len=len;
      }

      /*
      interrupt-parent tells you which interrupt controller interprets this device's interrupts property.
It's a single phandle — a reference to the controller node. Without it, the cells in interrupts have no defined meaning
      */

      if(str_eq(prop_name,"interrupt-parent"))
      {
       
         interrupt_parent_buffer=one_byte_tracker_pointer;
         interrupt_parent_len=len;
      }

      


      


 /* from one_byte_tracker_pointer to one_byte_tracker_pointer+len , lies information or data , the value of one_byte_tracvker_pointer is set in buffers and len value is also set in buffer_len 
 so now we can move onto next nodes by doing one_byte_tracker_pointer = one_byte_tracker_pointer+len;
 */


    one_byte_tracker_pointer = one_byte_tracker_pointer+len;
     byte_alignment((void **)&one_byte_tracker_pointer);


        }
        else if(token==FDT_NOP)
        {
          // Do nothing , I have no info
        }
        else if(token == FDT_END_NODE)
        {

          // Here we Will use the flags , the buffers , the buffer len we had set ! 


          if(memory_node_flag && reg_buffer &&reg_len)
      {
        extract_ram_info(reg_buffer,reg_len,address_cells,size_cells,out_info);
      }

      if(ssd_node_flag && reg_buffer && reg_len)
      { 
        
        extract_ssd_mmio_size(reg_buffer,reg_len,address_cells,size_cells,out_info,compatible_buffer);
      }

      if(interrupt_controller_flag&&reg_buffer&&reg_len)
      {
        // Call a function which will extract mmio_base and mmio size of that interrupt controller 
      }

       

      if(ssd_node_flag && irq_line_buffer && irq_line_len)
      {
       
        extract_ssd_irq_line_data(irq_line_buffer,irq_line_len,interrupt_cells,out_info);
      }

      if(ssd_node_flag && interrupt_parent_buffer )
      {
        
        extract_ssd_interrupt_controller_phandle_data(interrupt_parent_buffer);
      
      }

            if (resreved_memory_dt_struct_flag && reg_buffer && reg_len )
      {
        // Means we got some reserved regions ! 
   
        extract_reserved_regions_from_dt_struct(reg_buffer, reg_len,out_info,address_cells,size_cells);
      }

          // I am the end of the node , Reset every variable for new fresh iterations ! 
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


                     
          uart_puts("\n Hello , I am in main , giving you Ram Basew and Ram size\n");
    uart_puthex(out_info->ram_base_address);
    uart_puts("\n");
    uart_puthex(out_info->ram_size);
    uart_puts("\n");
}

void extract_ssd_mmio_size(uint8_t *base_address, size_t len_to_read,uint32_t address_cells, uint32_t size_cells, struct hardware_info *out_info ,char* compatibility)
{    

  uart_puts("\n Hello , I am in SSD DTB \n");
  


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

         

                        ssd_device_node.mmio_base_address=mmio_base;
                      ssd_device_node.mmio_size=mmio_size;
                 

                     int i=0;
                     uart_puts("\n  SSD COMPATIBLE  \n ");
                     while(compatibility[i]!='\0')
                     {
                      uart_putc(compatibility[i]);
                    ssd_device_node.compatible[i]=compatibility[i];
                      i++;
                     }
                     uart_puts("\n");
                    ssd_device_node.compatible[i]='\0';

                     uart_puts("\n  SSD MMIO BASE\n ");
                     uart_puthex(mmio_base);
                     uart_puts("\nSSD MMIO SIZE\n ");
                     uart_puthex(mmio_size);

                        if(str_eq(compatibility  , "virtio,mmio"))
                        {
                               extract_virtio_queue_address(mmio_base,mmio_size);  
                     
                        }
                                      
}

void extract_ssd_irq_line_data(uint8_t *base_address, size_t len_to_read,uint32_t interrupt_cells,  struct hardware_info *out_info )
{
       
       ssd_device_node.irq_cells_count=interrupt_cells;

       uint32_t * tracker_pointer = (uint32_t*)base_address ;
        int i=0 ;
       while((uint8_t*)tracker_pointer<base_address+len_to_read)
       {
        ssd_device_node.irq_cells[i]=__builtin_bswap32(*tracker_pointer);
        tracker_pointer=tracker_pointer+1;
        i++;
       }

       

}

void extract_ssd_interrupt_controller_phandle_data(uint8_t* base_address  )
{
  uint32_t * reader = (uint32_t*)base_address;
  ssd_device_node.interrupt_controller_phandle=__builtin_bswap32(*reader );
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


