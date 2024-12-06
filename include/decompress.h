#ifndef DECOMPRESS_H
#define DECOMPRESS_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

// Constants
#define MAX_SYMBOL_VALUE 285
#define MAX_CODE_BITS_LENGTH 32

// Structure for Huffman Tree
typedef struct
{
    uint16_t symbolValueTab[MAX_SYMBOL_VALUE];          // Symbol values
    uint32_t codeCompTab[MAX_SYMBOL_VALUE];             // Compressed codes
    uint8_t codeBitsTab[MAX_SYMBOL_VALUE];              // Lengths of codes
    uint16_t symbolValueTabOffsetTab[MAX_SYMBOL_VALUE]; // Offsets in the symbol value table
} HuffmanTree;

// State structure for managing decompression
typedef struct
{
    const uint32_t *input; // Input data (compressed)
    uint32_t inputSize;    // Size of the input data
    uint32_t inputPos;     // Current position in the input
    uint32_t head;         // Head for reading bits
    uint32_t bits;         // Bits read from input
    uint32_t buffer;       // Buffer for storing bits
    bool isEmpty;          // Flag to check if input is empty
} State;

// Global variable to track if the Huffman tree is initialized
static int HuffmanTreeDictInitialized = 0;

// Global variable to hold the Huffman tree
HuffmanTree HuffmanTreeDict; // Assume this is a defined structure for your Huffman tree

// Function to pull a byte
void pullByte(State *ioState)
{
    // Checking that we have less than 32 bits available
    if (ioState->bits >= 32)
    {
        fprintf(stderr, "Tried to pull a value while we still have 32 bits available.\n");
        exit(EXIT_FAILURE);
    }

    // Skip the last element of all 65536 bytes blocks
    if ((ioState->inputPos + 1) % 0x4000 == 0)
    {
        ++(ioState->inputPos);
    }

    // Checking that inputPos is not out of bounds
    if (ioState->inputPos >= ioState->inputSize)
    {
        fprintf(stderr, "Reached end of input while trying to fetch a new byte.\n");
        exit(EXIT_FAILURE);
    }

    // Fetching the next value
    uint32_t aValue = ioState->input[ioState->inputPos];

    // Pulling the data into head/buffer given that we need to keep the relevant bits
    if (ioState->bits == 0)
    {
        ioState->head = aValue;
        ioState->buffer = 0;
    }
    else
    {
        ioState->head = ioState->head | (aValue >> (ioState->bits));
        ioState->buffer = (aValue << (32 - ioState->bits));
    }

    // Updating state variables
    ioState->bits += 32;
    ++(ioState->inputPos);
}

// Function to ensure we have enough bits
void needBits(State *ioState, const uint8_t iBits)
{
    // Checking that we request at most 32 bits
    if (iBits > 32)
    {
        fprintf(stderr, "Tried to need more than 32 bits.\n");
        exit(EXIT_FAILURE);
    }

    if (ioState->bits < iBits)
    {
        pullByte(ioState);
    }
}

// Function to drop bits
void dropBits(State *ioState, const uint8_t iBits)
{
    // Checking that we request at most 32 bits
    if (iBits > 32)
    {
        fprintf(stderr, "Tried to drop more than 32 bits.\n");
        exit(EXIT_FAILURE);
    }

    if (iBits > ioState->bits)
    {
        fprintf(stderr, "Tried to drop more bits than we have.\n");
        exit(EXIT_FAILURE);
    }

    // Updating the values to drop the bits
    if (iBits == 32)
    {
        ioState->head = ioState->buffer;
        ioState->buffer = 0;
    }
    else
    {
        ioState->head = (ioState->head << iBits) | (ioState->buffer >> (32 - iBits));
        ioState->buffer = ioState->buffer << iBits;
    }

    // Update state info
    ioState->bits -= iBits;
}

// Function to read bits
uint32_t readBits(const State *iState, const uint8_t iBits)
{
    return (iState->head >> (32 - iBits));
}

