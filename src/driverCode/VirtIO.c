#include <stdint.h>
#include <stddef.h>
#include "../../include/driverHeaders/peripheral_struct.h"
#include "../../include/driverHeaders/uart.h"

/*
 BaseOS k-driver ( kernel driver ) for a virtio-mmio block device ( modern , version 2 ) .
 It reads ONE sector per call and waits for the device in a loop ( polling ) .
 It is only used at boot to fetch data off the disk , so waiting is fine .

 =====================  CONVENTIONS  =====================

 operation_data_array is an array of 20 bytes ( uint8_t ) .

   operation_data_array[0]  tells what the caller wants :
        0  =  tell me the BUFFER size
        1  =  tell me the QUEUE size
        2  =  read one sector

   operation_data_array[1..8]  ( only for operation 2 )
        the sector number , 8 bytes , little endian

 What the driver reads and writes for each operation :

   operation 0  ( buffer size )
        writes  :          bpt->size_of_buffer = size of the buffer in bytes
        The device is not touched , nothing needs to be set up before this call .

   operation 1  ( queue size )
           writes  : bpt->size_of_queue = size of the queue memory in bytes
        The device is not touched , nothing needs to be set up before this call .

   operation 2  ( read one sector )
        reads   : bpt->mmio_base_size_pair_array[0]  base address of the device
                  bpt->queues_address_array[0]       queue memory , size from operation 1 , 16 byte aligned , zeroed
                  bpt->buffer_addresses_array[0]     buffer , size from operation 0
                  bpt->operation_data_array[1..8]    sector number
        writes  : the sector into the buffer
        The first read sets the device up and sets bpt->setup_flag .

   Return value : 1 = success , 0 = failure ( bad request , bad device , io error , timeout ) .

   Boot flow of the kernel :
        1.  call with operation 0 and operation 1 to learn the sizes
        2.  allocate memory of those sizes , mark it occupied in the bitmap , zero it , put the addresses in the bpt
        3.  call with operation 2 once for every sector it needs
        4.  call virtio_ssd_stop when finished , then free the memory

 =========================================================
*/

/* ---------------- VirtIO MMIO register offsets ---------------- */
#define VIRTIO_MMIO_MAGIC_VALUE         0x000
#define VIRTIO_MMIO_VERSION             0x004
#define VIRTIO_MMIO_DEVICE_ID           0x008
#define VIRTIO_MMIO_DEVICE_FEATURES     0x010
#define VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014
#define VIRTIO_MMIO_DRIVER_FEATURES     0x020
#define VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024
#define VIRTIO_MMIO_QUEUE_SEL           0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX       0x034
#define VIRTIO_MMIO_QUEUE_NUM           0x038
#define VIRTIO_MMIO_QUEUE_READY         0x044
#define VIRTIO_MMIO_QUEUE_DESC_LOW      0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH     0x084
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW     0x090
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH    0x094
#define VIRTIO_MMIO_QUEUE_USED_LOW      0x0A0
#define VIRTIO_MMIO_QUEUE_USED_HIGH     0x0A4
#define VIRTIO_MMIO_QUEUE_NOTIFY        0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS    0x060
#define VIRTIO_MMIO_INTERRUPT_ACK       0x064
#define VIRTIO_MMIO_STATUS              0x070

#define VIRTIO_STATUS_ACKNOWLEDGE       1
#define VIRTIO_STATUS_DRIVER            2
#define VIRTIO_STATUS_DRIVER_OK         4
#define VIRTIO_STATUS_FEATURES_OK       8

/* ---------------- Our queue layout : one request at a time ---------------- */
#define QUEUE_SIZE          4                       // one request uses 3 descriptors ! 
#define AVAIL_OFFSET        64
#define USED_OFFSET         128
#define HEADER_OFFSET       192
#define STATUS_OFFSET       208
#define QUEUE_BYTES_NEEDED  224

#define AVAIL_F_NO_INTERRUPT 1                      // tells the device not to raise its interrupt ! 
#define DESC_F_NEXT         1
#define DESC_F_WRITE        2                       // the device writes into this buffer ! 
#define SECTOR_SIZE         512                     // virtio always counts sectors in 512 bytes ! 
#define SPIN_LIMIT          50000000

#define OPERATION_BUFFER_SIZE   0       // asks : how many bytes must the buffer be ! 
#define OPERATION_QUEUE_SIZE    1       // asks : how many bytes must the queue memory be ! 
#define OPERATION_READ_SECTOR   2       // reads one sector ! 

struct virtq_desc { uint64_t addr ; uint32_t len ; uint16_t flags ; uint16_t next ; } ;
struct virtq_avail { uint16_t flags ; uint16_t idx ; uint16_t ring[QUEUE_SIZE] ; } ;
struct virtq_used_elem { uint32_t id ; uint32_t len ; } ;
struct virtq_used { uint16_t flags ; uint16_t idx ; struct virtq_used_elem ring[QUEUE_SIZE] ; } ;
struct blk_request_header { uint32_t type ; uint32_t reserved ; uint64_t sector ; } ;

