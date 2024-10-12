# Variables
CC = gcc
CFLAGS = -Wall -Werror -pthread -O
TARGET = pzip
SRC = pzip.c file_operations.c rle_compression.c

# Default target
all: $(TARGET)

# Compile the source file into the executable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Clean up any build artifacts
clean:
	rm -f $(TARGET)
