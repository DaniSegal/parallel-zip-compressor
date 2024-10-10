#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      // For open()
#include <sys/mman.h>   // For mmap()
#include <sys/stat.h>   // For fstat()
#include <unistd.h>     // For close()
#include <sys/sysinfo.h> // For get_nprocs_conf()
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#define PART_SIZE 1024

// typedef struct {
//     char *data;
// } FilePart;

size_t current_part = 0;
size_t next_part_to_write = 0;
size_t num_parts = 0;
size_t file_size = 0;
char *file_data;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

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
    pthread_mutex_destroy(&write_mutex);
    pthread_cond_destroy(&cond);
}

void compress_rle(char *input, size_t input_size, char **output, size_t *output_size) {
    // Allocate maximum possible output size (worst case)
    size_t max_output_size = input_size * (sizeof(uint32_t) + sizeof(char)); // Max possible size
    char *out = malloc(max_output_size);
    if (!out) {
        perror("Error allocating memory for compressed data");
        exit(EXIT_FAILURE);
    }

    size_t out_index = 0;
    size_t i = 0;

    while (i < input_size) {
        char current_char = input[i];
        uint32_t count = 1;

        // Count consecutive characters
        while ((i + count < input_size) && (input[i + count] == current_char)) {
            count++;
        }

        // Write count (4 bytes) and character (1 byte) to output
        memcpy(out + out_index, &count, sizeof(uint32_t));
        out_index += sizeof(uint32_t);
        out[out_index++] = current_char;

        i += count;
    }

    // Reallocate output buffer to actual size
    out = realloc(out, out_index);
    if (!out) {
        perror("Error reallocating memory for compressed data");
        exit(EXIT_FAILURE);
    }

    *output = out;
    *output_size = out_index;
}

void *compress_and_write_part(void *arg) {
    while (1) {
        size_t part_index;

        // Get the next part to process safely
        pthread_mutex_lock(&mutex);
            if (current_part >= num_parts) {
                pthread_mutex_unlock(&mutex);
                break;
            }
            part_index = current_part;
            current_part++;
        pthread_mutex_unlock(&mutex);

        // Calculate part size (handle last part)
        size_t offset = part_index * PART_SIZE;
        size_t part_size = ((offset + PART_SIZE) <= file_size) ? PART_SIZE : (file_size - offset);

        char *part_data = file_data + offset;

        // Compress the part
        char *compressed_data = NULL;
        size_t compressed_size = 0;
        compress_rle(part_data, part_size, &compressed_data, &compressed_size);

        // Synchronize writing to the output file in order
        pthread_mutex_lock(&write_mutex);
        while (part_index != next_part_to_write) {
            // Wait until it's this part's turn to write
            pthread_cond_wait(&cond, &write_mutex);
        }

        
        fwrite(compressed_data, 1, compressed_size, stdout);
        free(compressed_data);
        next_part_to_write++;

        // Signal other threads that they may proceed
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&write_mutex);
        fprintf(stderr, "Thread processing part %zu\n", part_index);

    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename1> <filename2> ...\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_threads = get_nprocs();
    fprintf(stderr, "Number of available processors: %d\n", num_threads);

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


    // Step 5: Create thread pool
    pthread_t threads[num_threads];
    for (long i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, compress_and_write_part, NULL) != 0) {
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

    fprintf(stderr, "Compression completed successfully.\n");

    return EXIT_SUCCESS;
}
