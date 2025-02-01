#ifndef DECOMPRESS_H
#define DECOMPRESS_H

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_BITS_HASH 8
#define MAX_SYMBOL_VALUE 285
#define MAX_CODE_BITS_LENGTH 32

typedef struct
{
	uint8_t* input_buffer;
	uint64_t buffer_position_bytes;
	uint32_t bytes_available;
	uint32_t head_data;
	uint32_t buffer_data;
	uint8_t bits_available_data;
} StateData;

void pull_byte(StateData* state_data, uint32_t* head_data, uint8_t* bits_available_data)
{
	if (state_data->bytes_available >= sizeof(uint32_t))
	{
		// Copy 4 bytes from the input buffer into head_data
		memcpy(head_data, state_data->input_buffer + state_data->buffer_position_bytes, sizeof(uint32_t));

		// Update state_data properties
		state_data->bytes_available -= sizeof(uint32_t);
		state_data->buffer_position_bytes += sizeof(uint32_t);

		// Set the number of bits available
		*bits_available_data = sizeof(uint32_t) * 8; // 32 bits
	}
	else
	{
		// Not enough bytes available, reset values
		*head_data = 0;
		*bits_available_data = 0;
	}
}

uint32_t read_bits(StateData* state_data, uint8_t bits_number)
{
	uint32_t value = state_data->head_data >> ((sizeof(uint32_t) * 8) - bits_number);

	return value;
}

void drop_bits(StateData* state_data, uint8_t bits_number)
{
	if (state_data->bits_available_data < bits_number)
	{

		printf("Too much bits were asked to be dropped.\n");
	}
	uint8_t new_bits_available = 0;
	new_bits_available = state_data->bits_available_data - bits_number;
	if (new_bits_available >= (sizeof(uint32_t) * 8))
	{
		if (bits_number == (sizeof(uint32_t) * 8))
		{
			state_data->head_data = state_data->buffer_data;
			state_data->buffer_data = 0;
		}
		else
		{
			state_data->head_data = (state_data->head_data << bits_number) | (state_data->buffer_data >> ((sizeof(uint32_t) * 8) - bits_number));
			state_data->buffer_data = state_data->buffer_data << bits_number;
		}
		state_data->bits_available_data = new_bits_available;
	}
	else
	{
		uint32_t new_value = 0;
		uint8_t pulled_bits = 0;
		pull_byte(state_data, &new_value, &pulled_bits);

		if (bits_number == (sizeof(uint32_t) * 8))
		{
			state_data->head_data = 0;
		}
		else
		{
			state_data->head_data = state_data->head_data << bits_number;
		}
		state_data->head_data |= (state_data->buffer_data >> ((sizeof(uint32_t) * 8) - bits_number)) | (new_value >> new_bits_available);

		if (new_bits_available > 0)
		{
			state_data->buffer_data = new_value << ((sizeof(uint32_t) * 8) - new_bits_available);
		}

		state_data->bits_available_data = new_bits_available + pulled_bits;
	}
}

typedef struct
{
	uint32_t code_comparison_array[MAX_CODE_BITS_LENGTH];
	uint16_t symbol_value_array_offset_array[MAX_CODE_BITS_LENGTH];
	uint16_t symbol_value_array[MAX_SYMBOL_VALUE];
	uint8_t code_bits_array[MAX_CODE_BITS_LENGTH];

	bool symbol_value_hash_existence_array[1 << MAX_BITS_HASH];
	uint16_t symbol_value_hash_array[1 << MAX_BITS_HASH];
	uint8_t code_bits_hash_array[1 << MAX_BITS_HASH];

} HuffmanTree;

typedef struct
{
	bool symbol_list_by_bits_head_existence_array[MAX_CODE_BITS_LENGTH];
	uint16_t symbol_list_by_bits_head_array[MAX_CODE_BITS_LENGTH];

	bool symbol_list_by_bits_body_existence_array[MAX_SYMBOL_VALUE];
	uint16_t symbol_list_by_bits_body_array[MAX_SYMBOL_VALUE];
} HuffmanTreeBuilder;

