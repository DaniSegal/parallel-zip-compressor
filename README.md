
# Parallel File Compressor with RLE

This project is a multithreaded file compression tool that uses Run-Length Encoding (RLE) to efficiently compress data. Built in C, it employs the `pthread` library for parallel processing and optimized memory management using memory-mapped files (mmap). Designed to handle multiple files while balancing parallelism and serialized operations, this tool minimizes memory use by processing files individually and ensures output order through controlled synchronization.

## Features

- **Memory Mapping**: Uses `mmap` to map each file into memory for efficient access and manipulation.
- **Parallelism**: Dynamically determines the optimal number of threads (`get_nprocs()`) to divide files into manageable parts, each processed concurrently.
- **Serialized File Handling**: Files are processed one by one to save memory, with threads waiting at a barrier while each file is unmapped and the next file is loaded.
- **RLE Compression**: Implements a standard RLE algorithm for compressing data in O(n) time complexity.
- **Output Order Maintenance**: Threads write compressed data to `stdout` in the same order it was read, thanks to mutex-protected writes and condition variables.

## Getting Started

### Prerequisites
- **gcc**: Install via package manager (`sudo apt install gcc` on Linux) if not already available.
- **pthread library**: Typically included with most GCC installations.

### Building the Project
To compile the project, simply run:
```bash
make
```
This will compile all necessary files and create the executable `pzip`.

### Usage
To compress files, run the following command:
```bash
./pzip file1.txt file2.txt ...
```
Replace `file1.txt`, `file2.txt`, etc., with the names of the files you wish to compress. The compressed output is written to `stdout`.

## Design Highlights

- **Memory Efficiency**: Files are processed sequentially, with each file being unmapped after processing, conserving memory.
- **Parallel Processing**: Each file is divided into 1MB parts (configurable), processed by worker threads managed through a task queue model.
- **Order-Safe Output**: Maintains chronological output order using mutexes and condition variables for synchronized writing.

## Known Limitations
- **Edge Case**: RLE compression does not handle sequences that span across two parts of data due to the complexity it would add. For example, if `aaabbbbaaa` were split, it would compress inaccurately across parts.
