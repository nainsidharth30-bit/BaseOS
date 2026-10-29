#include<stdint.h>
#include "../../include/lib/dbt.h"
#include "../../include/lib/quicksort.h"
#include "../../include/lib/setzero.h"
#include "../../include/lib/memcpy.h"
#include "../../include/mkMAU/mkMAU.h"
#include<string.h>
#include "../../include/driverHeaders/uart.h"
#include "../../include/lib/alignbyte.h"
#include "../../include/driverHeaders/track_stack.h"
#include "../../include/driverHeaders/queue_address.h"
#include "../../include/lib/compare_start_address.h"
#include "../../include/string_utility.h"


#include"../interrupts/arm_cortex_a15_gic.h"
#include"../interrupts/arm_pl190_vic.h"
#include"../../include/driverHeaders/VIRTIO.h"
#include"../../include/driverHeaders/ahci.h"


/*
ARCHITECTURAL DECISIONS --- >  We are storing the peripheral devices hardware information and Interrupt Controller (IC) hardware information inside an array called devices which is present inside the struct hardware_info , so when we need some device data than we have to traverse the array and match the correct compatible string of node.compatible with compatible devices array we made , its expensive , so we made a rule that SSD Controller Data will always be placed at index 0 of devices array , and data of the IC to which the SSD controller is connected at index 1 .

That means if data of some peripheral devices is placed at index "x" than data of connected IC will always be present at index "x+1"

Benefits --> O(1) time complexirty 
Disadvantage --> Multiple peripheral devices are connected to one single IC hence data of IC will be copied many times in different indexes ! 
*/

/*
ARCHITECTURAL DECISION ---->  We need to match the phandle of the parent-controller node connected to some peripheral device node with the IC node ! 
For example we have the phandle data of the SSD Controller , now we need to match it with the IC whose phandle value is same to SSD Controller , which tells that SSD controller is connected to this IC ! 
But the traversal of the tree does not guarantee about whose nodes come first , IC node or SSD Controller node 
Hence , on traversal we store the IC node data in some array , and the position of that IC's data  is its phandle value , means if IC2 has a phandle value of 4 than it will be placed at index 4
Benefits----> O(1) time complexity
Disadvantages ---> More memory usage ! 
*/


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
this function extracts the ssd controllers uniq id and mapped sectors where the extension metadat is present ! 
*/


void extract_extensions_metadata_controllers_uniq_ids(struct hardware_info* out_info,
                                                      uint8_t* ssd_controller_uniq_id_buffer,
                                                      uint32_t len_uniq_id_buffer) ; 

                                                       void extract_extensions_metadata_mapped_sectors(struct hardware_info* out_info,
                                                uint8_t* ssd_controller_mapped_sectors_buffer,
                                                uint32_t len_mapped_sectors_buffer) ; 


/*
This function extract those reserved regions which are present inside the structure_block of dtb 
*/
void extract_reserved_regions_from_dt_struct(uint8_t* base_address ,size_t len_to_read , struct hardware_info* out_info,uint32_t address_cells ,uint32_t size_cells);

/*
This function extracts the mem_sec value which tells if the chip has mpu , mmu or none 
*/

void extract_mem_sec_value(uint8_t* mem_sec_buffer , struct hardware_info* out_info) ; 

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

/*
This function extracts IC Data 
*/

void extract_ic_devices_data(uint8_t* reg_buffer , uint16_t reg_len , uint8_t* compatible_buffer , uint8_t* irq_line_buffer , uint8_t irq_line_len , uint8_t* interrupt_controller_buffer ,uint16_t interrupt_parent_len , uint32_t address_cells , uint32_t size_cells , uint32_t interrupt_cells ,const struct ic_compatible_entry ic_compatibles[], struct hardware_info* out_info ,uint8_t* phandle_buffer );
/*
This Function extracts the  SSD devices Data  
*/
void extract_ssd_devices_data(uint8_t* reg_buffer , uint16_t reg_len , uint8_t* compatible_buffer , uint8_t* irq_line_buffer , uint8_t irq_line_len , uint8_t* interrupt_parent_buffer ,uint16_t interrupt_parent_len , uint32_t address_cells , uint32_t size_cells , uint32_t interrupt_cells, const struct ssd_compatible_entry ssd_compatibles[] , struct hardware_info* out_info , uint8_t* phandle_buffer , uint8_t* priority_buffer ,  uint8_t* queue_num_buffer , uint8_t* uniq_id_buffer  );



  /* Index Array to traverse out_info->devices array efficiently ! */


