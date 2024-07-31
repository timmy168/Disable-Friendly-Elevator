#include "bitmap.h"

//Find the first element's index in bitmap that is 0
void find_bitmap_nearest_0_index(unsigned int bitmap, int *nearest_index)
{
    *nearest_index = 0;
    while (((~bitmap & 0xffffffff) & 1 << *nearest_index) == 0)
    {
        *nearest_index += 1;
    }
}

// Find all the index in bit map that is 1 
void find_all_bitmap_1_index(unsigned int bitmap, int *one_count, int *one_index)
{
    *one_count = 0;
    for (int i = 0; i < 32; i++)
    {
        if (bitmap & (1 << i))
        {
            one_index[(*one_count)++] = i;
        }
    }
}

// Set specific digit in bit map to 1
void set_bitmap_1(unsigned int *bitmap, int bitmap_index)
{
    *bitmap |= 1u << bitmap_index;
}

// Set specific digit in bit map to 0
void set_bitmap_0(unsigned int *bitmap, int bitmap_index)
{
    *bitmap ^= 1u << bitmap_index;
}