void read_code(HuffmanTree* huffmantree_data, StateData* state_data, uint16_t* symbol_data)
{
	uint32_t hash_value = 0;
	hash_value = read_bits(state_data, 8);
	bool exist = huffmantree_data->symbol_value_hash_existence_array[hash_value];
	if (exist)
	{
		*symbol_data = huffmantree_data->symbol_value_hash_array[hash_value];
		uint8_t code_bits_hash = huffmantree_data->code_bits_hash_array[hash_value];
		drop_bits(state_data, code_bits_hash);
	}
	else
	{
		uint16_t index_data = 0;
		while (read_bits(state_data, 32) < huffmantree_data->code_comparison_array[index_data])
		{
			++index_data;
		}
		uint8_t temp_bits = huffmantree_data->code_bits_array[index_data];
		*symbol_data = huffmantree_data->symbol_value_array[huffmantree_data->symbol_value_array_offset_array[index_data] - ((read_bits(state_data, 32) - huffmantree_data->code_comparison_array[index_data]) >> (32 - temp_bits))];
		drop_bits(state_data, temp_bits);
	}
}

void clear_huffmantree(HuffmanTree* huffmantree)
{
	// Clear code_comparison_array and symbol_value_array_offset_array
	for (int i = 0; i < MAX_CODE_BITS_LENGTH; i++)
	{
		huffmantree->code_comparison_array[i] = 0;
		huffmantree->symbol_value_array_offset_array[i] = 0;
		huffmantree->code_bits_array[i] = 0;
	}

	// Clear symbol_value_array
	for (int i = 0; i < MAX_SYMBOL_VALUE; i++)
	{
		huffmantree->symbol_value_array[i] = 0;
	}

	// Clear symbol_value_hash_existence_array and other hash arrays
	for (int i = 0; i < (1 << MAX_BITS_HASH); i++)
	{
		huffmantree->symbol_value_hash_existence_array[i] = false;
		huffmantree->symbol_value_hash_array[i] = 0;
		huffmantree->code_bits_hash_array[i] = 0;
	}
}

void clear_huffmantree_builder(HuffmanTreeBuilder* huffmantree_builder)
{
	for (int i = 0; i < MAX_CODE_BITS_LENGTH; i++)
	{
		huffmantree_builder->symbol_list_by_bits_head_existence_array[i] = false;
		huffmantree_builder->symbol_list_by_bits_head_array[i] = 0;
	}

	for (int i = 0; i < MAX_SYMBOL_VALUE; i++)
	{
		huffmantree_builder->symbol_list_by_bits_body_existence_array[i] = false;
		huffmantree_builder->symbol_list_by_bits_body_array[i] = 0;
	}
}

void add_symbol(HuffmanTreeBuilder* huffmantree_builder, uint16_t symbol_data, uint8_t bits_data)
{
	if (huffmantree_builder->symbol_list_by_bits_head_existence_array[bits_data])
	{
		huffmantree_builder->symbol_list_by_bits_body_array[symbol_data] = huffmantree_builder->symbol_list_by_bits_head_array[bits_data];
		huffmantree_builder->symbol_list_by_bits_body_existence_array[symbol_data] = true;
		huffmantree_builder->symbol_list_by_bits_head_array[bits_data] = symbol_data;
	}
	else
	{
		huffmantree_builder->symbol_list_by_bits_head_array[bits_data] = symbol_data;
		huffmantree_builder->symbol_list_by_bits_head_existence_array[bits_data] = true;
	}
}

bool check_huffmantree_builder(HuffmanTreeBuilder* huffmantree_builder)
{
	// Check if all elements in symbol_list_by_bits_head_existence_array are false
	for (int i = 0; i < MAX_CODE_BITS_LENGTH; i++)
	{
		if (huffmantree_builder->symbol_list_by_bits_head_existence_array[i] == true)
		{
			return false; // Return false if any element is true
		}
	}

	return true; // Return true if all checks passed (i.e., all elements are false or 0)
}