// Function to read a code from the Huffman tree
void readCode(HuffmanTree *iHuffmanTree, State *ioState, uint16_t *ioCode)
{
    if (iHuffmanTree->codeCompTab[0] == 0)
    {
        fprintf(stderr, "Trying to read code from an empty HuffmanTree.\n");
        exit(EXIT_FAILURE);
    }

    needBits(ioState, 32);
    uint16_t anIndex = 0;
    uint32_t bitsRead = readBits(ioState, 32);

    while (bitsRead < iHuffmanTree->codeCompTab[anIndex])
    {
        ++anIndex;
    }

    uint8_t aNbBits = iHuffmanTree->codeBitsTab[anIndex];

    *ioCode = iHuffmanTree->symbolValueTab[iHuffmanTree->symbolValueTabOffsetTab[anIndex] -
                                           ((bitsRead - iHuffmanTree->codeCompTab[anIndex]) >> (32 - aNbBits))];

    dropBits(ioState, aNbBits);
}

// Function prototype for building the Huffman tree
void buildHuffmanTree(HuffmanTree *ioHuffmanTree, int16_t *ioWorkingBitTab, int16_t *ioWorkingCodeTab)
{
    // Building the HuffmanTree
    uint32_t aCode = 0;
    uint8_t aNbBits = 0;
    uint16_t aCodeCompTabIndex = 0;
    uint16_t aSymbolOffset = 0;

    while (aNbBits < MAX_CODE_BITS_LENGTH)
    {
        if (ioWorkingBitTab[aNbBits] != -1)
        {
            int16_t aCurrentSymbol = ioWorkingBitTab[aNbBits];
            while (aCurrentSymbol != -1)
            {
                // Registering the code
                ioHuffmanTree->symbolValueTab[aSymbolOffset] = aCurrentSymbol;
                ++aSymbolOffset;
                aCurrentSymbol = ioWorkingCodeTab[aCurrentSymbol];
                --aCode; // Decrement code value for next symbol
            }

            // Minimum code value for aNbBits bits
            ioHuffmanTree->codeCompTab[aCodeCompTabIndex] = ((aCode + 1) << (32 - aNbBits));

            // Number of bits for l_codeCompIndex index
            ioHuffmanTree->codeBitsTab[aCodeCompTabIndex] = aNbBits;

            // Offset in symbolValueTab table to reach the value
            ioHuffmanTree->symbolValueTabOffsetTab[aCodeCompTabIndex] = aSymbolOffset - 1;

            ++aCodeCompTabIndex;
        }
        aCode = (aCode << 1) + 1; // Increment code for next length
        ++aNbBits;
    }
}

void fillTabsHelper(const uint8_t iBits, const int16_t iSymbol, int16_t *ioWorkingBitTab, int16_t *ioWorkingCodeTab)
{
    // Check out of bounds
    if (iBits >= MAX_CODE_BITS_LENGTH)
    {
        fprintf(stderr, "Error: Too many bits.\n");
        exit(EXIT_FAILURE); // Exit or handle the error appropriately
    }

    if (iSymbol >= MAX_SYMBOL_VALUE)
    {
        fprintf(stderr, "Error: Too high symbol.\n");
        exit(EXIT_FAILURE); // Exit or handle the error appropriately
    }

    if (ioWorkingBitTab[iBits] == -1)
    {
        ioWorkingBitTab[iBits] = iSymbol;
    }
    else
    {
        ioWorkingCodeTab[iSymbol] = ioWorkingBitTab[iBits];
        ioWorkingBitTab[iBits] = iSymbol;
    }
}

