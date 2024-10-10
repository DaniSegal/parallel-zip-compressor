#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      // For open()
#include <sys/mman.h>   // For mmap()
#include <sys/stat.h>   // For fstat()
#include <unistd.h>     // For close()
#include <sys/sysinfo.h> // For get_nprocs_conf()

#define PART_SIZE 1024

typedef struct {
    char *data;
} FilePart;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename1> <filename2> ...\n", argv[0]);
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

    // Step 4: Split the data into parts and create the task queue
    size_t num_parts = (file_size + PART_SIZE - 1) / PART_SIZE; // Calculate the number of parts
    char **parts = malloc(num_parts * sizeof(char *));
    if (parts == NULL) {
        perror("Error allocating memory for parts");
        munmap(file_data, file_size);
        close(fd);
        return EXIT_FAILURE;
    }

     for (size_t i = 0; i < num_parts; i++) {
        parts[i] = file_data + (i * PART_SIZE);
        printf("Part %zu: Address = %p\n", i, (void *)parts[i]);
    }

    // Step 5: Access the data
    for (size_t i = 0; i < file_size; i++) {
        putchar(file_data[i]); // Print each character in the file
    }
    printf("\n");

    // Step 6: Unmap the memory
    if (munmap(file_data, file_size) == -1) {
        perror("Error unmapping file");
        close(fd);
        return EXIT_FAILURE;
    }

    // Step 7: Close the file
    close(fd);

    int available_procs = get_nprocs_conf();
    printf("Number of available processors: %d\n", available_procs);

    return EXIT_SUCCESS;
}