bool build_huffmantree(HuffmanTree* huffmantree_data, HuffmanTreeBuilder* huffmantree_builder)
{
	if (check_huffmantree_builder(huffmantree_builder))
	{
		return false; // Return false if the builder is in an invalid state
	}

	clear_huffmantree(huffmantree_data); // Clear existing Huffman tree data

	uint32_t code_data = 0;
	uint8_t bits_data = 0;

	// Loop through bits_data to build the tree
	while (bits_data <= MAX_BITS_HASH)
	{
		bool existence = huffmantree_builder->symbol_list_by_bits_head_existence_array[bits_data];

		if (existence)
		{
			uint16_t current_symbol = huffmantree_builder->symbol_list_by_bits_head_array[bits_data];

			while (existence)
			{
				uint16_t hash_value = (uint16_t)(code_data << (MAX_BITS_HASH - bits_data));
				uint16_t next_hash_value = (uint16_t)((code_data + 1) << (MAX_BITS_HASH - bits_data));

				// Update hash values
				while (hash_value < next_hash_value)
				{
					huffmantree_data->symbol_value_hash_existence_array[hash_value] = true;
					huffmantree_data->symbol_value_hash_array[hash_value] = current_symbol;
					huffmantree_data->code_bits_hash_array[hash_value] = bits_data;
					++hash_value;
				}

				// Move to the next symbol in the body array
				existence = huffmantree_builder->symbol_list_by_bits_body_existence_array[current_symbol];
				current_symbol = huffmantree_builder->symbol_list_by_bits_body_array[current_symbol];
				--code_data;
			}
		}

		// Shift code_data and increment bits_data
		code_data = (code_data << 1) + 1;
		++bits_data;
	}

	uint16_t code_comparison_array_index = 0;
	uint16_t symbol_offset = 0;

	// Continue building the tree for larger bit sizes
	while (bits_data < MAX_CODE_BITS_LENGTH)
	{
		bool existence = huffmantree_builder->symbol_list_by_bits_head_existence_array[bits_data];
		if (existence)
		{
			uint16_t current_symbol = huffmantree_builder->symbol_list_by_bits_head_array[bits_data];

			while (existence)
			{
				// Store the symbol in the symbol value array
				huffmantree_data->symbol_value_array[symbol_offset] = current_symbol;
				++symbol_offset;

				// Move to the next symbol in the body array
				existence = huffmantree_builder->symbol_list_by_bits_body_existence_array[current_symbol];
				current_symbol = huffmantree_builder->symbol_list_by_bits_body_array[current_symbol];
				--code_data;
			}

			// Update the comparison array with the code data
			huffmantree_data->code_comparison_array[code_comparison_array_index] = ((code_data + 1) << (32 - bits_data));
			huffmantree_data->code_bits_array[code_comparison_array_index] = bits_data;
			huffmantree_data->symbol_value_array_offset_array[code_comparison_array_index] = symbol_offset - 1;
			++code_comparison_array_index;
		}

		// Shift code_data and increment bits_data
		code_data = (code_data << 1) + 1;
		++bits_data;
	}

	return true; // Return true if the Huffman tree was successfully built
}

bool parse_huffmantree(StateData* state_data, HuffmanTree* huffmantree_data, HuffmanTreeBuilder* huffmantree_builder, HuffmanTree* huffmantree_static)
{
	uint16_t number_of_symbols = 0;
	number_of_symbols = (uint16_t)read_bits(state_data, 16); // Read number of symbols
	drop_bits(state_data, 16);                               // Drop the 16 bits read for the number of symbols

	if (number_of_symbols > MAX_SYMBOL_VALUE)
	{

		printf("Too many symbols to decode.\n");
		return false; // Return false if there are too many symbols
	}

	clear_huffmantree_builder(huffmantree_builder); // Clear the builder

	int16_t remaining_symbols = number_of_symbols - 1; // Initialize remaining symbols to number_of_symbols - 1

	while (remaining_symbols >= 0)
	{
		uint16_t code_data = 0;
		read_code(huffmantree_static, state_data, &code_data); // Read the Huffman code

		uint8_t code_number_of_bits = code_data & 0x1F;         // Extract number of bits
		uint16_t code_number_of_symbols = (code_data >> 5) + 1; // Extract number of symbols

		if (code_number_of_bits == 0)
		{
			remaining_symbols -= code_number_of_symbols; // If code_number_of_bits is 0, decrement remaining_symbols
		}
		else
		{
			while (code_number_of_symbols > 0)
			{
				// Add symbol before decrementing remaining_symbols
				add_symbol(huffmantree_builder, remaining_symbols, code_number_of_bits);
				--remaining_symbols;      // Decrement remaining_symbols after adding symbol
				--code_number_of_symbols; // Decrement number of symbols to process
			}
		}
	}

	// After processing all symbols, build the Huffman tree
	return build_huffmantree(huffmantree_data, huffmantree_builder);
}