void initializeHuffmanTreeDict()
{
    int16_t aWorkingBitTab[MAX_CODE_BITS_LENGTH];
    int16_t aWorkingCodeTab[MAX_SYMBOL_VALUE];

    // Initialize working tables
    memset(aWorkingBitTab, -1, MAX_CODE_BITS_LENGTH * sizeof(int16_t)); // Use -1 to indicate uninitialized
    memset(aWorkingCodeTab, -1, MAX_SYMBOL_VALUE * sizeof(int16_t));    // Use -1 to indicate uninitialized
    // clang-format off
    // Define your bits and symbols arrays
    uint8_t ibits[] = {
        3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8,
        8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
        10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 13,
        13, 13, 13, 13, 13, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
        16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    }; // Example values, adjust as needed
    int16_t isymbols[] = {
        0x0A, 0x09, 0x08, 0x0C, 0x0B, 0x07, 0x00, 0xE0, 0x2A, 0x29, 0x06, 0x4A, 0x40, 0x2C, 0x2B,
        0x28, 0x20, 0x05, 0x04, 0x49, 0x48, 0x27, 0x26, 0x25, 0x0D, 0x03, 0x6A, 0x69, 0x4C, 0x4B,
        0x47, 0x24, 0xE8, 0xA0, 0x89, 0x88, 0x68, 0x67, 0x63, 0x60, 0x46, 0x23, 0xE9, 0xC9, 0xC0,
        0xA9, 0xA8, 0x8A, 0x87, 0x80, 0x66, 0x65, 0x45, 0x44, 0x43, 0x2D, 0x02, 0x01, 0xE5, 0xC8,
        0xAA, 0xA5, 0xA4, 0x8B, 0x85, 0x84, 0x6C, 0x6B, 0x64, 0x4D, 0x0E, 0xE7, 0xCA, 0xC7, 0xA7,
        0xA6, 0x86, 0x83, 0xE6, 0xE4, 0xC4, 0x8C, 0x2E, 0x22, 0xEC, 0xC6, 0x6D, 0x4E, 0xEA, 0xCC,
        0xAC, 0xAB, 0x8D, 0x11, 0x10, 0x0F, 0xFF, 0xFE, 0xFD, 0xFC, 0xFB, 0xFA, 0xF9, 0xF8, 0xF7,
        0xF6, 0xF5, 0xF4, 0xF3, 0xF2, 0xF1, 0xF0, 0xEF, 0xEE, 0xED, 0xEB, 0xE3, 0xE2, 0xE1, 0xDF,
        0xDE, 0xDD, 0xDC, 0xDB, 0xDA, 0xD9, 0xD8, 0xD7, 0xD6, 0xD5, 0xD4, 0xD3, 0xD2, 0xD1, 0xD0,
        0xCF, 0xCE, 0xCD, 0xCB, 0xC5, 0xC3, 0xC2, 0xC1, 0xBF, 0xBE, 0xBD, 0xBC, 0xBB, 0xBA, 0xB9,
        0xB8, 0xB7, 0xB6, 0xB5, 0xB4, 0xB3, 0xB2, 0xB1, 0xB0, 0xAF, 0xAE, 0xAD, 0xA3, 0xA2, 0xA1,
        0x9F, 0x9E, 0x9D, 0x9C, 0x9B, 0x9A, 0x99, 0x98, 0x97, 0x96, 0x95, 0x94, 0x93, 0x92, 0x91,
        0x90, 0x8F, 0x8E, 0x82, 0x81, 0x7F, 0x7E, 0x7D, 0x7C, 0x7B, 0x7A, 0x79, 0x78, 0x77, 0x76,
        0x75, 0x74, 0x73, 0x72, 0x71, 0x70, 0x6F, 0x6E, 0x62, 0x61, 0x5F, 0x5E, 0x5D, 0x5C, 0x5B,
        0x5A, 0x59, 0x58, 0x57, 0x56, 0x55, 0x54, 0x53, 0x52, 0x51, 0x50, 0x4F, 0x42, 0x41, 0x3F,
        0x3E, 0x3D, 0x3C, 0x3B, 0x3A, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34, 0x33, 0x32, 0x31, 0x30,
        0x2F, 0x21, 0x1F, 0x1E, 0x1D, 0x1C, 0x1B, 0x1A, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13,
        0x12,
    }; // Example values, adjust as needed
    // clang-format on

    size_t symbolCount = sizeof(ibits) / sizeof(ibits[0]); // Calculate number of symbols

    // Populate the working tables
    for (size_t i = 0; i < symbolCount; i++)
    {
        fillTabsHelper(ibits[i], isymbols[i], aWorkingBitTab, aWorkingCodeTab);
    }

    // Build the Huffman tree
    buildHuffmanTree(&HuffmanTreeDict, aWorkingBitTab, aWorkingCodeTab);
}

