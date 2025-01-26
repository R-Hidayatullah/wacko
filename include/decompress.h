#ifndef DECOMPRESS_H
#define DECOMPRESS_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

typedef struct
{
    uint8_t *data;
    size_t size;
    size_t byte_pos;
    int bit_pos;
} BitReader;

// Initialize the BitReader
void init_bit_reader(BitReader *reader, uint8_t *data, size_t size)
{
    reader->data = data;
    reader->size = size;
    reader->byte_pos = 0;
    reader->bit_pos = 8;
}

// Read the next n bits into a uint32_t (or smaller if n < 32), from right to left
uint32_t read_bits(BitReader *reader, int n)
{
    uint32_t result = 0;

    // Read n bits
    for (int i = 0; i < n; i++)
    {
        if (reader->byte_pos >= reader->size)
        {
            return -1; // End of data
        }

        if (reader->bit_pos == 8)
        {
            reader->bit_pos = 0;
        }

        // Shift the result to make room for the next bit (to the right)
        result |= ((reader->data[reader->byte_pos] >> (7 - reader->bit_pos)) & 1) << i;

        reader->bit_pos++;

        if (reader->bit_pos == 8)
        {
            reader->byte_pos++;
        }
    }

    return result;
}

// Drop the next n bits
void drop_bits(BitReader *reader, int n)
{
    for (int i = 0; i < n; i++)
    {
        read_bits(reader, 1); // Just read and discard bits
    }
}

uint8_t *decompress_data(uint8_t *compressed_data, uint32_t compressed_size, uint32_t *decompressed_size)
{
    if (compressed_data == NULL)
    {
        printf("There is no compressed data!\n");
        return NULL; // Return NULL to indicate an error
    }

    BitReader bit_reader;

    init_bit_reader(&bit_reader, compressed_data, compressed_size);

    uint32_t uncompressed_size = 0;

    // Drop the first 32 bits (already consumed)
    drop_bits(&bit_reader, 32);

    // Read the next 32 bits to determine uncompressed size
    uncompressed_size = read_bits(&bit_reader, 32);

    // Drop the 32 bits we just read
    drop_bits(&bit_reader, 32);
    printf("Compressed size : %d \n", compressed_size);
    printf("Decompressed size : %d \n", uncompressed_size);
    // If the caller provided a pointer for decompressed size, store the size there
    if (decompressed_size != NULL)
    {
        *decompressed_size = uncompressed_size;
    }

    // Allocate memory for decompressed data
    uint8_t *decompressed_data = (uint8_t *)malloc(uncompressed_size);
    if (decompressed_data == NULL)
    {
        printf("Memory allocation failed!\n");
        return NULL; // Return NULL if allocation fails
    }

    // TODO: Implement the actual decompression logic here

    return decompressed_data; // Return the allocated buffer (currently empty)
}

#endif // DECOMPRESS_H
