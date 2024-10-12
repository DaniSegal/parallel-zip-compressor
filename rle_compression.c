#include "rle_compression.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

// Helper function to count consecutive characters
static uint32_t count_consecutive_chars(char *input, size_t input_size, size_t start_index) {
    char current_char = input[start_index];
    uint32_t char_count = 1;

    while ((start_index + char_count < input_size) && (input[start_index + char_count] == current_char)) {
        char_count++;
    }

    return char_count;
}

// Helper function to write RLE encoding to the output buffer
static void write_rle_encoding(char *out_buffer, size_t *out_buffer_index, char current_char, uint32_t char_count) {
    // Write char_count (4 bytes) and character (1 byte) to compressed_buffer_output
    memcpy(out_buffer + *out_buffer_index, &char_count, sizeof(uint32_t));
    *out_buffer_index += sizeof(uint32_t);
    out_buffer[*out_buffer_index] = current_char;
    (*out_buffer_index)++;
}

// Main RLE compression function
void compress_rle(char *input, size_t input_size, char **compressed_buffer_output, size_t *compressed_buffer_output_size) {
    // Allocate maximum possible compressed_buffer_output size (worst case)
    size_t max_output_size = input_size * (sizeof(uint32_t) + sizeof(char));
    char *out_buffer = malloc(max_output_size);
    if (!out_buffer) {
        perror("Error allocating memory for compressed data");
        exit(EXIT_FAILURE);
    }

    size_t out_buffer_index = 0;
    size_t i = 0;

    while (i < input_size) {
        char current_char = input[i];
        uint32_t char_count = count_consecutive_chars(input, input_size, i);
        write_rle_encoding(out_buffer, &out_buffer_index, current_char, char_count);
        i += char_count;
    }

    // Reallocate compressed_buffer_output to its actual size, can be removed for CPU efficiency if we can spare the 'wasted' memory
    char *temp_buffer = realloc(out_buffer, out_buffer_index);
    if (!temp_buffer) {
        perror("Error reallocating memory for compressed data");
        free(out_buffer);
        exit(EXIT_FAILURE);
    }
    out_buffer = temp_buffer;
    *compressed_buffer_output = out_buffer;
    *compressed_buffer_output_size = out_buffer_index;
}
