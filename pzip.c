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

void compress_rle(char *input, size_t input_size, char **compressed_buffer_output, size_t *compressed_buffer_output_size) {
    // Allocate maximum possible compressed_buffer_output size (worst case)
    size_t max_output_size = input_size * (sizeof(uint32_t) + sizeof(char)); // Max possible size
    char *out_buffer = malloc(max_output_size);
    if (!out_buffer) {
        perror("Error allocating memory for compressed data");
        exit(EXIT_FAILURE);
    }

    size_t out_buffer_index = 0;
    size_t i = 0;

    while (i < input_size) {
        char current_char = input[i];
        uint32_t char_count = 1;

        // Count consecutive characters
        while ((i + char_count < input_size) && (input[i + char_count] == current_char)) {
            char_count++;
        }

        // Write char_count (4 bytes) and character (1 byte) to compressed_buffer_output
        memcpy(out_buffer + out_buffer_index, &char_count, sizeof(uint32_t));
        out_buffer_index += sizeof(uint32_t);
        out_buffer[out_buffer_index] = current_char;
        out_buffer_index++;

        i += char_count;
    }

    // Reallocate compressed_buffer_output buffer to actual size
    out_buffer = realloc(out_buffer, out_buffer_index);
    if (!out_buffer) {
        perror("Error reallocating memory for compressed data");
        exit(EXIT_FAILURE);
    }

    *compressed_buffer_output = out_buffer;
    *compressed_buffer_output_size = out_buffer_index;
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
        char *compressed_part_data = NULL;
        size_t compressed_size = 0;
        compress_rle(part_data, part_size, &compressed_part_data, &compressed_size);

        // Synchronize writing to the compressed_buffer_output file in order
        pthread_mutex_lock(&write_mutex);
        while (part_index != next_part_to_write) {
            // Wait until it's this part's turn to write
            pthread_cond_wait(&cond, &write_mutex);
        }

        
        fwrite(compressed_part_data, 1, compressed_size, stdout);
        free(compressed_part_data);
        next_part_to_write++;

        // Signal other threads that they may proceed
        pthread_cond_broadcast(&cond);
        fprintf(stderr, "Thread processing part %zu\n", part_index);
        pthread_mutex_unlock(&write_mutex);
        

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
