# Variables
CC = gcc
CFLAGS = -Wall -Werror -pthread -O
TARGET = testmain
SRC = testmain.c file_operations.c 

# Default target
all: $(TARGET)

# Compile the source file into the executable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Clean up any build artifacts
clean:
	rm -f $(TARGET)
