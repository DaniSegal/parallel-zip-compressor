#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      // For open()
#include <sys/mman.h>   // For mmap()
#include <sys/stat.h>   // For fstat()
#include <unistd.h>     // For close()

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Step 1: Open the file
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    // Step 2: Get the file size
    struct stat file_stat;    
    if (fstat(fd, &file_stat) == -1) {
        perror("Error getting file size");
        close(fd);
        return EXIT_FAILURE;
    }
    size_t file_size = file_stat.st_size;

    // Step 3: Map the file into memory
    char *file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        return EXIT_FAILURE;
    }

    // Step 4: Access the data
    for (size_t i = 0; i < file_size; i++) {
        putchar(file_data[i]); // Print each character in the file
    }
    printf("\n");

    // Step 5: Unmap the memory
    if (munmap(file_data, file_size) == -1) {
        perror("Error unmapping file");
        close(fd);
        return EXIT_FAILURE;
    }

    // Step 6: Close the file
    close(fd);

    return EXIT_SUCCESS;
}