// Function to parse the Huffman tree
void parseHuffmanTree(State *ioState, HuffmanTree *ioHuffmanTree)
{
    // Reading the number of symbols to read
    needBits(ioState, 16);
    uint16_t aNumberOfSymbols = (uint16_t)readBits(ioState, 16); // C-style cast
    dropBits(ioState, 16);

    if (aNumberOfSymbols > MAX_SYMBOL_VALUE)
    {
        fprintf(stderr, "Too many symbols to decode.\n");
        exit(EXIT_FAILURE);
    }

    int16_t aWorkingBitTab[MAX_CODE_BITS_LENGTH];
    int16_t aWorkingCodeTab[MAX_SYMBOL_VALUE];

    // Initialize our workingTabs
    memset(aWorkingBitTab, 0xFF, MAX_CODE_BITS_LENGTH * sizeof(int16_t));
    memset(aWorkingCodeTab, 0xFF, MAX_SYMBOL_VALUE * sizeof(int16_t));

    int16_t aRemainingSymbols = aNumberOfSymbols - 1;

    // Fetching the code repartition
    while (aRemainingSymbols >= 0)
    {
        uint16_t aCode = 0;
        readCode(&HuffmanTreeDict, ioState, &aCode);

        uint16_t aCodeNumberOfBits = aCode & 0x1F;
        uint16_t aCodeNumberOfSymbols = (aCode >> 5) + 1;

        if (aCodeNumberOfBits == 0)
        {
            aRemainingSymbols -= aCodeNumberOfSymbols;
        }
        else
        {
            while (aCodeNumberOfSymbols > 0)
            {
                if (aWorkingBitTab[aCodeNumberOfBits] == -1)
                {
                    aWorkingBitTab[aCodeNumberOfBits] = aRemainingSymbols;
                }
                else
                {
                    aWorkingCodeTab[aRemainingSymbols] = aWorkingBitTab[aCodeNumberOfBits];
                    aWorkingBitTab[aCodeNumberOfBits] = aRemainingSymbols;
                }
                --aRemainingSymbols;
                --aCodeNumberOfSymbols;
            }
        }
    }

    // Effectively build the Huffman tree
    buildHuffmanTree(ioHuffmanTree, aWorkingBitTab, aWorkingCodeTab);
}

