

#include <stdint.h>
#include<stddef.h>
#include "../../include/driverHeaders/peripheral_struct.h"
#include "../../include/driverHeaders/uart.h"        

/* =========================================================================
 * VirtIO MMIO Register Offsets
 * Base Address: Passed in the peripheral_request_bpt
 * ========================================================================= */

/* --- Identity Registers (Used once at boot to verify the device) --- */
/* Read this to verify you are talking to a VirtIO device. Should be 0x74726976 ("virt"). */
#define VIRTIO_MMIO_MAGIC_VALUE         0x000  

/* Read this to check the version. Must be 2 for modern VirtIO. */
#define VIRTIO_MMIO_VERSION             0x004  

/* Read this to check the device type. 2 means SSD/Block Device. */
#define VIRTIO_MMIO_DEVICE_ID           0x008  

/* --- Feature Negotiation (Used once to agree on capabilities) --- */
/* Read this to see what features the hardware supports. */
#define VIRTIO_MMIO_DEVICE_FEATURES     0x010  

/* Write 0 or 1 here to select which 32-bit half of the device features to read. */
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014  

/* Write here to tell the device which features your driver accepts. */
#define VIRTIO_MMIO_DRIVER_FEATURES     0x020  

/* Write 0 or 1 here to select which 32-bit half of the driver features to write. */
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024  

/* --- Queue Setup (Used once to give the device RAM addresses) --- */
/* Write the queue index (0) here to select which queue you are configuring. */
#define VIRTIO_MMIO_QUEUE_SEL           0x030  

/* Read this to find out the maximum number of slots the device allows. */
#define VIRTIO_MMIO_QUEUE_NUM_MAX       0x034  

/* Write your chosen number of slots here (must be <= MAX). */
#define VIRTIO_MMIO_QUEUE_NUM           0x038  

/* Write 1 here when you are done giving addresses, telling the device to use the queue. */
#define VIRTIO_MMIO_QUEUE_READY         0x044  

/* Write the lower 32 bits of the physical address of the Descriptor Table. */
#define VIRTIO_MMIO_QUEUE_DESC_LOW      0x080  
/* Write the upper 32 bits of the Descriptor Table address. */
#define VIRTIO_MMIO_QUEUE_DESC_HIGH     0x084  

/* Write the lower 32 bits of the physical address of the Available Ring (Work List). */
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW     0x090  
/* Write the upper 32 bits of the Available Ring address. */
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH    0x094  

/* Write the lower 32 bits of the physical address of the Used Ring (Finished List). */
#define VIRTIO_MMIO_QUEUE_USED_LOW      0x0A0  
/* Write the upper 32 bits of the Used Ring address. */
#define VIRTIO_MMIO_QUEUE_USED_HIGH     0x0A4  

/* --- Runtime Operation (Used every time a request is made or finished) --- */
/* Write the queue index (0) here to "ring the doorbell" and tell the device there is a new request. */
#define VIRTIO_MMIO_QUEUE_NOTIFY        0x050  

/* Read this in your interrupt handler to see why the device fired an interrupt. */
#define VIRTIO_MMIO_INTERRUPT_STATUS    0x060  

/* Write the status bits back here to acknowledge the interrupt so the device stops pinging the CPU. */
#define VIRTIO_MMIO_INTERRUPT_ACK       0x064  

/* --- Global Status (Used during initialization handshake) --- */
/* Read/Write here to step through the device initialization sequence. */
#define VIRTIO_MMIO_STATUS              0x070  


/* =========================================================================
 * VirtIO Status Bits (Written to VIRTIO_MMIO_STATUS)
 * ========================================================================= */
/* I see the device */
#define VIRTIO_STATUS_ACKNOWLEDGE       1  
/* I have a driver for it */
#define VIRTIO_STATUS_DRIVER            2  
/* I am ready to start sending requests */
#define VIRTIO_STATUS_DRIVER_OK         4  
/* We have successfully agreed on features */
#define VIRTIO_STATUS_FEATURES_OK       8  
/* Something went horribly wrong */
#define VIRTIO_STATUS_FAILED            128 


int  virtio_setup(struct peripheral_request_bpt *bpt) ; 
void virtio_read_request(struct peripheral_request_bpt *bpt);




void virtio_write_request(struct peripheral_request_bpt *bpt) ; 





int virtio_ssd_driver(struct peripheral_request_bpt *bpt)
{
    uart_puts("\n Hello I am Virtio , MMIO \n");

    /*
  Step-->1   Doing the Setup based on value of Setup flag ! 
    */

    if(!bpt->setup_flag)
    {
       virtio_setup(bpt);
    }

     

    
    /* 
   Step-->2    Deciding the type of Operation ! 
    */
   if(bpt->operation_data_array[0])
   {
       virtio_read_request(bpt);
   }
   else
   {
      virtio_write_request(bpt);
   }

}

void virtio_read_request(struct peripheral_request_bpt *bpt)
{
     
}




void virtio_write_request(struct peripheral_request_bpt *bpt)
{

}


int  virtio_setup(struct peripheral_request_bpt *bpt)
{
       /*
       Step-->1
       Checking for magic value  , version and the device id
       
       */

       uint32_t * reg = NULL ; 
       // reg pointsa towards magic value register ! 
       reg =  ( uint32_t*)(bpt->mmio_base_size_pair_array[0] + VIRTIO_MMIO_MAGIC_VALUE);

       if(*reg != 0x74726976)
       {
          uart_puts("\n Magic Value of Virtio Failed !");
          return 0 ; 
       }

       // reg points towards virtio version !

       reg = ( uint32_t*)(bpt->mmio_base_size_pair_array[0] + VIRTIO_MMIO_VERSION);
       if(*reg != 2)
       {
          uart_puts("\n Modern Version Failed of Virtio , mmio");
          return 0 ; 
       }

       // reg points towards virtio device id or trype

    reg =    ( uint32_t*)(bpt->mmio_base_size_pair_array[0] + VIRTIO_MMIO_DEVICE_ID);

         if(*reg != 2)
       {
          uart_puts("\n It is not SSD/Block type ");
          return 0 ; 
       }

       uart_puts("Magic Value , device type and version is fine ! \n");

}