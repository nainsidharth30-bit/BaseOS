#include <stdint.h>
#include "../../include/driverHeaders/uart.h"
#include "../../include/mkMAU/mkMAU.h"
#include "../../include/lib/dbt.h"
#include "../../include/mkMAU/mobilemkMAU.h"
#include "../../include/mkMAU/mkmau_utils.h"
#include "../../include/driverHeaders/track_stack.h"
#include "../../include/lib/quicksort.h"
#include "../../include/lib/compare_start_address.h"
#include "../../include/interrupts/ic_bpt.h"
#include "../../include/lib/alignbyte.h"
#include "stddef.h"

/* Minimum extension memory request should be this */
#define MMAU_SIZE 4096  

/* Fixed sizes for kernel buffer and SSD queue, replacing the old globals */
#define KERNEL_BUFFER_SIZE  516    /* 512-byte sector + 4 safety bytes */
#define SSD_QUEUE_SIZE      6656   /* 256 slots * 26 bytes per slot */

/* Tracks free and occupied available memory for extensions */
struct mkmau_node *memory_tracker_array = NULL;
int memory_tracker_array_size ;

/* Array node shifting function */
extern void array_node_shift_end(struct mkmau_node *memory_tracker_array,size_t shift_units,size_t from_which_node, size_t array_size);

/* Finds a free range of memory based on the merged_array */
int mkMAU_find_free_range_for_range_nodes_array(uintptr_t kernel_start_address,  uint32_t merge_array_count,  struct reserved_region *merged_array, uintptr_t number_of_bytes, uintptr_t *free_range_start_address,uintptr_t *free_range_end_address,struct hardware_info *out_info, uint8_t byte_align);


void mkMAU_main(struct hardware_info *out_info, uintptr_t x1_register)
{
    extern char _kernel_start[];
    extern char _kernel_end[];

    uintptr_t kernel_start = (uintptr_t)_kernel_start;
    uintptr_t kernel_end   = (uintptr_t)_kernel_end;
    size_t    kernel_size  = kernel_end - kernel_start;

    uint64_t size_of_reserved_regions = 0;

    /* Calculate total reserved region size */
    reserved_region_ranges_size(out_info, &size_of_reserved_regions);

    uart_puts("\n size of Resreved Regions memory \n");
    uart_puthex((uintptr_t)size_of_reserved_regions);
    uart_puts("\n \n");

    /* Free RAM available to extensions, based on kernel + reserved regions only */
    uint64_t size_of_available_ram = out_info->ram_size
                                     - ((uint64_t)kernel_size + size_of_reserved_regions);

    uart_puts("\n size of free memory \n");
    uart_puthex((uintptr_t)size_of_available_ram);
    uart_puts("\n \n");

    uart_puts("\n size of kernel \n");
    uart_puthex((uintptr_t)kernel_size);
    uart_puts("\n \n");

    /* Max number of tracker nodes */
    uintptr_t total_number_of_possible_nodes = size_of_available_ram / MMAU_SIZE;

    uintptr_t free_range_start_address;
    uintptr_t free_range_end_address;
    
    /* Local variables to hold the allocated addresses */
    uintptr_t local_kernel_buffer_address;
    uintptr_t local_ssd_queue_address;

    /* merged_array holds all currently occupied memory */
    struct reserved_region merged_array[(out_info->rsv_count + 1 + 1 + 1 + 1)];

    uart_puts("hello");
    track_stack();

    struct reserved_region temporaray_array[(out_info->rsv_count + 1)];

    /* Copy reserved regions into temporary_array */
    int i = 0;
    while (i < out_info->rsv_count)
    {
        temporaray_array[i].start = out_info->rsv_regions[i].start;
        temporaray_array[i].end   = out_info->rsv_regions[i].end;
        i++;
    }
    temporaray_array[i].start = kernel_start;
    temporaray_array[i].end   = kernel_end;

    array_sort(temporaray_array, sizeof(struct reserved_region),
               out_info->rsv_count + 1, compare_start_address);

    uint32_t merge_array_count = mkmau_merge_array_rsv_regions(
        temporaray_array, out_info->rsv_count + 1, merged_array);

    uart_puthex(merge_array_count);

    /* Print step 1: kernel + reserved regions */
    int temp = merge_array_count;
    uart_puts("\n Kernel inserted !  \n");
    while (temp != -1)
    {
        uart_puts("\n Region Start = "); uart_puthex(merged_array[temp].start);
        uart_puts("\n Region End   = "); uart_puthex(merged_array[temp].end);
        temp--;
    }

    /* Reserve tracker array */
    merge_array_count = mkMAU_find_free_range_for_range_nodes_array(
        kernel_start, merge_array_count, merged_array,
        total_number_of_possible_nodes * sizeof(struct mkmau_node),
        &free_range_start_address, &free_range_end_address, out_info, 0);

    memory_tracker_array = (struct mkmau_node *)free_range_start_address;

    array_sort(merged_array, sizeof(struct reserved_region),
               merge_array_count + 1, compare_start_address);
    int j = 0;
    while (j < merge_array_count + 1)
    {
        temporaray_array[j].start = merged_array[j].start;
        temporaray_array[j].end   = merged_array[j].end;
        j++;
    }
    merge_array_count = mkmau_merge_array_rsv_regions(
        temporaray_array, merge_array_count + 1, merged_array);

    temp = merge_array_count;
    uart_puts("\n memory_tracker_array inserted !  \n");
    while (temp != -1)
    {
        uart_puts("\n Region start = "); uart_puthex(merged_array[temp].start);
        uart_puts("\n Region end   = "); uart_puthex(merged_array[temp].end);
        temp--;
    }

    /* Reserve kernel buffer */
    merge_array_count = mkMAU_find_free_range_for_range_nodes_array(
        kernel_start, merge_array_count, merged_array,
        KERNEL_BUFFER_SIZE,
        &free_range_start_address, &free_range_end_address, out_info, 0);

    local_kernel_buffer_address = free_range_start_address;

    array_sort(merged_array, sizeof(struct reserved_region),
               merge_array_count + 1, compare_start_address);
    int jk = 0;
    while (jk < merge_array_count + 1)
    {
        temporaray_array[jk].start = merged_array[jk].start;
        temporaray_array[jk].end   = merged_array[jk].end;
        jk++;
    }
    merge_array_count = mkmau_merge_array_rsv_regions(
        temporaray_array, merge_array_count + 1, merged_array);

    temp = merge_array_count;
    uart_puts("\n Kernel Buffer inserted at: "); uart_puthex(local_kernel_buffer_address);
    uart_puts("\n");
    while (temp != -1)
    {
        uart_puts("\n Region start = "); uart_puthex(merged_array[temp].start);
        uart_puts("\n Region end   = "); uart_puthex(merged_array[temp].end);
        temp--;
    }

    /* Reserve SSD queue */
    merge_array_count = mkMAU_find_free_range_for_range_nodes_array(
        kernel_start, merge_array_count, merged_array,
        SSD_QUEUE_SIZE,
        &free_range_start_address, &free_range_end_address, out_info, 1);

    local_ssd_queue_address = free_range_start_address;

    array_sort(merged_array, sizeof(struct reserved_region),
               merge_array_count + 1, compare_start_address);
    int k = 0;
    while (k < merge_array_count + 1)
    {
        temporaray_array[k].start = merged_array[k].start;
        temporaray_array[k].end   = merged_array[k].end;
        k++;
    }
    merge_array_count = mkmau_merge_array_rsv_regions(
        temporaray_array, merge_array_count + 1, merged_array);

    temp = merge_array_count;
    uart_puts("\n SSD QUEUE inserted at: "); uart_puthex(local_ssd_queue_address);
    uart_puts("\n");
    while (temp != -1)
    {
        uart_puts("\n Region start = "); uart_puthex(merged_array[temp].start);
        uart_puts("\n Region end   = "); uart_puthex(merged_array[temp].end);
        temp--;
    }
}


