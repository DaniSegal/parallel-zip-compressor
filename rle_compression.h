#ifndef RLE_COMPRESSION_H
#define RLE_COMPRESSION_H

#include <stddef.h>

/**
 * @brief Compresses the input data using Run Length Encoding (RLE) algorithm.
 *
 * This function takes an input buffer and compresses it using the Run Length Encoding (RLE)
 * algorithm. The compressed output is allocated and returned through the provided pointers.
 *
 * @param[in]  input                     The input buffer containing data to be compressed.
 * @param[in]  input_size                The size of the input buffer.
 * @param[out] compressed_buffer_output  Pointer to store the address of the compressed output buffer.
 * @param[out] compressed_buffer_output_size Pointer to store the size of the compressed output buffer.
 *
 * @note The caller is responsible for freeing the memory allocated for the compressed output buffer.
 */
void compress_rle(char *input, size_t input_size, char **compressed_buffer_output, size_t *compressed_buffer_output_size);

#endif // RLE_COMPRESSION_H