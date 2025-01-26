#include "wacko.h"

int main()
{
    DatFile dat_file;
    // Initialize dat_file (optionally, you can set it to default values)
    memset(&dat_file, 0, sizeof(DatFile));

    // Load the DAT file
    const char *file_path = "Local.dat"; // Change to your actual path if needed
    // const char *file_path = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Guild Wars 2\\Gw2.dat"; // Change to your actual path if needed
    load_dat_file(file_path, &dat_file);

    // Print out the parsed data for debugging purposes
    printf("\nDAT File Version: %d\n", dat_file.header.version);
    printf("MFT Header Identifier: %.4s\n", dat_file.mft_header.identifier);
    printf("Number of MFT Entries: %u\n\n", dat_file.mft_header.num_entries);

    // Example: Extract MFT data based on file ID or base ID
    uint32_t search_id = 16; // Change this to the ID you want to search for

    uint8_t *mft_data = extract_mft_data(file_path, &dat_file, search_id);
    if (mft_data)
    {
        // Successfully extracted data, use it as needed
        // For example, do something with mft_data here
        free(mft_data); // Free after use
    }

    // Clean up allocated memory
    free(dat_file.mft_data);
    free(dat_file.mft_index_data);

    return 0;
}
