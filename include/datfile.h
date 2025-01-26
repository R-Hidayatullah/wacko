#ifndef DATFILE_H
#define DATFILE_H

#include "decompress.h"

#define DAT_MAGIC_NUMBER 3
#define MFT_MAGIC_NUMBER 4
#define MFT_ENTRY_INDEX_NUM 2

typedef struct
{
    uint8_t version;
    uint8_t identifier[DAT_MAGIC_NUMBER];
    uint32_t header_size;
    uint32_t unknown_field;
    uint32_t chunk_size;
    uint32_t crc;
    uint32_t unknown_field_2;
    uint64_t mft_offset;
    uint32_t mft_size;
    uint32_t flags;
} DatHeader;

typedef struct
{
    uint8_t identifier[MFT_MAGIC_NUMBER];
    uint64_t unknown;
    uint32_t num_entries;
    uint32_t unknown_field_2;
    uint32_t unknown_field_3;
} MFTHeader;

typedef struct
{
    uint64_t offset;
    uint32_t size;
    uint16_t compression_flag;
    uint16_t entry_flag;
    uint32_t counter;
    uint32_t crc;
} MFTData;

typedef struct
{
    uint32_t file_id;
    uint32_t base_id;
} MFTIndexData;

typedef struct
{
    DatHeader header;
    MFTHeader mft_header;
    MFTData *mft_data;
    MFTIndexData *mft_index_data;
} DatFile;

// Function to read little-endian unsigned integers
uint16_t read_uint16_le(FILE *file)
{
    uint16_t value;
    fread(&value, sizeof(value), 1, file);
    return value;
}

uint32_t read_uint32_le(FILE *file)
{
    uint32_t value;
    fread(&value, sizeof(value), 1, file);
    return value;
}

int32_t read_int32_le(FILE *file)
{
    int32_t value;
    fread(&value, sizeof(value), 1, file);
    return value;
}

uint64_t read_uint64_le(FILE *file)
{
    uint64_t value;
    fread(&value, sizeof(value), 1, file);
    return value;
}

void debug_print_header(const DatHeader *header)
{
    printf("Header Debug Info:\n");
    printf("  Header Size:       %d bytes\n", header->header_size); // Unsigned integer
    printf("  Unknown Field:     %d \n", header->unknown_field);    // 32-bit hex
    printf("  Chunk Size:        %d bytes\n", header->chunk_size);  // Unsigned integer
    printf("  CRC:               %d \n", header->crc);              // 32-bit hex
    printf("  Unknown Field 2:   %d \n", header->unknown_field_2);  // 32-bit hex
    printf("  MFT Offset:        %llu \n", header->mft_offset);     // 64-bit hex
    printf("  MFT Size:          %d bytes\n", header->mft_size);    // Unsigned integer
    printf("  Flags:             %d \n", header->flags);            // 32-bit hex
}

void debug_print_mft_header(const MFTHeader *header)
{
    printf("MFT Header Debug Info:\n");
    printf("  Unknown:           %llu\n", header->unknown);       // 64-bit hex
    printf("  Number of Entries: %d\n", header->num_entries);     // Unsigned integer
    printf("  Unknown Field 2:   %d\n", header->unknown_field_2); // 32-bit hex
    printf("  Unknown Field 3:   %d\n", header->unknown_field_3); // 32-bit hex
}

void debug_print_mft_data(const MFTData *data, uint32_t index)
{
    printf("MFTData[%d] Debug Info:\n", index);
    printf("  Offset:            %llu\n", data->offset);         // 64-bit hex
    printf("  Size:              %d bytes\n", data->size);       // Unsigned integer
    printf("  Compression Flag:  %d\n", data->compression_flag); // 16-bit hex
    printf("  Entry Flag:        %d\n", data->entry_flag);       // 16-bit hex
    printf("  Counter:           %d\n", data->counter);          // Unsigned integer
    printf("  CRC:               %d\n", data->crc);              // 32-bit hex
}

void debug_print_mft_index_data(const MFTIndexData *data, uint32_t index)
{
    printf("MFTIndexData[%d] Debug Info:\n", index);
    printf("  File ID:           %d\n", data->file_id); // Unsigned integer
    printf("  Base ID:           %d\n", data->base_id); // Unsigned integer
}

