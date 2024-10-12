#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>      
#include <sys/mman.h>   
#include <sys/stat.h>   
#include <unistd.h>     
#include <sys/sysinfo.h> 
#include <pthread.h>
#include <string.h>
#include <stdint.h>
#include "file_operations.h"
#include "rle_compression.h"

#define PART_SIZE 1024

// Global variables
size_t current_part = 0;
size_t next_part_to_write = 0;
size_t num_parts = 0;
size_t last_part_size = 0;
size_t file_size = 0;
char *file_data;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void reset_globals() {
    current_part = 0;
    next_part_to_write = 0;
    num_parts = 0;
    last_part_size = 0;
    file_size = 0;
    file_data = NULL;
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

        // Calculate offset to know where is the current part's data
        size_t offset = part_index * PART_SIZE;

        // Calculate part size (handle last part)
        size_t part_size = (part_index == num_parts - 1) ? last_part_size : PART_SIZE;

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
        pthread_mutex_unlock(&write_mutex);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename1> <filename2> ...\n", argv[0]);
        return EXIT_FAILURE;
    }
    // Get the number of available processors and initialize a threads array
    int num_threads = get_nprocs();
    fprintf(stderr, "Number of available processors: %d\n", num_threads);
    pthread_t threads[num_threads];

    // Main loop to process all files
    for (int i = 1; i < argc; i++) {
        fprintf(stderr, "Processing file: %s\n", argv[i]);

        // Open the file, map the file into memory and get file size returned into dedicated variable
        int fd = -1;
        file_data = open_and_map_file(argv[i], &fd, &file_size);
        if (file_data == MAP_FAILED) {
            return EXIT_FAILURE;
        }

        // Calculate the number of parts and the size of the last part
        num_parts = (file_size + PART_SIZE - 1) / PART_SIZE;
        last_part_size = file_size % PART_SIZE;
        if (last_part_size == 0 && file_size > 0) {
            last_part_size = PART_SIZE;
        }

        // Create thread pool
        for (long i = 0; i < num_threads; i++) {
            if (pthread_create(&threads[i], NULL, compress_and_write_part, NULL) != 0) {
                perror("Error creating thread");
                release_resources(fd, file_data, file_size);
                return EXIT_FAILURE;
            }
        }

        // Join the threads
        for (long i = 0; i < num_threads; i++) {
            if (pthread_join(threads[i], NULL) != 0) {
                perror("Error joining thread");
                release_resources(fd, file_data, file_size);
                return EXIT_FAILURE;
            }
        }

        // Cleanup
        release_resources(fd, file_data, file_size);
        reset_globals();
    }

    // Destroy mutex and condition variable
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&write_mutex);
    pthread_cond_destroy(&cond);
    fprintf(stderr, "Compression completed successfully.\n");
    return EXIT_SUCCESS;
}