
# Parallel File Compressor with RLE

This project is a multithreaded file compression tool implementing Run-Length Encoding (RLE) for data compression. Written in C, it demonstrates an approach to efficiently distributing tasks (in this case, RLE compression) across multiple worker threads to fully utilize all available CPU cores and boost performance. Using the pthread library, this tool achieves parallel processing while optimizing memory management through memory-mapped files (mmap). Designed to handle multiple files sequentially, it balances parallel and serialized operations to minimize memory usage and maintain correct output order through careful thread synchronization.

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


## Known Limitations

This project does not handle the specific case where a sequence of identical characters spans across two "parts" of data. 

### Example of the Limitation

Let's say we’re processing a file in parts of 5 bytes. If the input file contains a sequence like `'aaabbbbaaa'`, it would be split into two parts:

- **Part 1**: `'aaabb'`
- **Part 2**: `'bbaaa'`

With this setup, each part would be compressed individually, resulting in an output of `'3a2b'` for the first part and `'2b3a'` for the second. However, if we compressed the entire sequence at once without splitting, the ideal output would be `'3a4b3a'`.

### Why It Wasn’t Implemented

Fixing this edge case would require more complex synchronization between threads, including the need to “remember” data at the boundaries of each part and coordinate across threads. This would increase the program's memory (space complexity) and processing time (time complexity) because each thread would need to share and coordinate data across boundaries rather than working independently. Given that this issue is relatively minor and would come at a high cost to performance, I chose to prioritize simplicity and efficiency over handling this edge case.