uint16_t ssd_index_array [10];
uint8_t ssd_index_array_count = 0 ;    


uint16_t ic_index_array [10];
uint8_t ic_index_array_count = 0 ;

 /* Index Array to traverse out_info->devices array efficiently Over  ! */





//                                                                  CODE STARTS FROM HERE ! 

     
   struct device_entry ssd_device_node = {0} ;  // For Storing the SSD Controller information , no matter which model of SSD Controller ! 

   struct device_entry ic_node = {0};
   





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





          uart_puts("\n ALl Data Extracted \n");


          /* ============================================================
 * Print all SSD devices, using ssd_index_array to find them
 * ============================================================ */
uart_puts("\n\n===== SSD DEVICES =====\n");
uart_puts("ssd_index_array_count = ");
uart_puthex(ssd_index_array_count);
uart_puts("\n");

for (uint32_t i = 0; i < ssd_index_array_count; i++) {
    uint32_t idx = ssd_index_array[i];
    struct device_entry *d = &out_info->devices[idx];

    uart_puts("\nSSD #");
    uart_puthex(i);
    uart_puts(" (devices[");
    uart_puthex(idx);
    uart_puts("])\n");

    uart_puts("  phandle    = ");
    uart_puthex(d->phandle);
    uart_puts("\n");

    uart_puts("  compatible = ");
    uart_puts(d->compatible);
    uart_puts("\n");

    uart_puts("  mmio_array_count = ");
    uart_puthex(d->mmio_base_size_array_count);
    uart_puts("\n");

    for (uint32_t p = 0; p + 1 < d->mmio_base_size_array_count; p += 2) {
        uart_puts("    base = ");
        uart_puthex(d->mmio_base_size_array[p]);
        uart_puts("  size = ");
        uart_puthex(d->mmio_base_size_array[p + 1]);
        uart_puts("\n");
    }

    uart_puts("  irq_cells_count = ");
    uart_puthex(d->irq_cells_count);
    uart_puts("\n");

    for (uint32_t c = 0; c < d->irq_cells_count; c++) {
        uart_puts("    irq_cell[");
        uart_puthex(c);
        uart_puts("] = ");
        uart_puthex(d->irq_cells[c]);
        uart_puts("\n");
    }

    uart_puts("  peripheral_code fn = ");
    uart_puthex((uintptr_t)d->peripheral_code);
    uart_puts("\n");
}


/* ============================================================
 * Print all IC devices, using ic_index_array to find them
 * ============================================================ */
uart_puts("\n\n===== IC DEVICES =====\n");
uart_puts("ic_index_array_count = ");
uart_puthex(ic_index_array_count);
uart_puts("\n");

for (uint32_t i = 0; i < ic_index_array_count; i++) {
    uint32_t idx = ic_index_array[i];
    struct device_entry *d = &out_info->devices[idx];

    uart_puts("\nIC #");
    uart_puthex(i);
    uart_puts(" (devices[");
    uart_puthex(idx);
    uart_puts("])\n");

    uart_puts("  phandle    = ");
    uart_puthex(d->phandle);
    uart_puts("\n");

    uart_puts("  compatible = ");
    uart_puts(d->compatible);
    uart_puts("\n");

    uart_puts("  mmio_array_count = ");
    uart_puthex(d->mmio_base_size_array_count);
    uart_puts("\n");

    for (uint32_t p = 0; p + 1 < d->mmio_base_size_array_count; p += 2) {
        uart_puts("    base = ");
        uart_puthex(d->mmio_base_size_array[p]);
        uart_puts("  size = ");
        uart_puthex(d->mmio_base_size_array[p + 1]);
        uart_puts("\n");
    }

    uart_puts("  ic_code fn = ");
    uart_puthex((uintptr_t)d->ic_code);
    uart_puts("\n");
}
uart_puts("\n\n");

          

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
        uint32_t size_cells = 2; 

        /*
        What it says: how many 32-bit cells are used to represent one interrupt in a device's interrupts property.
Where it appears: on the interrupt controller node — the node that will interpret the interrupts. Not on the device.
        */
        uint32_t  interrupt_cells = 0 ;

      

       int reserved_regions_depth = -1;
       int depth =0 ;

       // Extern Variables from Main  !   //

       

       // Extern Variables from Main  End !//

       




       //------------------SSD CONTROLLERS ARRAY ----------//
       /*An Array which stores names of various SSD Controller ! */
   static const struct ic_compatible_entry ic_compatibles[] = {
 { "arm,cortex-a15-gic", arm_cortex_a15_gic },
    { "arm,pl190-vic",      arm_pl190_vic      },
    { 0, 0 }
};
//---------------- SSD CONTROLLERS ARRAY OVER --------//