void initialize_static_huffmantree(HuffmanTree* huffmantree_static)
{
	HuffmanTreeBuilder huffmantree_builder;
	clear_huffmantree_builder(&huffmantree_builder);

	add_symbol(&huffmantree_builder, 0x0A, 3);
	add_symbol(&huffmantree_builder, 0x09, 3);
	add_symbol(&huffmantree_builder, 0x08, 3);

	add_symbol(&huffmantree_builder, 0x0C, 4);
	add_symbol(&huffmantree_builder, 0x0B, 4);
	add_symbol(&huffmantree_builder, 0x07, 4);
	add_symbol(&huffmantree_builder, 0x00, 4);

	add_symbol(&huffmantree_builder, 0xE0, 5);
	add_symbol(&huffmantree_builder, 0x2A, 5);
	add_symbol(&huffmantree_builder, 0x29, 5);
	add_symbol(&huffmantree_builder, 0x06, 5);

	add_symbol(&huffmantree_builder, 0x4A, 6);
	add_symbol(&huffmantree_builder, 0x40, 6);
	add_symbol(&huffmantree_builder, 0x2C, 6);
	add_symbol(&huffmantree_builder, 0x2B, 6);
	add_symbol(&huffmantree_builder, 0x28, 6);
	add_symbol(&huffmantree_builder, 0x20, 6);
	add_symbol(&huffmantree_builder, 0x05, 6);
	add_symbol(&huffmantree_builder, 0x04, 6);

	add_symbol(&huffmantree_builder, 0x49, 7);
	add_symbol(&huffmantree_builder, 0x48, 7);
	add_symbol(&huffmantree_builder, 0x27, 7);
	add_symbol(&huffmantree_builder, 0x26, 7);
	add_symbol(&huffmantree_builder, 0x25, 7);
	add_symbol(&huffmantree_builder, 0x0D, 7);
	add_symbol(&huffmantree_builder, 0x03, 7);

	add_symbol(&huffmantree_builder, 0x6A, 8);
	add_symbol(&huffmantree_builder, 0x69, 8);
	add_symbol(&huffmantree_builder, 0x4C, 8);
	add_symbol(&huffmantree_builder, 0x4B, 8);
	add_symbol(&huffmantree_builder, 0x47, 8);
	add_symbol(&huffmantree_builder, 0x24, 8);

	add_symbol(&huffmantree_builder, 0xE8, 9);
	add_symbol(&huffmantree_builder, 0xA0, 9);
	add_symbol(&huffmantree_builder, 0x89, 9);
	add_symbol(&huffmantree_builder, 0x88, 9);
	add_symbol(&huffmantree_builder, 0x68, 9);
	add_symbol(&huffmantree_builder, 0x67, 9);
	add_symbol(&huffmantree_builder, 0x63, 9);
	add_symbol(&huffmantree_builder, 0x60, 9);
	add_symbol(&huffmantree_builder, 0x46, 9);
	add_symbol(&huffmantree_builder, 0x23, 9);

	add_symbol(&huffmantree_builder, 0xE9, 10);
	add_symbol(&huffmantree_builder, 0xC9, 10);
	add_symbol(&huffmantree_builder, 0xC0, 10);
	add_symbol(&huffmantree_builder, 0xA9, 10);
	add_symbol(&huffmantree_builder, 0xA8, 10);
	add_symbol(&huffmantree_builder, 0x8A, 10);
	add_symbol(&huffmantree_builder, 0x87, 10);
	add_symbol(&huffmantree_builder, 0x80, 10);
	add_symbol(&huffmantree_builder, 0x66, 10);
	add_symbol(&huffmantree_builder, 0x65, 10);
	add_symbol(&huffmantree_builder, 0x45, 10);
	add_symbol(&huffmantree_builder, 0x44, 10);
	add_symbol(&huffmantree_builder, 0x43, 10);
	add_symbol(&huffmantree_builder, 0x2D, 10);
	add_symbol(&huffmantree_builder, 0x02, 10);
	add_symbol(&huffmantree_builder, 0x01, 10);

	add_symbol(&huffmantree_builder, 0xE5, 11);
	add_symbol(&huffmantree_builder, 0xC8, 11);
	add_symbol(&huffmantree_builder, 0xAA, 11);
	add_symbol(&huffmantree_builder, 0xA5, 11);
	add_symbol(&huffmantree_builder, 0xA4, 11);
	add_symbol(&huffmantree_builder, 0x8B, 11);
	add_symbol(&huffmantree_builder, 0x85, 11);
	add_symbol(&huffmantree_builder, 0x84, 11);
	add_symbol(&huffmantree_builder, 0x6C, 11);
	add_symbol(&huffmantree_builder, 0x6B, 11);
	add_symbol(&huffmantree_builder, 0x64, 11);
	add_symbol(&huffmantree_builder, 0x4D, 11);
	add_symbol(&huffmantree_builder, 0x0E, 11);

	add_symbol(&huffmantree_builder, 0xE7, 12);
	add_symbol(&huffmantree_builder, 0xCA, 12);
	add_symbol(&huffmantree_builder, 0xC7, 12);
	add_symbol(&huffmantree_builder, 0xA7, 12);
	add_symbol(&huffmantree_builder, 0xA6, 12);
	add_symbol(&huffmantree_builder, 0x86, 12);
	add_symbol(&huffmantree_builder, 0x83, 12);

	add_symbol(&huffmantree_builder, 0xE6, 13);
	add_symbol(&huffmantree_builder, 0xE4, 13);
	add_symbol(&huffmantree_builder, 0xC4, 13);
	add_symbol(&huffmantree_builder, 0x8C, 13);
	add_symbol(&huffmantree_builder, 0x2E, 13);
	add_symbol(&huffmantree_builder, 0x22, 13);

	add_symbol(&huffmantree_builder, 0xEC, 14);
	add_symbol(&huffmantree_builder, 0xC6, 14);
	add_symbol(&huffmantree_builder, 0x6D, 14);
	add_symbol(&huffmantree_builder, 0x4E, 14);

	add_symbol(&huffmantree_builder, 0xEA, 15);
	add_symbol(&huffmantree_builder, 0xCC, 15);
	add_symbol(&huffmantree_builder, 0xAC, 15);
	add_symbol(&huffmantree_builder, 0xAB, 15);
	add_symbol(&huffmantree_builder, 0x8D, 15);
	add_symbol(&huffmantree_builder, 0x11, 15);
	add_symbol(&huffmantree_builder, 0x10, 15);
	add_symbol(&huffmantree_builder, 0x0F, 15);

	add_symbol(&huffmantree_builder, 0xFF, 16);
	add_symbol(&huffmantree_builder, 0xFE, 16);
	add_symbol(&huffmantree_builder, 0xFD, 16);
	add_symbol(&huffmantree_builder, 0xFC, 16);
	add_symbol(&huffmantree_builder, 0xFB, 16);
	add_symbol(&huffmantree_builder, 0xFA, 16);
	add_symbol(&huffmantree_builder, 0xF9, 16);
	add_symbol(&huffmantree_builder, 0xF8, 16);
	add_symbol(&huffmantree_builder, 0xF7, 16);
	add_symbol(&huffmantree_builder, 0xF6, 16);
	add_symbol(&huffmantree_builder, 0xF5, 16);
	add_symbol(&huffmantree_builder, 0xF4, 16);
	add_symbol(&huffmantree_builder, 0xF3, 16);
	add_symbol(&huffmantree_builder, 0xF2, 16);
	add_symbol(&huffmantree_builder, 0xF1, 16);
	add_symbol(&huffmantree_builder, 0xF0, 16);
	add_symbol(&huffmantree_builder, 0xEF, 16);
	add_symbol(&huffmantree_builder, 0xEE, 16);
	add_symbol(&huffmantree_builder, 0xED, 16);
	add_symbol(&huffmantree_builder, 0xEB, 16);
	add_symbol(&huffmantree_builder, 0xE3, 16);
	add_symbol(&huffmantree_builder, 0xE2, 16);
	add_symbol(&huffmantree_builder, 0xE1, 16);
	add_symbol(&huffmantree_builder, 0xDF, 16);
	add_symbol(&huffmantree_builder, 0xDE, 16);
	add_symbol(&huffmantree_builder, 0xDD, 16);
	add_symbol(&huffmantree_builder, 0xDC, 16);
	add_symbol(&huffmantree_builder, 0xDB, 16);
	add_symbol(&huffmantree_builder, 0xDA, 16);
	add_symbol(&huffmantree_builder, 0xD9, 16);
	add_symbol(&huffmantree_builder, 0xD8, 16);
	add_symbol(&huffmantree_builder, 0xD7, 16);
	add_symbol(&huffmantree_builder, 0xD6, 16);
	add_symbol(&huffmantree_builder, 0xD5, 16);
	add_symbol(&huffmantree_builder, 0xD4, 16);
	add_symbol(&huffmantree_builder, 0xD3, 16);
	add_symbol(&huffmantree_builder, 0xD2, 16);
	add_symbol(&huffmantree_builder, 0xD1, 16);
	add_symbol(&huffmantree_builder, 0xD0, 16);
	add_symbol(&huffmantree_builder, 0xCF, 16);
	add_symbol(&huffmantree_builder, 0xCE, 16);
	add_symbol(&huffmantree_builder, 0xCD, 16);
	add_symbol(&huffmantree_builder, 0xCB, 16);
	add_symbol(&huffmantree_builder, 0xC5, 16);
	add_symbol(&huffmantree_builder, 0xC3, 16);
	add_symbol(&huffmantree_builder, 0xC2, 16);
	add_symbol(&huffmantree_builder, 0xC1, 16);
	add_symbol(&huffmantree_builder, 0xBF, 16);
	add_symbol(&huffmantree_builder, 0xBE, 16);
	add_symbol(&huffmantree_builder, 0xBD, 16);
	add_symbol(&huffmantree_builder, 0xBC, 16);
	add_symbol(&huffmantree_builder, 0xBB, 16);
	add_symbol(&huffmantree_builder, 0xBA, 16);
	add_symbol(&huffmantree_builder, 0xB9, 16);
	add_symbol(&huffmantree_builder, 0xB8, 16);
	add_symbol(&huffmantree_builder, 0xB7, 16);
	add_symbol(&huffmantree_builder, 0xB6, 16);
	add_symbol(&huffmantree_builder, 0xB5, 16);
	add_symbol(&huffmantree_builder, 0xB4, 16);
	add_symbol(&huffmantree_builder, 0xB3, 16);
	add_symbol(&huffmantree_builder, 0xB2, 16);
	add_symbol(&huffmantree_builder, 0xB1, 16);
	add_symbol(&huffmantree_builder, 0xB0, 16);
	add_symbol(&huffmantree_builder, 0xAF, 16);
	add_symbol(&huffmantree_builder, 0xAE, 16);
	add_symbol(&huffmantree_builder, 0xAD, 16);
	add_symbol(&huffmantree_builder, 0xA3, 16);
	add_symbol(&huffmantree_builder, 0xA2, 16);
	add_symbol(&huffmantree_builder, 0xA1, 16);
	add_symbol(&huffmantree_builder, 0x9F, 16);
	add_symbol(&huffmantree_builder, 0x9E, 16);
	add_symbol(&huffmantree_builder, 0x9D, 16);
	add_symbol(&huffmantree_builder, 0x9C, 16);
	add_symbol(&huffmantree_builder, 0x9B, 16);
	add_symbol(&huffmantree_builder, 0x9A, 16);
	add_symbol(&huffmantree_builder, 0x99, 16);
	add_symbol(&huffmantree_builder, 0x98, 16);
	add_symbol(&huffmantree_builder, 0x97, 16);
	add_symbol(&huffmantree_builder, 0x96, 16);
	add_symbol(&huffmantree_builder, 0x95, 16);
	add_symbol(&huffmantree_builder, 0x94, 16);
	add_symbol(&huffmantree_builder, 0x93, 16);
	add_symbol(&huffmantree_builder, 0x92, 16);
	add_symbol(&huffmantree_builder, 0x91, 16);
	add_symbol(&huffmantree_builder, 0x90, 16);
	add_symbol(&huffmantree_builder, 0x8F, 16);
	add_symbol(&huffmantree_builder, 0x8E, 16);
	add_symbol(&huffmantree_builder, 0x82, 16);
	add_symbol(&huffmantree_builder, 0x81, 16);
	add_symbol(&huffmantree_builder, 0x7F, 16);
	add_symbol(&huffmantree_builder, 0x7E, 16);
	add_symbol(&huffmantree_builder, 0x7D, 16);
	add_symbol(&huffmantree_builder, 0x7C, 16);
	add_symbol(&huffmantree_builder, 0x7B, 16);
	add_symbol(&huffmantree_builder, 0x7A, 16);
	add_symbol(&huffmantree_builder, 0x79, 16);
	add_symbol(&huffmantree_builder, 0x78, 16);
	add_symbol(&huffmantree_builder, 0x77, 16);
	add_symbol(&huffmantree_builder, 0x76, 16);
	add_symbol(&huffmantree_builder, 0x75, 16);
	add_symbol(&huffmantree_builder, 0x74, 16);
	add_symbol(&huffmantree_builder, 0x73, 16);
	add_symbol(&huffmantree_builder, 0x72, 16);
	add_symbol(&huffmantree_builder, 0x71, 16);
	add_symbol(&huffmantree_builder, 0x70, 16);
	add_symbol(&huffmantree_builder, 0x6F, 16);
	add_symbol(&huffmantree_builder, 0x6E, 16);
	add_symbol(&huffmantree_builder, 0x62, 16);
	add_symbol(&huffmantree_builder, 0x61, 16);
	add_symbol(&huffmantree_builder, 0x5F, 16);
	add_symbol(&huffmantree_builder, 0x5E, 16);
	add_symbol(&huffmantree_builder, 0x5D, 16);
	add_symbol(&huffmantree_builder, 0x5C, 16);
	add_symbol(&huffmantree_builder, 0x5B, 16);
	add_symbol(&huffmantree_builder, 0x5A, 16);
	add_symbol(&huffmantree_builder, 0x59, 16);
	add_symbol(&huffmantree_builder, 0x58, 16);
	add_symbol(&huffmantree_builder, 0x57, 16);
	add_symbol(&huffmantree_builder, 0x56, 16);
	add_symbol(&huffmantree_builder, 0x55, 16);
	add_symbol(&huffmantree_builder, 0x54, 16);
	add_symbol(&huffmantree_builder, 0x53, 16);
	add_symbol(&huffmantree_builder, 0x52, 16);
	add_symbol(&huffmantree_builder, 0x51, 16);
	add_symbol(&huffmantree_builder, 0x50, 16);
	add_symbol(&huffmantree_builder, 0x4F, 16);
	add_symbol(&huffmantree_builder, 0x42, 16);
	add_symbol(&huffmantree_builder, 0x41, 16);
	add_symbol(&huffmantree_builder, 0x3F, 16);
	add_symbol(&huffmantree_builder, 0x3E, 16);
	add_symbol(&huffmantree_builder, 0x3D, 16);
	add_symbol(&huffmantree_builder, 0x3C, 16);
	add_symbol(&huffmantree_builder, 0x3B, 16);
	add_symbol(&huffmantree_builder, 0x3A, 16);
	add_symbol(&huffmantree_builder, 0x39, 16);
	add_symbol(&huffmantree_builder, 0x38, 16);
	add_symbol(&huffmantree_builder, 0x37, 16);
	add_symbol(&huffmantree_builder, 0x36, 16);
	add_symbol(&huffmantree_builder, 0x35, 16);
	add_symbol(&huffmantree_builder, 0x34, 16);
	add_symbol(&huffmantree_builder, 0x33, 16);
	add_symbol(&huffmantree_builder, 0x32, 16);
	add_symbol(&huffmantree_builder, 0x31, 16);
	add_symbol(&huffmantree_builder, 0x30, 16);
	add_symbol(&huffmantree_builder, 0x2F, 16);
	add_symbol(&huffmantree_builder, 0x21, 16);
	add_symbol(&huffmantree_builder, 0x1F, 16);
	add_symbol(&huffmantree_builder, 0x1E, 16);
	add_symbol(&huffmantree_builder, 0x1D, 16);
	add_symbol(&huffmantree_builder, 0x1C, 16);
	add_symbol(&huffmantree_builder, 0x1B, 16);
	add_symbol(&huffmantree_builder, 0x1A, 16);
	add_symbol(&huffmantree_builder, 0x19, 16);
	add_symbol(&huffmantree_builder, 0x18, 16);
	add_symbol(&huffmantree_builder, 0x17, 16);
	add_symbol(&huffmantree_builder, 0x16, 16);
	add_symbol(&huffmantree_builder, 0x15, 16);
	add_symbol(&huffmantree_builder, 0x14, 16);
	add_symbol(&huffmantree_builder, 0x13, 16);
	add_symbol(&huffmantree_builder, 0x12, 16);

	build_huffmantree(huffmantree_static, &huffmantree_builder);
}

