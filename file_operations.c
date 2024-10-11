// file_operations.c

#include "file_operations.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      // For open()
#include <sys/mman.h>   // For mmap()
#include <sys/stat.h>   // For fstat()
#include <unistd.h>     // For close()

int open_file(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
    }
    return fd;
}

int get_file_size(int fd, size_t *file_size) {
    struct stat file_stat;
    if (fstat(fd, &file_stat) == -1) {
        perror("Error getting file size");
        return -1;
    }
    *file_size = file_stat.st_size;
    return 0;
}

char *map_file(int fd, size_t file_size) {
    char *data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("Error mapping file");
    }
    return data;
}

void unmap_file(char *file_data, size_t file_size) {
    if (file_data != NULL && file_data != MAP_FAILED) {
        if (munmap(file_data, file_size) == -1) {
            perror("Error unmapping file");
        }
    }
}

void close_file(int fd) {
    if (fd != -1) {
        close(fd);
    }
}

char *open_and_map_file(const char *filename, int *fd, size_t *file_size) {
    *fd = open_file(filename);
    if (*fd == -1) {
        return MAP_FAILED;
    }

    if (get_file_size(*fd, file_size) == -1) {
        close_file(*fd);
        return MAP_FAILED;
    }

    char *data = map_file(*fd, *file_size);
    if (data == MAP_FAILED) {
        close_file(*fd);
    }
    return data;
}

void release_resources(int fd, char *file_data, size_t file_size) {
    unmap_file(file_data, file_size);
    close_file(fd);
}