// Function to inflate data
void inflate_data(State *ioState, uint8_t *ioOutputTab, const uint32_t iOutputSize)
{
    uint32_t anOutputPos = 0;

    // Reading the const write size addition value
    needBits(ioState, 8);
    dropBits(ioState, 4);
    uint16_t aWriteSizeConstAdd = readBits(ioState, 4) + 1;
    dropBits(ioState, 4);

    // Declaring our HuffmanTrees
    HuffmanTree aHuffmanTreeSymbol;
    HuffmanTree aHuffmanTreeCopy;

    while (anOutputPos < iOutputSize)
    {
        // Resetting Huffman trees
        memset(&aHuffmanTreeSymbol, 0, sizeof(HuffmanTree));
        memset(&aHuffmanTreeCopy, 0, sizeof(HuffmanTree));

        // Reading HuffmanTrees
        parseHuffmanTree(ioState, &aHuffmanTreeSymbol);
        parseHuffmanTree(ioState, &aHuffmanTreeCopy);

        // Reading MaxCount
        needBits(ioState, 4);
        uint32_t aMaxCount = (readBits(ioState, 4) + 1) << 12;
        dropBits(ioState, 4);

        uint32_t aCurrentCodeReadCount = 0;

        while ((aCurrentCodeReadCount < aMaxCount) && (anOutputPos < iOutputSize))
        {
            ++aCurrentCodeReadCount;

            // Reading next code
            uint16_t aCode = 0;
            readCode(&aHuffmanTreeSymbol, ioState, &aCode);

            if (aCode < 0x100)
            {
                ioOutputTab[anOutputPos] = (uint8_t)aCode; // C-style cast
                ++anOutputPos;
                continue;
            }

            // We are in copy mode!
            // Reading the additional info to know the write size
            aCode -= 0x100;

            // Write size
            div_t aCodeDiv4 = div(aCode, 4);

            uint32_t aWriteSize = 0;
            if (aCodeDiv4.quot == 0)
            {
                aWriteSize = aCode;
            }
            else if (aCodeDiv4.quot < 7)
            {
                aWriteSize = ((1 << (aCodeDiv4.quot - 1)) * (4 + aCodeDiv4.rem));
            }
            else if (aCode == 28)
            {
                aWriteSize = 0xFF;
            }
            else
            {
                fprintf(stderr, "Invalid value for writeSize code.\n");
                exit(EXIT_FAILURE);
            }

            // Additional bits
            if (aCodeDiv4.quot > 1 && aCode != 28)
            {
                uint8_t aWriteSizeAddBits = aCodeDiv4.quot - 1;
                needBits(ioState, aWriteSizeAddBits);
                aWriteSize |= readBits(ioState, aWriteSizeAddBits);
                dropBits(ioState, aWriteSizeAddBits);
            }
            aWriteSize += aWriteSizeConstAdd;

            // Write offset
            // Reading the write offset
            readCode(&aHuffmanTreeCopy, ioState, &aCode);

            div_t aCodeDiv2 = div(aCode, 2);

            uint32_t aWriteOffset = 0;
            if (aCodeDiv2.quot == 0)
            {
                aWriteOffset = aCode;
            }
            else if (aCodeDiv2.quot < 17)
            {
                aWriteOffset = ((1 << (aCodeDiv2.quot - 1)) * (2 + aCodeDiv2.rem));
            }
            else
            {
                fprintf(stderr, "Invalid value for writeOffset code.\n");
                exit(EXIT_FAILURE);
            }

            // Additional bits
            if (aCodeDiv2.quot > 1)
            {
                uint8_t aWriteOffsetAddBits = aCodeDiv2.quot - 1;
                needBits(ioState, aWriteOffsetAddBits);
                aWriteOffset |= readBits(ioState, aWriteOffsetAddBits);
                dropBits(ioState, aWriteOffsetAddBits);
            }
            aWriteOffset += 1;

            uint32_t anAlreadyWritten = 0;
            while ((anAlreadyWritten < aWriteSize) && (anOutputPos < iOutputSize))
            {
                ioOutputTab[anOutputPos] = ioOutputTab[anOutputPos - aWriteOffset];
                ++anOutputPos;
                ++anAlreadyWritten;
            }
        }
    }
}

uint8_t *inflateBuffer(uint32_t iInputSize, const uint32_t *iInputTab, uint32_t *ioOutputSize, uint8_t *ioOutputTab)
{
    if (iInputTab == NULL)
    {
        fprintf(stderr, "Error: Input buffer is null.\n");
        return NULL;
    }

    if (!HuffmanTreeDictInitialized)
    {
        initializeHuffmanTreeDict();
        HuffmanTreeDictInitialized = 1;
    }

    // Initialize state
    State aState;
    aState.input = iInputTab;
    aState.inputSize = iInputSize;
    aState.inputPos = 0;
    aState.head = 0;
    aState.bits = 0;
    aState.buffer = 0;
    aState.isEmpty = 0;

    // Skipping header & getting size of the uncompressed data
    needBits(&aState, 32);
    dropBits(&aState, 32);

    // Getting size of the uncompressed data
    needBits(&aState, 32);
    uint32_t anOutputSize = readBits(&aState, 32);
    dropBits(&aState, 32);

    if (*ioOutputSize != 0)
    {
        // We do not take max here as we won't be able to have more than the output available
        if (anOutputSize > *ioOutputSize)
        {
            anOutputSize = *ioOutputSize;
        }
    }

    *ioOutputSize = anOutputSize;

    // Allocate memory for output buffer
    ioOutputTab = (uint8_t *)malloc(sizeof(uint8_t) * anOutputSize);
    if (ioOutputTab == NULL)
    {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        *ioOutputSize = 0;
        return NULL;
    }

    // Inflate data
    inflate_data(&aState, ioOutputTab, anOutputSize);

    return ioOutputTab;
}

#endif // DECOMPRESS_H