void decompress(StateData* state_data, uint32_t decompressed_size, uint8_t* decompressed_data)
{
	uint32_t output_position = 0;

	drop_bits(state_data, 4);

	// Read the constant add size
	uint16_t write_size_const_add = 0;
	write_size_const_add = read_bits(state_data, 4);
	write_size_const_add += 1;
	drop_bits(state_data, 4);

	printf("Write size const add: %d\n", write_size_const_add);

	// Initialize Huffman trees and builder
	HuffmanTree huffmantree_symbol;
	HuffmanTree huffmantree_copy;
	HuffmanTreeBuilder huffmantree_builder;
	HuffmanTree huffmantree_static;

	initialize_static_huffmantree(&huffmantree_static);

	// Start decompressing while we have data to process
	while (output_position < decompressed_size)
	{
		clear_huffmantree(&huffmantree_symbol);
		clear_huffmantree(&huffmantree_copy);

		// Parse the Huffman trees for symbol and copy
		if (!parse_huffmantree(state_data, &huffmantree_symbol, &huffmantree_builder, &huffmantree_static) ||
			!parse_huffmantree(state_data, &huffmantree_copy, &huffmantree_builder, &huffmantree_static))
		{
			printf("Error: Failed to parse Huffman tree.\n");
			break; // Exit if parsing fails
		}

		// Read the max count value

		uint32_t max_count = 0;
		max_count = read_bits(state_data, 4);
		max_count = (max_count + 1) << 12;
		drop_bits(state_data, 4); // Drop the remaining 4 bits

		// Process each symbol until we reach max_count or decompressed_size
		uint32_t current_code_read_count = 0;
		while (current_code_read_count < max_count && output_position < decompressed_size)
		{
			++current_code_read_count;

			// Read the next symbol from the bitstream
			uint16_t symbol_data = 0;
			read_code(&huffmantree_static, state_data, &symbol_data);

			if (symbol_data < 0x100)
			{
				decompressed_data[output_position] = (uint8_t)symbol_data;
				++output_position;
				continue;
			}

			symbol_data -= 0x100;

			div_t code_div_4 = div(symbol_data, 4);

			uint32_t write_size = 0;
			if (code_div_4.quot == 0)
			{
				write_size = symbol_data;

			}
			else if (code_div_4.quot < 7)
			{
				write_size = ((1 << (code_div_4.quot - 1)) * (4 + code_div_4.rem));
			}
			else if (symbol_data == 28)
			{
				write_size = 0xFF;
			}
			else {
				printf("Invalid value for write size code!\n");
			}

			if (code_div_4.quot > 1 && symbol_data != 28)
			{
				uint8_t write_size_add_bits = (uint8_t)(code_div_4.quot - 1);
				uint32_t write_size_add;
				write_size_add = read_bits(state_data, write_size_add_bits);
				write_size |= write_size_add;
				drop_bits(state_data, write_size_add_bits);
			}

			write_size += write_size_const_add;
			read_code(&huffmantree_static, state_data, &symbol_data);

			div_t code_div_2 = div(symbol_data, 2);

			uint32_t write_offset = 0;

			if (code_div_2.quot == 0)
			{
				write_offset = symbol_data;
			}
			else if (code_div_2.quot < 17) {
				write_offset = ((1 << (code_div_2.quot - 1)) * (2 + code_div_2.rem));
			}
			else {
				printf("Invalid value for write offset code!\n");
			}

			if (code_div_2.quot > 1)
			{
				uint8_t write_offset_add_bits = (uint8_t)(code_div_2.quot - 1);
				uint32_t write_offset_add;
				read_bits(state_data, write_offset_add);
				write_offset |= write_offset_add;
				drop_bits(state_data, write_offset_add_bits);
			}

			write_offset += 1;
			uint32_t already_written = 0;
			while ((already_written < write_size) && (output_position < decompressed_size)) {
				decompressed_data[output_position] = decompressed_data[output_position - write_offset];
				++output_position;
				++already_written;

			}
		}

	}
}

