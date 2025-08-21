# Simple Makefile for Custom Shell

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g

# Directories
SRCDIR = src
INCDIR = include

# Source files
SOURCES = $(SRCDIR)/shell.c $(SRCDIR)/shell_input.c $(SRCDIR)/parser.c $(SRCDIR)/command.c $(SRCDIR)/hop.c

# Output executable
TARGET = shell.out

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -I$(INCDIR) $(SOURCES) -o $(TARGET)

# Clean build files
clean:
	rm -f $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

# Help target
help:
	@echo "Available targets:"
	@echo "  all     - Build the shell (default)"
	@echo "  clean   - Remove executable"
	@echo "  run     - Build and run the shell"
	@echo "  help    - Show this help"

.PHONY: all clean run help