static const struct ssd_compatible_entry ssd_compatibles[] = {
     { "virtio,mmio", virtio_ssd_driver },
    { "generic-ahci", ahci_driver },
    { 0, 0 }
    
};



      

       //  -------- FLAG SECTION  ---------------------//
           
         int resreved_memory_dt_struct_flag =0 ;  
     
         int memory_node_flag = 0;
          
         int ssd_node_flag =0;
   
         uint8_t device_flag = 0;
     
         uint8_t compatible_flag =0;

         uint8_t uniq_id_flag = 0 ; 

         uint8_t interrupt_cells_flag=0;
        
         uint8_t interrupt_controller_flag = 0 ; 

         uint8_t phandle_flag = 0 ;

         uint8_t not_disabled_flag = 1 ;

        uint8_t priority_flag = 0 ; 
        uint8_t block_devices_queue_flag = 0 ; 

   



       //----------- FLAG SECTION OVER ---------------//

       //---------------- Buffers ----------------------//
         uint8_t *reg_buffer = NULL;
          size_t reg_len = 0;

             uint8_t *irq_line_buffer = NULL;
             size_t irq_line_len = 0;

             uint8_t * interrupt_parent_buffer = NULL;
             size_t interrupt_parent_len = 0;

             

           
             char compatible_buffer[40] = ""; 

             uint8_t * uniq_id_buffer = NULL ; 

             /* Stores the value of the phandle ! */

             uint8_t* interrupt_controller_buffer = NULL;
             size_t interrupt_buffer_len = 0 ;

             /* stores phandle of peripheral devices */
             uint8_t* phandle_buffer = NULL ; 
             
             uint8_t* priority_buffer = NULL ; 
             uint8_t * block_device_queue_buffer = NULL ; 

            
           



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
          uniq_id_buffer=NULL;
          uniq_id_flag=0 ; 
          phandle_flag=0;
          not_disabled_flag=1;
          interrupt_cells=0;
          interrupt_cells_flag=0;

                  interrupt_controller_buffer = NULL;
             interrupt_buffer_len = 0 ;
             interrupt_controller_flag=0;

             phandle_buffer=NULL;

             priority_flag=0;
             priority_buffer=NULL;

             block_device_queue_buffer=NULL;
             block_devices_queue_flag = 0 ; 

         

      

           

          
             set_zero(&ssd_device_node,sizeof(struct device_entry));
             set_zero(&ic_node,sizeof(struct device_entry));
          


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
    interrupt_cells_flag=1;
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


 if(str_eq(prop_name,"baseos,irq-priority"))
 {
  priority_flag = 1 ; 
  priority_buffer = one_byte_tracker_pointer; 
 }

 if(str_eq(prop_name , "baseos,num-queue"))
 {
  block_devices_queue_flag = 1 ; 
  block_device_queue_buffer = one_byte_tracker_pointer ; 
 }


 if(str_eq(prop_name , "baseos,mem-sec"))
 {
   extract_mem_sec_value(one_byte_tracker_pointer , out_info);
 }


 if(str_eq(prop_name , "baseos,ssd-controller-extensions-uniq-id"))
 {  uart_puts("\nI am runing , unique id  \n");


   
    extract_extensions_metadata_controllers_uniq_ids(
        out_info,
        one_byte_tracker_pointer,
        len);

    
 }

 if(str_eq(prop_name , "baseos,sector-number-extensions"))
 {
  uart_puts("\nI am runing  , mapped sectorsd \n");

   
    extract_extensions_metadata_mapped_sectors(
        out_info,
        one_byte_tracker_pointer,
        len );
 }

 if(str_eq(prop_name,"baseos,uniq-id"))
 {
  uniq_id_flag=1 ; 
  uniq_id_buffer=one_byte_tracker_pointer ; 
 }




      
  


      if(str_eq(prop_name,"phandle"))
      {
        phandle_flag=1;
           phandle_buffer=one_byte_tracker_pointer;
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



if (str_eq(prop_name, "compatible"))
{
    compatible_flag = 1;

    int n = 0;                                  /* copy first string */
    while (n < (int)sizeof(compatible_buffer) - 1 && one_byte_tracker_pointer[n] != '\0') {
        compatible_buffer[n] = one_byte_tracker_pointer[n];
        n++;
    }
    compatible_buffer[n] = '\0';

    for (int k = 0; ssd_compatibles[k].compatible != 0; k++)   /* is it an SSD? */
        if (str_eq(ssd_compatibles[k].compatible, compatible_buffer)) { ssd_node_flag = 1; break; }
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


      if(str_eq(prop_name,"status"))
      {
        if(str_eq(one_byte_tracker_pointer,"disabled"))
        {
          not_disabled_flag=0;
        }
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

      


      /*
      Extracting SSD Controller Data ! 
      */


      if(ssd_node_flag && compatible_flag && reg_len && irq_line_len && not_disabled_flag&&block_devices_queue_flag&&uniq_id_flag)
      {
       extract_ssd_devices_data(reg_buffer,reg_len,compatible_buffer,irq_line_buffer,irq_line_len,interrupt_parent_buffer,interrupt_parent_len,address_cells,size_cells,interrupt_cells,ssd_compatibles,out_info,phandle_buffer , priority_buffer , block_device_queue_buffer , uniq_id_buffer);
      }



     /*
      Extracting SSD Controller Data Over ! 
      */

   

      /*
      Extracting IC Controller Data ! 
      */

      if(interrupt_controller_flag&&reg_len&&compatible_flag&&not_disabled_flag&&phandle_flag&&interrupt_cells_flag)
      {
        extract_ic_devices_data(reg_buffer,reg_len,compatible_buffer,irq_line_buffer,irq_line_len,interrupt_parent_buffer,interrupt_parent_len,address_cells,size_cells,interrupt_cells,ic_compatibles,out_info,phandle_buffer);
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
          interrupt_buffer_len=0;
          interrupt_parent_buffer=NULL;
          interrupt_cells=0;
          interrupt_cells_flag=0;
          phandle_buffer=NULL;
          
          compatible_buffer[0] = '\0'; 
          not_disabled_flag=1;

                   block_device_queue_buffer=NULL;
             block_devices_queue_flag = 0 ;


             set_zero(&ssd_device_node,sizeof(struct device_entry));
             set_zero(&ic_node,sizeof(struct device_entry));



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

void extract_ssd_devices_data(uint8_t* reg_buffer , uint16_t reg_len , uint8_t* compatible_buffer , uint8_t* irq_line_buffer , uint8_t irq_line_len , uint8_t* interrupt_parent_buffer ,uint16_t interrupt_parent_len , uint32_t address_cells , uint32_t size_cells , uint32_t interrupt_cells ,const  struct ssd_compatible_entry ssd_compatibles[] , struct hardware_info* out_info,uint8_t* phandle_buffer , uint8_t* priority_buffer , uint8_t* queue_num_buffer, uint8_t* uniq_id_buffer   )
{
    /*
    Extracting mmio base and mmio size first ! 
    */

     uint32_t* tracker = (uint32_t*)reg_buffer ; 
     uint32_t read_bytes = 0 ;

     while(read_bytes<reg_len)
     {
        uintptr_t mmio_base = 0 ; 
        uintptr_t mmio_size = 0 ; 

        int a_c = 0 ;
        while(a_c!=address_cells)
        {
          uint32_t cell = __builtin_bswap32(*tracker);
          mmio_base = (mmio_base<<32)|cell ; 
          a_c++;
          tracker++;
          read_bytes = read_bytes+4;
        }
        int s_c = 0 ; 
        while(s_c!=size_cells)
        {
          uint32_t cell = __builtin_bswap32(*tracker);
          mmio_size = (mmio_size<<32)|cell ;
          s_c++;
          tracker++;
          read_bytes = read_bytes+4;
        }

        ssd_device_node.mmio_base_size_array[ssd_device_node.mmio_base_size_array_count] = mmio_base ; 
        ssd_device_node.mmio_base_size_array_count++ ; 
        ssd_device_node.mmio_base_size_array[ssd_device_node.mmio_base_size_array_count] = mmio_size ; 
         ssd_device_node.mmio_base_size_array_count++ ; 
     }

     /*
     Extracting Compatibles and assigning Function Poinrter Driver ! 
     */
   
     int i = 0 ; 
     while(ssd_compatibles[i].compatible!=0&&ssd_compatibles[i].peripheral_code!=0)
     {
      if(str_eq(ssd_compatibles[i].compatible,compatible_buffer))
      {
        int len = 0 ; 
        while(compatible_buffer[len]!='\0')
        {
          len++;
        }
        len++;
        memcopy(ssd_device_node.compatible,compatible_buffer,len);
        ssd_device_node.peripheral_code=ssd_compatibles[i].peripheral_code ; 
        break;
      }
      i++;
     }

     /*
     Extracting the phandle now  ! 
     */

     tracker = (uint32_t*)phandle_buffer; 

     uint32_t phandle = __builtin_bswap32(*tracker) ; 
     ssd_device_node.phandle=phandle;

     tracker = (uint32_t*)interrupt_parent_buffer ;
     phandle = __builtin_bswap32(*tracker) ; 
     ssd_device_node.interrupt_controller_phandle=phandle;
   

     /*Extracting irq line Data ! */

     tracker = (uint32_t*)irq_line_buffer ; 
     int i_c = 0 ; 
     while(i_c!=irq_line_len)
     {
        uint32_t cell = __builtin_bswap32(*tracker);
        ssd_device_node.irq_cells[ssd_device_node.irq_cells_count] = cell; 
        tracker++;
        ssd_device_node.irq_cells_count ++ ;
      i_c=i_c+4;
     }

     /* Extracting The Priority Buffer Now ! */
     ssd_device_node.irq_priority = __builtin_bswap32(*(uint32_t*)priority_buffer);

     uart_puts("==========================\n");
     uart_puthex(ssd_device_node.irq_priority);
  uart_puts("==========================\n");

  /*
   Extracting the Queue Num Value Now ! 
  */

   ssd_device_node.queue_num = __builtin_bswap32(*(uint32_t*)queue_num_buffer);
      uart_puts("==========================\n");
     uart_puthex(ssd_device_node.queue_num);
  uart_puts("==========================\n");

  /*
  Extracting the uniq_id value ! 
  */

  ssd_device_node.uniq_id = __builtin_bswap32(*(uint32_t*)uniq_id_buffer);
      uart_puts("==========RARARARARARARARA================\n");
     uart_puthex(ssd_device_node.uniq_id);
  uart_puts("==========================\n");

     /*
     Copying in devices array and updating the ssd_index_array ! 
     */



     memcopy(&out_info->devices[out_info->device_count] ,&ssd_device_node,sizeof(struct device_entry) );
     ssd_index_array[ssd_index_array_count]=out_info->device_count;
     ssd_index_array_count++;
     out_info->device_count++ ; 

}


void extract_ic_devices_data(uint8_t* reg_buffer , uint16_t reg_len , uint8_t* compatible_buffer , uint8_t* irq_line_buffer , uint8_t irq_line_len , uint8_t* interrupt_parent_buffer ,uint16_t interrupt_parent_len , uint32_t address_cells , uint32_t size_cells , uint32_t interrupt_cells ,const struct ic_compatible_entry ic_compatibles[], struct hardware_info* out_info,uint8_t* phandle_buffer )
{
  /* Extracting MMIO Base and MMIO size Data ! */

  uint32_t* tracker = (uint32_t*)reg_buffer ; 
  uint32_t bytes_read =  0 ; 

  while(bytes_read<reg_len)
  {
     uintptr_t mmio_base = 0 ;
     uintptr_t mmio_size = 0 ; 

     int a_c = 0 ; 
     while(a_c<address_cells)
     {
      uint32_t cell = __builtin_bswap32(*tracker) ; 
      mmio_base = (mmio_base<<32)|cell ; 
      tracker++;
      bytes_read=bytes_read+4;
      a_c++;
     }
       
     int s_c = 0 ;
     while(s_c<size_cells)
     {
      uint32_t cell = __builtin_bswap32(*tracker) ; 
      mmio_size = (mmio_size<<32)|cell ; 
      tracker++;
      bytes_read=bytes_read+4;
      s_c++;
     }

     ic_node.mmio_base_size_array[ic_node.mmio_base_size_array_count] = mmio_base ; 
     ic_node.mmio_base_size_array_count++;
    ic_node.mmio_base_size_array[ic_node.mmio_base_size_array_count] = mmio_size ; 
     ic_node.mmio_base_size_array_count++;

  }


    /*  Extracting Compatible Name and assigning correct  Ic Controller Code ! */

     int i =0 ; 
     while(ic_compatibles[i].compatible!=0&&ic_compatibles[i].ic_code!=0)
     {
      if(str_eq(ic_compatibles[i].compatible,compatible_buffer))
      {
          int len = 0 ;
          while(compatible_buffer[len]!='\0')
          {
            len++;
          }
          len ++;
          memcopy(ic_node.compatible,compatible_buffer,len);
         ic_node.ic_code=ic_compatibles[i].ic_code ; 
          break;
      }
      i++;
     }

     /*Extracting phandle Data ! */

tracker = (uint32_t*)phandle_buffer; 

     uint32_t phandle = __builtin_bswap32(*tracker) ; 
     ic_node.phandle=phandle;

     tracker = (uint32_t*)interrupt_parent_buffer ;
     phandle = __builtin_bswap32(*tracker) ; 
     ic_node.interrupt_controller_phandle=phandle;
   

     /* Populating the out_info devices array and also the ic_indexes_array ! */

   
     ic_node.irq_cells_count=0;

     /* ---- print ic_node before copy ---- */
     uart_puts("\n===== IC NODE (pre-copy) =====\n");
     uart_puts("  phandle    = ");
     uart_puthex(ic_node.phandle);
     uart_puts("\n");
     uart_puts("  compatible = ");
     uart_puts(ic_node.compatible);
     uart_puts("\n");
     uart_puts("  mmio_array_count = ");
     uart_puthex(ic_node.mmio_base_size_array_count);
     uart_puts("\n");
     for (uint32_t p = 0; p + 1 < ic_node.mmio_base_size_array_count; p += 2) {
         uart_puts("    base = ");
         uart_puthex(ic_node.mmio_base_size_array[p]);
         uart_puts("  size = ");
         uart_puthex(ic_node.mmio_base_size_array[p + 1]);
         uart_puts("\n");
     }
     uart_puts("  ic_code fn = ");
     uart_puthex((uintptr_t)ic_node.ic_code);
     uart_puts("\n");


    memcopy( &out_info->devices[out_info->device_count]  , &ic_node , sizeof(struct device_entry));
    ic_index_array[ic_index_array_count]=out_info->device_count ; 
    out_info->device_count++;
    ic_index_array_count++;

}

void extract_mem_sec_value(uint8_t* mem_sec_buffer , struct hardware_info* out_info)
{
   uint32_t value = __builtin_bswap32(*(uint32_t*)mem_sec_buffer);
  out_info->mem_sec = value; 
}


void extract_extensions_metadata_mapped_sectors(struct hardware_info* out_info,
                                                uint8_t* ssd_controller_mapped_sectors_buffer,
                                                uint32_t len_mapped_sectors_buffer)
{
    uint32_t *tracker = (uint32_t*)ssd_controller_mapped_sectors_buffer;

    uint32_t len_end = 0;
    uint32_t i = 0;

    while (len_end < len_mapped_sectors_buffer)
    {
        uint32_t value = __builtin_bswap32(*tracker);
        out_info->ssd_controller_extensions_mapped_sector_array[i] = value;
        i++;
        len_end = len_end + 4;
        tracker++;
    }
}


void extract_extensions_metadata_controllers_uniq_ids(struct hardware_info* out_info,
                                                      uint8_t* ssd_controller_uniq_id_buffer,
                                                      uint32_t len_uniq_id_buffer)
{
    uint32_t *tracker = (uint32_t*)ssd_controller_uniq_id_buffer;

    uint32_t len_end = 0;
    uint32_t i = 0;

    while (len_end < len_uniq_id_buffer)
    {
        uint32_t value = __builtin_bswap32(*tracker);
        out_info->ssd_controller_extensions_uniq_id_array[i] = value;
        i++;
        len_end = len_end + 4;
        tracker++;
    }
}