#include<stdint.h>
#include "../../include/interrupts/ic_bpt.h"
#include "../../include/interrupts/arm_cortex_a15_gic.h"
#include"../../include/driverHeaders/uart.h"

/* =========================================================================
 * GICv2 Distributor (GICD) Register Offsets
 * Base Address: 0x08000000 (from DTB)
 * ========================================================================= */

/* --- General Control --- */
/* Master switch. Write 1 to enable the distributor. */
#define GICD_CTLR           0x000  

/* Tells you how many interrupt lines this GIC supports. */
#define GICD_TYPER          0x004  

/* --- Enable/Disable Lines --- */
/* Write 1 to a bit to ENABLE that interrupt line. 
   Math: offset = 0x100 + (line / 32) * 4 */
#define GICD_ISENABLER      0x100  

/* Write 1 to a bit to DISABLE that interrupt line. 
   Math: offset = 0x180 + (line / 32) * 4 */
#define GICD_ICENABLER      0x180  

/* --- Pending Status (Software Triggering) --- */
/* Write 1 to force an interrupt to be pending (fake an interrupt for testing). 
   Math: offset = 0x200 + (line / 32) * 4 */
#define GICD_ISPENDR        0x200  

/* Write 1 to clear a pending interrupt. 
   Math: offset = 0x280 + (line / 32) * 4 */
#define GICD_ICPENDR        0x280  

/* --- Configuration --- */
/* Sets priority (0-255, lower is higher priority). 1 byte per line. 
   Math: offset = 0x400 + line */
#define GICD_IPRIORITYR     0x400  

/* Sets which CPU core handles the line. 1 byte per line. 
   For single-core, write 0x01. 
   Math: offset = 0x800 + line */
#define GICD_ITARGETSR      0x800  

/* Sets trigger type: 1 = Edge-triggered, 0 = Level-triggered. 2 bits per line. 
   Math: offset = 0xC00 + (line / 16) * 4 */
#define GICD_ICFGR          0xC00  

/* Used to generate Software Generated Interrupts (SGIs) across cores. */
#define GICD_SGIR           0xF00  



/* =========================================================================
 * GICv2 CPU Interface (GICC) Register Offsets
 * Base Address: 0x08010000 (from DTB)
 * ========================================================================= */

/* --- General Control --- */
/* Master switch. Write 1 to allow the GIC to send interrupts to the CPU. */
#define GICC_CTLR           0x000  

/* Priority Mask. Interrupts with a priority value HIGHER (numerically larger) 
   than this mask are blocked. Write 0xFF to allow everything. */
#define GICC_PMR            0x004  

/* Binary Point Register. Controls preemption (nested interrupts). */
#define GICC_BPR            0x008  

/* --- Interrupt Handling --- */
/* READ this to ACKNOWLEDGE an interrupt. Returns the interrupt ID (line number). */
#define GICC_IAR            0x00C  

/* WRITE the interrupt ID back here when done. Tells the GIC "End of Interrupt". */
#define GICC_EOIR           0x010  

/* Shows the priority of the interrupt currently being handled. */
#define GICC_RPR            0x014  

/* Shows the highest priority pending interrupt (without acknowledging it). */
#define GICC_HPPIR          0x018  

/* --- Deactivation (Only needed if using EOI mode 1) --- */
/* Explicitly marks an interrupt as deactivated after handling. */
#define GICC_DIR            0x1000 




void enable_irq_line_set_priority_set_interrupt_type(struct ic_bpt* bpt);
void turn_on_gicc_gicd(struct ic_bpt* bpt);

int arm_cortex_a15_gic(struct ic_bpt * bpt)
{
    uart_puts("\n I am arm cortex gic \n");
    
    switch (bpt->operation_code)
    {
    case 1:
        /* Here we will turn on the particular interrupt ! Means some irq line  */
        enable_irq_line_set_priority_set_interrupt_type(bpt);
        break;

    case 2 :
    /* Here we will turn on the GICC and GICD CTLR registers */
      turn_on_gicc_gicd(bpt);
      return 0 ; 
    break;
    
    default:
        break;
    }
}

void  enable_irq_line_set_priority_set_interrupt_type(struct ic_bpt* bpt)
{

    /* Step -- > 1 Turning interrupt line on ! */
      
       uintptr_t base_address = bpt->mmio_base_mmio_size_pair_array[0]; 
      
       /* Now we will find the correct ISENABLER Register for that particular irq line !  */
       uint32_t irq_line ;

       if(bpt->irq_cells[0]==0)
       {
        // Means type is spi
        irq_line = bpt->irq_cells[1]+32;
       }
       else
       {
        // Type is PPI 
        irq_line = bpt->irq_cells[1]+16;
       }

        int reg_number  = irq_line/32 ; 
        int bit_to_set = irq_line%32 ; 
        
        volatile uint32_t* gicd_isenabler_reg = (volatile uint32_t* )(base_address+(GICD_ISENABLER+reg_number*4));

        /* Now turning the irq line on !*/

        *gicd_isenabler_reg |= (1<<bit_to_set);


       /* Step--> 2   setting the priority for given irq line !    */ 
         volatile uint8_t * priority_reg  = (volatile uint8_t*) (base_address + GICD_IPRIORITYR + irq_line);
         *priority_reg = bpt->priority_value ; 

      /* Step --> 3  setting the trigger type !    */  
      
      reg_number = irq_line/16 ; 
      int bits_to_set = (irq_line%16)*2 ; 
      
      volatile uint32_t * gicd_icfgr_register = (volatile uint32_t*)(base_address+GICD_ICFGR + reg_number*4);

            switch (bpt->irq_cells[2])
      {
      case 1:
        /* The interrupt fires once when the signal goes from low to high.  Write 0b01  */
        *gicd_icfgr_register &= ~(3U << bits_to_set);   /* Clear the 2 bits first */
        *gicd_icfgr_register |= (1U << bits_to_set);    /* Set to 0b01 (Edge) */
        break;

      case 2:
        /* I am omitting the case 2 because it is not valid for spi  */
        break;

      case 4:
        /* Active high level-sensitive. Write 0b00 */
        *gicd_icfgr_register &= ~(3U << bits_to_set);   /* Clear the 2 bits (leaves 0b00) */
        break;

      case 8:
        /* Active low level-sensitive. Write 0b00 */
        *gicd_icfgr_register &= ~(3U << bits_to_set);   /* Clear the 2 bits (leaves 0b00) */
        break;
      
      default:
        break;
      }


}

void turn_on_gicc_gicd(struct ic_bpt* bpt)
{
      /* Step-->1 Turning the GICD on */
      uintptr_t base_address = bpt->mmio_base_mmio_size_pair_array[0];  
      // Skipping the GICD bank size 

      volatile uint32_t* gicd_ctrl = (  volatile uint32_t* )(base_address+GICD_CTLR);
      *gicd_ctrl = 1 ;

      /* Step --> 2 Turning GICC on */
      base_address = bpt->mmio_base_mmio_size_pair_array[2];
    volatile uint32_t* gicc_ctrl = (  volatile uint32_t* )(base_address+GICC_CTLR);
      *gicc_ctrl = 1 ;
 
      /* Step --> 3
      By default, the GICC has a Priority Mask that blocks everything. If we  don't clear it, all interrupts are silently dropped.
 We  need to write 0xFF to the Priority Mask Register  to allow all priority levels through
      */

      volatile uint32_t* gicc_pmr = (  volatile uint32_t* )(base_address+GICC_PMR);
      *gicc_pmr = 0xFF ; 

}