void array_node_shift_end(struct mkmau_node *memory_tracker_array,
                           size_t shift_units,
                           size_t from_which_node,
                           size_t array_size)
{
    if (array_size <= 1 || from_which_node >= array_size - 1) {
        return;
    }

    size_t copy_iterator = array_size - 1;

    while (copy_iterator != from_which_node)
    {
        struct mkmau_node node;
        node = memory_tracker_array[copy_iterator];
        memory_tracker_array[copy_iterator + 1] = node;
        copy_iterator--;
    }
}


int mkMAU_find_free_range_for_range_nodes_array(uintptr_t kernel_start_address,
                                                uint32_t merge_array_count,
                                                struct reserved_region *merged_array,
                                                uintptr_t number_of_bytes,
                                                uintptr_t *free_range_start_address,
                                                uintptr_t *free_range_end_address,
                                                struct hardware_info *out_info,
                                                uint8_t byte_align)
{
    uintptr_t start_address = out_info->ram_base_address;
    uintptr_t end_address;

    uint32_t iterator = 0;

    while (iterator <= merge_array_count)
    {
        end_address = merged_array[iterator].start;

        if (start_address == end_address)
        {
            start_address = merged_array[iterator].end;
            iterator++;
            continue;
        }
        else if (start_address < end_address)
        {
            if (end_address - start_address >= number_of_bytes)
            {
                *free_range_start_address = start_address;

                if (byte_align)
                {
                    byte_alignemnt_by_4096((void *)free_range_start_address);

                    if (*free_range_start_address + number_of_bytes <= end_address)
                    {
                        *free_range_end_address = *free_range_start_address + number_of_bytes;
                        merge_array_count++;
                        merged_array[merge_array_count].start = *free_range_start_address;
                        merged_array[merge_array_count].end   = *free_range_start_address + number_of_bytes;
                        return merge_array_count;
                    }
                    else
                    {
                        start_address = merged_array[iterator].end;
                        iterator++;
                        continue;
                    }
                }

                *free_range_end_address = *free_range_start_address + number_of_bytes;
                merge_array_count++;
                merged_array[merge_array_count].start = *free_range_start_address;
                merged_array[merge_array_count].end   = *free_range_start_address + number_of_bytes;
                return merge_array_count;
            }
        }
    }
}