uint8_t* decompress_data(uint8_t* compressed_data, uint32_t compressed_size, uint32_t* decompressed_size)
{
	if (compressed_data == NULL)
	{
		printf("There is no compressed data!\n");
		return NULL; // Return NULL to indicate an error
	}

	StateData state_data;
	state_data.bytes_available = compressed_size;
	state_data.input_buffer = compressed_data;
	state_data.buffer_position_bytes = 0;
	state_data.buffer_data = 0;
	uint32_t temp_head_data = 0;
	uint8_t temp_bytes_available_data = 0;

	pull_byte(&state_data, &temp_head_data, &temp_bytes_available_data);

	state_data.head_data = temp_head_data;
	state_data.bits_available_data = temp_bytes_available_data;

	uint32_t uncompressed_size = 0;

	drop_bits(&state_data, 32);

	uncompressed_size = read_bits(&state_data, 32);
	drop_bits(&state_data, 32);

	printf("Compressed size : %d \n", compressed_size);
	printf("Decompressed size : %d \n", uncompressed_size);

	// If the caller provided a pointer for decompressed size, store the size there
	if (decompressed_size != 0)
	{
		*decompressed_size = uncompressed_size;
	}

	// Allocate memory for decompressed data
	uint8_t* decompressed_data = (uint8_t*)malloc(sizeof(uint8_t) * uncompressed_size);
	if (decompressed_data == NULL)
	{
		printf("Memory allocation failed!\n");
		return NULL; // Return NULL if allocation fails
	}

	decompress(&state_data, uncompressed_size, decompressed_data);

	return decompressed_data; // Return the allocated buffer (currently empty)
}

#endif // DECOMPRESS_H
