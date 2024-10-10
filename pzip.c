#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      // For open()
#include <sys/mman.h>   // For mmap()
#include <sys/stat.h>   // For fstat()
#include <unistd.h>     // For close()
#include <sys/sysinfo.h> // For get_nprocs_conf()
#include <pthread.h>

#define PART_SIZE 1024

typedef struct {
    char *data;
} FilePart;

size_t current_part = 0;
size_t num_parts = 0;
size_t file_size = 0;
char *file_data;
pthread_mutex_t mutex;

void cleanup(int fd, char *file_data, size_t file_size) {
    if (file_data != NULL && file_data != MAP_FAILED) {
        if (munmap(file_data, file_size) == -1) {
            perror("Error unmapping file");
        }
    }
    if (fd != -1) {
        close(fd);
    }
    pthread_mutex_destroy(&mutex);
}

void *compress_part(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex);
            if (current_part >= num_parts) {
                pthread_mutex_unlock(&mutex);
                break;
            }
            size_t part_index = current_part;
            current_part++;
        pthread_mutex_unlock(&mutex);

        char *part_data = file_data + (part_index * PART_SIZE);
        printf("Thread %ld processing part %zu\n", (long)arg, part_index);

        // Perform RLE compression here (placeholder for now)
        // Example: compress_rle(part_data, PART_SIZE);
    }
    return NULL;
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename1> <filename2> ...\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_threads = get_nprocs();

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
        cleanup(fd, file_data, file_size);
        return EXIT_FAILURE;
    }
    file_size = file_stat.st_size;

    // Step 3: Map the file into memory
    file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED) {
        perror("Error mapping file");
        cleanup(fd, file_data, file_size);
        return EXIT_FAILURE;
    }

    // Step 4: Calculate the number of parts
    num_parts = (file_size + PART_SIZE - 1) / PART_SIZE;
    pthread_mutex_init(&mutex, NULL);

    // Step 5: Create thread pool
    pthread_t threads[num_threads];
    for (long i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, compress_part, (void *)i) != 0) {
            perror("Error creating thread");
            cleanup(fd, file_data, file_size);
            return EXIT_FAILURE;
        }
    }

    // step 6: join the threads
    for (long i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Error joining thread");
            cleanup(fd, file_data, file_size);
            return EXIT_FAILURE;
        }
    }

    // Step 7: Cleanup
    cleanup(fd, file_data, file_size);

    printf("Number of available processors: %d\n", num_threads);

    return EXIT_SUCCESS;
}