static inline uint32_t virtio_read_register(uintptr_t base , uint32_t offset)
{
    return *(volatile uint32_t*)(base + offset) ;
}

static inline void virtio_write_register(uintptr_t base , uint32_t offset , uint32_t value)
{
    *(volatile uint32_t*)(base + offset) = value ;
}

int virtio_setup(struct peripheral_request_bpt *bpt) ;
int virtio_read_sector(struct peripheral_request_bpt *bpt) ;

int virtio_ssd_driver(struct peripheral_request_bpt *bpt)
{
    uint8_t operation = bpt->operation_data_array[0] ;

    // These two only answer a question , they do not touch the device ! 
        if(operation == OPERATION_BUFFER_SIZE)
    {
        bpt->size_of_buffer = SECTOR_SIZE ;
        return 1 ;
    }
    if(operation == OPERATION_QUEUE_SIZE)
    {
        bpt->size_of_queue = QUEUE_BYTES_NEEDED ;
        return 1 ;
    }
    if(!bpt->setup_flag)
    {
        if(!virtio_setup(bpt))
        {
            return 0 ;
        }
    }
    return virtio_read_sector(bpt) ;
}

// Call this when the boot work is finished , the device forgets our queue ! 
void virtio_ssd_stop(struct peripheral_request_bpt *bpt)
{
    virtio_write_register(bpt->mmio_base_size_pair_array[0] , VIRTIO_MMIO_STATUS , 0) ;
    bpt->setup_flag = 0 ;
}

