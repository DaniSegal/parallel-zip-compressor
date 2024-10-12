
#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <stddef.h>

/**
 * @brief Opens and maps a file into memory.
 *
 * @param filename The name of the file to be opened and mapped.
 * @param fd Pointer to store the file descriptor.
 * @param file_size Pointer to store the size of the file.
 * @return Pointer to the mapped memory on success, or MAP_FAILED on failure.
 */
char *open_and_map_file(const char *filename, int *fd, size_t *file_size);

/**
 * @brief Releases resources by unmapping file data and closing the file.
 *
 * @param fd The file descriptor to be closed.
 * @param file_data Pointer to the mapped memory.
 * @param file_size The size of the mapped file.
 */
void release_resources(int fd, char *file_data, size_t file_size);

#endif // FILE_OPERATIONS_H