// Function to load .dat file and populate DatFile structure
void load_dat_file(const char *file_path, DatFile *dat_file)
{
    if (!strstr(file_path, ".dat"))
    {
        fprintf(stderr, "Invalid file extension. Expected '.dat'.\n");
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen(file_path, "rb");
    if (!file)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fread(&dat_file->header.version, sizeof(uint8_t), 1, file);
    fread(dat_file->header.identifier, sizeof(uint8_t), DAT_MAGIC_NUMBER, file);
    dat_file->header.header_size = read_uint32_le(file);
    dat_file->header.unknown_field = read_uint32_le(file);
    dat_file->header.chunk_size = read_uint32_le(file);
    dat_file->header.crc = read_uint32_le(file);
    dat_file->header.unknown_field_2 = read_uint32_le(file);
    dat_file->header.mft_offset = read_uint64_le(file);
    dat_file->header.mft_size = read_uint32_le(file);
    dat_file->header.flags = read_uint32_le(file);
    debug_print_header(&dat_file->header);

    fseeko(file, dat_file->header.mft_offset, SEEK_SET);
    fread(dat_file->mft_header.identifier, sizeof(uint8_t), MFT_MAGIC_NUMBER, file);
    dat_file->mft_header.unknown = read_uint64_le(file);
    dat_file->mft_header.num_entries = read_uint32_le(file);
    dat_file->mft_header.unknown_field_2 = read_uint32_le(file);
    dat_file->mft_header.unknown_field_3 = read_uint32_le(file);
    debug_print_mft_header(&dat_file->mft_header);
    if (memcmp(dat_file->mft_header.identifier, (uint8_t[]){0x4D, 0x66, 0x74, 0x1A}, MFT_MAGIC_NUMBER) != 0)
    {
        fprintf(stderr, "Not a MFT file: invalid header magic\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    dat_file->mft_data = (MFTData *)malloc(dat_file->mft_header.num_entries * sizeof(MFTData));
    if (dat_file->mft_data == NULL)
    {
        fprintf(stderr, "Memory allocation failed for MFTData\n");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    for (uint32_t i = 1; i < dat_file->mft_header.num_entries; ++i)
    {
        dat_file->mft_data[i].offset = read_uint64_le(file);
        dat_file->mft_data[i].size = read_uint32_le(file);
        dat_file->mft_data[i].compression_flag = read_uint16_le(file);
        dat_file->mft_data[i].entry_flag = read_uint16_le(file);
        dat_file->mft_data[i].counter = read_uint32_le(file);
        dat_file->mft_data[i].crc = read_uint32_le(file);
    }
    uint32_t mft_data_index = dat_file->mft_header.num_entries - 1;
    debug_print_mft_data(&dat_file->mft_data[mft_data_index], mft_data_index); // Print MFTData for each entry

    uint32_t num_index_entries = dat_file->mft_data[MFT_ENTRY_INDEX_NUM].size / sizeof(MFTIndexData);
    dat_file->mft_index_data = (MFTIndexData *)malloc(num_index_entries * sizeof(MFTIndexData));
    if (dat_file->mft_index_data == NULL)
    {
        fprintf(stderr, "Memory allocation failed for MFTIndexData\n");
        free(dat_file->mft_data); // Cleanup previous allocation
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fseeko(file, dat_file->mft_data[MFT_ENTRY_INDEX_NUM].offset, SEEK_SET);
    for (uint32_t i = 0; i < num_index_entries; ++i)
    {
        dat_file->mft_index_data[i].file_id = read_uint32_le(file);
        dat_file->mft_index_data[i].base_id = read_uint32_le(file);
    }

    uint32_t mft_index_data_num = num_index_entries - 2;
    debug_print_mft_index_data(&dat_file->mft_index_data[mft_index_data_num], mft_index_data_num); // Print MFTIndexData for each entry

    fclose(file);
}

uint8_t *extract_mft_data(const char *file_path, DatFile *dat_file, uint32_t number)
{
    size_t index_number = 0;
    bool found = false;

    // Find the corresponding MFT entry
    uint32_t num_index_entries = dat_file->mft_data[MFT_ENTRY_INDEX_NUM].size / sizeof(MFTIndexData);
    for (size_t i = 0; i < num_index_entries; i++)
    {
        if (dat_file->mft_index_data[i].file_id == number)
        {
            printf("Found!\n");
            printf("MFT Index Entry %zu:\n", i);
            printf("File ID: %u\n", dat_file->mft_index_data[i].file_id);
            printf("Base ID: %u\n", dat_file->mft_index_data[i].base_id);
            index_number = dat_file->mft_index_data[i].base_id;
            found = true;
            break;
        }

        if (dat_file->mft_index_data[i].base_id == number)
        {
            printf("Found!\n");
            printf("MFT Index Entry %zu:\n", i);
            printf("File ID: %u\n", dat_file->mft_index_data[i].file_id);
            printf("Base ID: %u\n", dat_file->mft_index_data[i].base_id);
            index_number = dat_file->mft_index_data[i].base_id;
            found = true;
            break;
        }
    }

    if (!found)
    {
        fprintf(stderr, "MFT entry not found!\n");
        exit(EXIT_FAILURE);
    }

    // Get the MFT data corresponding to the found index
    MFTData *mft_entry = &dat_file->mft_data[index_number];

    // Check if the file is compressed
    if (mft_entry->compression_flag != 0)
    {
        printf("File is compressed!\n");
    }

    // Allocate buffer for MFT data
    uint8_t *buffer = (uint8_t *)malloc(mft_entry->size);
    if (buffer == NULL)
    {
        fprintf(stderr, "Memory allocation failed for buffer\n");
        exit(EXIT_FAILURE);
    }

    // Open the file to read data
    FILE *file = fopen(file_path, "rb"); // Open the file again to read data
    if (!file)
    {
        perror("Error opening file");
        free(buffer);
        exit(EXIT_FAILURE);
    }

    fseeko(file, mft_entry->offset, SEEK_SET);
    fread(buffer, 1, mft_entry->size, file);
    fclose(file);

    // Print the first 16 bytes of the MFT data (Hex) before decompression
    printf("First 16 bytes of MFT data before decompression (Hex):\n");
    for (size_t i = 0; i < 16 && i < mft_entry->size; ++i)
    {
        printf("%02X ", buffer[i]);
    }
    printf("\n");

    // Print the first 16 bytes of the MFT data (ASCII) before decompression
    printf("First 16 bytes of MFT data before decompression (ASCII):\n");
    for (size_t i = 0; i < 16 && i < mft_entry->size; ++i)
    {
        if (isprint(buffer[i]))
        {
            printf("%c", buffer[i]);
        }
        else
        {
            printf(".");
        }
    }
    printf("\n");

    // Decompress if the file is compressed
    if (mft_entry->compression_flag != 0)
    {
        uint32_t decompressedSize = 0;
        uint8_t *decompressedBuffer = inflateBuffer(mft_entry->size, (const uint32_t *)buffer, &decompressedSize, NULL);

        if (decompressedBuffer == NULL)
        {
            fprintf(stderr, "Decompression failed!\n");
            free(buffer); // Free the original buffer
            exit(EXIT_FAILURE);
        }

        free(buffer);                // Free the compressed buffer
        buffer = decompressedBuffer; // Update buffer to point to decompressed data
        printf("Decompressed MFT data size: %u bytes\n", decompressedSize);

        // Print the first 16 bytes of the MFT data (Hex) after decompression
        printf("First 16 bytes of MFT data after decompression (Hex):\n");
        for (size_t i = 0; i < 16 && i < decompressedSize; ++i)
        {
            printf("%02X ", buffer[i]);
        }
        printf("\n");

        // Print the first 16 bytes of the MFT data (ASCII) after decompression
        printf("First 16 bytes of MFT data after decompression (ASCII):\n");
        for (size_t i = 0; i < 16 && i < decompressedSize; ++i)
        {
            if (isprint(buffer[i]))
            {
                printf("%c", buffer[i]);
            }
            else
            {
                printf(".");
            }
        }
        printf("\n");
    }

    return buffer; // Return the buffer containing the MFT data
}

#endif // DATFILE_H
