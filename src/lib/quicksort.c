#include<stdint.h>
#include<stddef.h>


 static void  swap_values (uint8_t * a , uint8_t* b , uintptr_t node_size_bytes)
{
    while(node_size_bytes--)
    {
        uint8_t temp = *a ;
        *a = *b ;
        *b = temp ;
        a++;
        b++;
    }
}



void array_sort(void *array_base_address, 
                uintptr_t node_size_bytes, 
                int array_size, 
                int (*compare_start_address)(const void *a, const void *b))
{
    if (!array_base_address || array_size < 2 || !compare_start_address) return;

    uint8_t *base = (uint8_t *)array_base_address;

    for (int i = 0; i < array_size - 1; i++)
    {
        for (int j = 0; j < array_size - i - 1; j++)
        {
            uint8_t *elem_a = base + (j * node_size_bytes);
            uint8_t *elem_b = base + ((j + 1) * node_size_bytes);

            int result = compare_start_address(elem_a, elem_b);

            // If elem_a > elem_b, swap them to push larger elements right
            if (result > 0)
            {
                swap_values(elem_a, elem_b, node_size_bytes);
            }
        }
    }
}