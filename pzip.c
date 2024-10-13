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
int fd = -1;
int done = 0;

pthread_mutex_t current_part_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t write_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t write_cond = PTHREAD_COND_INITIALIZER;
pthread_barrier_t barrier;

/**
 * @brief Resets global variables used for tracking file processing.
 *
 * This function sets all global variables that are used to track the progress of
 * file processing back to their initial states. It should be called before starting
 * processing of a new file.
 */
void reset_globals() {
    current_part = 0;
    next_part_to_write = 0;
    num_parts = 0;
    last_part_size = 0;
    file_size = 0;
    file_data = NULL;
}

/**
 * @brief Gets the index of the next part of the file to be processed.
 *
 * This function is thread-safe and utilizes a mutex to manage access to the
 * shared `current_part` variable. It increments the `current_part` counter to
 * ensure that each thread processes a unique part of the file.
 *
 * @return The index of the next part to be processed, or -1 if all parts have been processed.
 */
size_t get_next_part() {
    size_t part_index;
    pthread_mutex_lock(&current_part_mutex);
        if (current_part >= num_parts) {
            pthread_mutex_unlock(&current_part_mutex);
            return -1;
        }
        part_index = current_part;
        current_part++;
    pthread_mutex_unlock(&current_part_mutex);
    return part_index;
}

/**
 * @brief Writes compressed data for a specified part to stdout in order.
 *
 * This function ensures that compressed parts are written to stdout in the correct order.
 * It uses a mutex and condition variable to manage safe and ordered access, ensuring that only
 * the thread responsible for the current part writes to stdout.
 *
 * @param compressed_data A pointer to the compressed data buffer to be written.
 * @param compressed_data_size The size of the compressed data buffer.
 * @param part_index The index of the file part being written.
 */
void write_compressed_part(char *compressed_data, size_t compressed_data_size, size_t part_index) {

    // Synchronize writing to the compressed_buffer_output file in order
    pthread_mutex_lock(&write_mutex);
    while (part_index != next_part_to_write) {
        pthread_cond_wait(&write_cond, &write_mutex);
    }
    // Write the compressed data to stdout
    fwrite(compressed_data, 1, compressed_data_size, stdout);
    free(compressed_data);

    // Increment the next part to write to maintain order
    next_part_to_write++;

    // Signal other threads that they may proceed
    pthread_cond_broadcast(&write_cond);
    pthread_mutex_unlock(&write_mutex);
}

/**
 * @brief The main core loop function for threads that compress and write file parts.
 *
 * Each thread executes this function, which processes file parts using an outer and an inner
 * while loop.
 *
 * - **Outer Loop**: Runs as long as the `done` flag is not set, allowing the thread to
 *   process multiple files. Each iteration begins by synchronizing at a barrier, ensuring
 *   all threads are ready for the next file prepared by the main thread.
 *
 * - **Inner Loop**: Within each file, this loop retrieves the next part, compresses it
 *   with RLE, and writes the output to stdout in order. It continues until all parts
 *   of the file are processed.
 *
 * After a file is completed, the thread waits at the barrier again, synchronizing with the
 * main thread before moving to the next file. This ensures all threads finish each file
 * before starting a new one.
 *
 * @param arg Unused parameter, passed to conform to pthread function signature.
 * @return Always returns NULL.
 */
void *compress_and_write_part(void *arg) {
    while (1) {
        pthread_barrier_wait(&barrier);
        if (done) break;
        while (1) {
            size_t part_index = get_next_part();
            if (part_index == -1) break;
            // Calculate offset to know where is the current part's data
            size_t offset = part_index * PART_SIZE;
            char *part_data = file_data + offset;

            // Calculate part size (handle last part)
            size_t part_size = (part_index == num_parts - 1) ? last_part_size : PART_SIZE;

            // Initialize variables to store compressed part data and size
            char *compressed_part_data = NULL;
            size_t compressed_part_size = 0;
            // Compress the part using RLE algorithm
            compress_rle(part_data, part_size, &compressed_part_data, &compressed_part_size); 

            // Write the compressed part to stdout
            write_compressed_part(compressed_part_data, compressed_part_size, part_index);
        }
        pthread_barrier_wait(&barrier);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename1> <filename2> ...\n", argv[0]);
        return EXIT_FAILURE;
    }
    // Get the number of available processors and initialize a threads array
    int num_threads = get_nprocs() - 1;
    fprintf(stderr, "Number of available processors: %d\n", num_threads);
    pthread_t threads[num_threads];

    pthread_barrier_init(&barrier, NULL, num_threads + 1);

    // Create thread pool
    for (long i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, compress_and_write_part, NULL) != 0) {
            perror("Error creating thread");
            release_resources(fd, file_data, file_size);
            return EXIT_FAILURE;
        }
    }

    // Main loop to process all files
    for (int i = 1; i < argc; i++) {
        fprintf(stderr, "Processing file: %s\n", argv[i]);

        // Open the file, map the file into memory and get file size returned into dedicated variable
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

        // Signal all threads to start processing the file
        pthread_barrier_wait(&barrier);
        // Wait for all threads to finish processing the file
        pthread_barrier_wait(&barrier);

        // Cleanup
        release_resources(fd, file_data, file_size);
        reset_globals();
    }
    
    // Signal all threads to finish
    done = 1;
    pthread_barrier_wait(&barrier);

    // Join the threads
    for (long i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Error joining thread");
            release_resources(fd, file_data, file_size);
            return EXIT_FAILURE;
        }
    }
    
    // Destroy mutex, condition and barrier variables
    pthread_mutex_destroy(&current_part_mutex);
    pthread_mutex_destroy(&write_mutex);
    pthread_cond_destroy(&write_cond);
    pthread_barrier_destroy(&barrier);

    fprintf(stderr, "Compression completed successfully.\n");
    return EXIT_SUCCESS;
}