int virtio_setup(struct peripheral_request_bpt *bpt)
{
    uintptr_t base  = bpt->mmio_base_size_pair_array[0] ;
    uintptr_t queue = bpt->queues_address_array[0] ;

    if(virtio_read_register(base , VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976)
    {
        uart_puts("\n Magic Value of Virtio Failed !") ;
        return 0 ;
    }
    if(virtio_read_register(base , VIRTIO_MMIO_VERSION) != 2)
    {
        uart_puts("\n Modern Version Failed of Virtio , mmio") ;
        return 0 ;
    }
    if(virtio_read_register(base , VIRTIO_MMIO_DEVICE_ID) != 2)
    {
        uart_puts("\n It is not SSD/Block type ") ;
        return 0 ;
    }

    // Zero the queue memory , the device must not see garbage ! 
    for(uint32_t i = 0 ; i < QUEUE_BYTES_NEEDED ; i += 4)
    {
        *(volatile uint32_t*)(queue + i) = 0 ;
    }

    // We poll , so ask the device not to raise its interrupt ! 
    ((volatile struct virtq_avail*)(queue + AVAIL_OFFSET))->flags = AVAIL_F_NO_INTERRUPT ;

    // Handshake ! 
    virtio_write_register(base , VIRTIO_MMIO_STATUS , 0) ;
    virtio_write_register(base , VIRTIO_MMIO_STATUS , VIRTIO_STATUS_ACKNOWLEDGE) ;
    virtio_write_register(base , VIRTIO_MMIO_STATUS , VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER) ;

    // Modern devices need the VERSION_1 feature , we accept only that ! 
    virtio_write_register(base , VIRTIO_MMIO_DEVICE_FEATURES_SEL , 1) ;
    if(!(virtio_read_register(base , VIRTIO_MMIO_DEVICE_FEATURES) & 1))
    {
        uart_puts("\n Virtio device does not support VERSION_1 ") ;
        return 0 ;
    }

    virtio_write_register(base , VIRTIO_MMIO_DRIVER_FEATURES_SEL , 0) ;
    virtio_write_register(base , VIRTIO_MMIO_DRIVER_FEATURES , 0) ;
    virtio_write_register(base , VIRTIO_MMIO_DRIVER_FEATURES_SEL , 1) ;
    virtio_write_register(base , VIRTIO_MMIO_DRIVER_FEATURES , 1) ;

    virtio_write_register(base , VIRTIO_MMIO_STATUS , VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK) ;
    if(!(virtio_read_register(base , VIRTIO_MMIO_STATUS) & VIRTIO_STATUS_FEATURES_OK))
    {
        uart_puts("\n Virtio rejected our features ") ;
        return 0 ;
    }

    // Queue 0 ! 
    virtio_write_register(base , VIRTIO_MMIO_QUEUE_SEL , 0) ;
    if(virtio_read_register(base , VIRTIO_MMIO_QUEUE_NUM_MAX) < QUEUE_SIZE)
    {
        uart_puts("\n Virtio queue is too small ") ;
        return 0 ;
    }
    virtio_write_register(base , VIRTIO_MMIO_QUEUE_NUM , QUEUE_SIZE) ;

    uint64_t desc_address  = (uint64_t)queue ;
    uint64_t avail_address = (uint64_t)queue + AVAIL_OFFSET ;
    uint64_t used_address  = (uint64_t)queue + USED_OFFSET ;

    virtio_write_register(base , VIRTIO_MMIO_QUEUE_DESC_LOW   , (uint32_t)desc_address) ;
    virtio_write_register(base , VIRTIO_MMIO_QUEUE_DESC_HIGH  , (uint32_t)(desc_address >> 32)) ;
    virtio_write_register(base , VIRTIO_MMIO_QUEUE_AVAIL_LOW  , (uint32_t)avail_address) ;
    virtio_write_register(base , VIRTIO_MMIO_QUEUE_AVAIL_HIGH , (uint32_t)(avail_address >> 32)) ;
    virtio_write_register(base , VIRTIO_MMIO_QUEUE_USED_LOW   , (uint32_t)used_address) ;
    virtio_write_register(base , VIRTIO_MMIO_QUEUE_USED_HIGH  , (uint32_t)(used_address >> 32)) ;

    virtio_write_register(base , VIRTIO_MMIO_QUEUE_READY , 1) ;
    virtio_write_register(base , VIRTIO_MMIO_STATUS , VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK) ;

    bpt->setup_flag = 1 ;
    return 1 ;
}

int virtio_read_sector(struct peripheral_request_bpt *bpt)
{
    uintptr_t base   = bpt->mmio_base_size_pair_array[0] ;
    uintptr_t queue  = bpt->queues_address_array[0] ;
    uintptr_t buffer = bpt->buffer_addresses_array[0] ;

    // Sector number , 8 bytes little endian , bytes 1 to 8 ! 
    uint64_t sector = 0 ;
    for(int i = 0 ; i < 8 ; i++)
    {
        sector |= ((uint64_t)bpt->operation_data_array[1 + i]) << (8 * i) ;
    }

    volatile struct virtq_desc*         desc   = (volatile struct virtq_desc*)queue ;
    volatile struct virtq_avail*        avail  = (volatile struct virtq_avail*)(queue + AVAIL_OFFSET) ;
    volatile struct virtq_used*         used   = (volatile struct virtq_used*)(queue + USED_OFFSET) ;
    volatile struct blk_request_header* header = (volatile struct blk_request_header*)(queue + HEADER_OFFSET) ;
    volatile uint8_t*                   status = (volatile uint8_t*)(queue + STATUS_OFFSET) ;

    header->type     = 0 ;          // 0 = read ! 
    header->reserved = 0 ;
    header->sector   = sector ;
    *status          = 0xFF ;       // the device overwrites this , 0 means ok ! 

    // Chain of three : header -- > our buffer -- > status byte ! 
    desc[0].addr  = (uint64_t)(queue + HEADER_OFFSET) ;
    desc[0].len   = sizeof(struct blk_request_header) ;
    desc[0].flags = DESC_F_NEXT ;
    desc[0].next  = 1 ;

    desc[1].addr  = (uint64_t)buffer ;
    desc[1].len   = SECTOR_SIZE ;
    desc[1].flags = DESC_F_NEXT | DESC_F_WRITE ;
    desc[1].next  = 2 ;

    desc[2].addr  = (uint64_t)(queue + STATUS_OFFSET) ;
    desc[2].len   = 1 ;
    desc[2].flags = DESC_F_WRITE ;
    desc[2].next  = 0 ;

    // Give the chain to the device , then ring the doorbell ! 
    uint16_t used_before = used->idx ;
    uint16_t slot        = avail->idx ;

    avail->ring[slot % QUEUE_SIZE] = 0 ;
    __asm__ volatile("dmb sy" ::: "memory") ;
    avail->idx = (uint16_t)(slot + 1) ;
    __asm__ volatile("dmb sy" ::: "memory") ;

    virtio_write_register(base , VIRTIO_MMIO_QUEUE_NOTIFY , 0) ;

    // Wait until the device says it is done ! 
    uint32_t spins = SPIN_LIMIT ;
    while(used->idx == used_before)
    {
        if(--spins == 0)
        {
            uart_puts("\n Virtio read timed out ") ;
            return 0 ;
        }
    }
    __asm__ volatile("dmb sy" ::: "memory") ;

    // Clear the device's interrupt flag so its line does not stay raised ! 
    virtio_write_register(base , VIRTIO_MMIO_INTERRUPT_ACK , virtio_read_register(base , VIRTIO_MMIO_INTERRUPT_STATUS)) ;

    if(*status != 0)
    {
        uart_puts("\n Virtio read failed ") ;
        return 0 ;
    }
    return 1 ;
}