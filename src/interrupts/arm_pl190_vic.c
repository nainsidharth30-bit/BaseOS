#include<stdint.h>
#include "../../include/interrupts/ic_bpt.h"
#include "../../include/interrupts/arm_pl190_vic.h"
#include"../../include/driverHeaders/uart.h"



/* =========================================================================
 * PL190 VIC (Vectored Interrupt Controller) Register Offsets
 * All registers are 32-bit (4 bytes) wide.
 * ========================================================================= */

/* --- Status Registers (Read-Only) --- */
/* Shows which lines are actively requesting an IRQ right now. */
#define VIC_IRQ_STATUS          0x000  

/* Shows which lines are actively requesting an FIQ right now.  */
#define VIC_FIQ_STATUS          0x004  

/* Shows which lines are firing BEFORE masking. Use this to debug devices that you forgot to enable. */
#define VIC_RAW_INTR            0x008  

/* --- Routing Registers --- */
/* Routes lines to IRQ (write 0) or FIQ (write 1). Leave all bits at 0. */
#define VIC_INT_SELECT          0x00C  

/* --- Enable/Disable Registers (The Master Switches) --- */
/* Write a 1 to bit N to TURN ON the interrupt for line N. Writing 0 does nothing. */
#define VIC_INT_ENABLE          0x010  

/* Write a 1 to bit N to TURN OFF the interrupt for line N. */
#define VIC_INT_ENABLE_CLEAR    0x014  

/* --- Software Interrupt Registers (For Testing) --- */
/* Write a 1 to bit N to fake an interrupt on line N without any hardware. */
#define VIC_SOFT_INT            0x018  

/* Write a 1 to bit N to cancel the fake interrupt. */
#define VIC_SOFT_INT_CLEAR      0x01C  

/* --- Protection Register --- */
/* Enables protected mode. Leave at 0. */
#define VIC_PROTECTION          0x020  

/* --- Vectoring Registers (The Core Mechanism) --- */
/* Read: Gets the memory address of the ISR for the current interrupt, AND acknowledges it. 
   Write: Anything (e.g., 0) to signal "End of Interrupt" so the VIC can deliver the next one. */
#define VIC_VECT_ADDR           0x030  

/* If a line fires but isn't assigned to a slot, the VIC returns this fallback address. */
#define VIC_DEF_VECT_ADDR       0x034  

/* --- Vector Priority Slots (16 slots, 0 = highest priority) --- */
/* Write the memory address of your handler function (ISR) into these slots. */
#define VIC_VECT_ADDR0          0x100  
#define VIC_VECT_ADDR1          0x104  
#define VIC_VECT_ADDR2          0x108  
#define VIC_VECT_ADDR3          0x10C  
#define VIC_VECT_ADDR4          0x110  
#define VIC_VECT_ADDR5          0x114  
#define VIC_VECT_ADDR6          0x118  
#define VIC_VECT_ADDR7          0x11C  
#define VIC_VECT_ADDR8          0x120  
#define VIC_VECT_ADDR9          0x124  
#define VIC_VECT_ADDR10         0x128  
#define VIC_VECT_ADDR11         0x12C  
#define VIC_VECT_ADDR12         0x130  
#define VIC_VECT_ADDR13         0x134  
#define VIC_VECT_ADDR14         0x138  
#define VIC_VECT_ADDR15         0x13C  

/* Write to these to link a specific interrupt line to a priority slot. 
   Bits 0-4 = Line number (0-31). Bit 5 = Enable bit (write 1 to turn this slot on). */
#define VIC_VECT_CNTL0          0x200  
#define VIC_VECT_CNTL1          0x204  
#define VIC_VECT_CNTL2          0x208  
#define VIC_VECT_CNTL3          0x20C  
#define VIC_VECT_CNTL4          0x210  
#define VIC_VECT_CNTL5          0x214  
#define VIC_VECT_CNTL6          0x218  
#define VIC_VECT_CNTL7          0x21C  
#define VIC_VECT_CNTL8          0x220  
#define VIC_VECT_CNTL9          0x224  
#define VIC_VECT_CNTL10         0x228  
#define VIC_VECT_CNTL11         0x22C  
#define VIC_VECT_CNTL12         0x230  
#define VIC_VECT_CNTL13         0x234  
#define VIC_VECT_CNTL14         0x238  
#define VIC_VECT_CNTL15         0x23C  

/* Bit definition to enable a vector slot when writing to VIC_VECT_CNTL */
#define VIC_VECT_CNTL_ENABLE    (1 << 5)  

/* --- ID Registers (Read-Only) --- */
/* Hardcoded hardware ID. Read these to verify you are actually talking to a PL190. */
#define VIC_PERIPHID0           0xFE0  
#define VIC_PERIPHID1           0xFE4  
#define VIC_PERIPHID2           0xFE8  
#define VIC_PERIPHID3           0xFEC  
#define VIC_PCELLID0            0xFF0  
#define VIC_PCELLID1            0xFF4  
#define VIC_PCELLID2            0xFF8  
#define VIC_PCELLID3            0xFFC  





int arm_pl190_vic(struct ic_bpt * bpt)
{
     uart_puts("\n I am arm cortex vic \n");

     switch (bpt->operation_code)
     {
     case 1:
        break;
     
     
     }

     return 0; 
    
}





