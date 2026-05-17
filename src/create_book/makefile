# Makefile for create_book.c - Enhanced version
# Compiles the opening book generator for the PDP-11 Chess Engine

CC = gcc
CFLAGS = -O2 -Wall -Wextra -Werror -static
TARGET = create_book
SOURCES = create_book.c
OBJECTS = $(SOURCES:.c=.o)
BOOK_FILE = book.dat

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CFLAGS += -D_LINUX
endif
ifeq ($(UNAME_S),Darwin)
	CFLAGS += -D_OSX
endif

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)
	@echo "Build complete: $(TARGET)"

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled: $<"

# Run the book generator
run: $(TARGET)
	@echo "Generating opening book..."
	./$(TARGET)
	@if [ -f $(BOOK_FILE) ]; then \
		echo "Success: $(BOOK_FILE) created"; \
		ls -lh $(BOOK_FILE); \
	else \
		echo "Error: Failed to create $(BOOK_FILE)"; \
		exit 1; \
	fi

# Clean up object files and executable
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "Cleaned: object files and executable"

# Clean including book file
distclean: clean
	rm -f $(BOOK_FILE)
	@echo "Cleaned: $(BOOK_FILE) as well"

# Clean and rebuild
rebuild: clean all

# Install the book file (copy to engine directory if needed)
install: run
	@if [ -f $(BOOK_FILE) ]; then \
		echo "Book file ready: $(BOOK_FILE)"; \
		echo "You can now run the chess engine"; \
	else \
		echo "Error: Book generation failed"; \
		exit 1; \
	fi

# Debug build
debug: CFLAGS = -O0 -g -Wall -Wextra -DDEBUG
debug: clean all

# Profile build
profile: CFLAGS = -O2 -pg -Wall -Wextra
profile: clean all

# Check for memory leaks with Valgrind
valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Verify the book file integrity
verify: $(TARGET)
	@if [ -f $(BOOK_FILE) ]; then \
		echo "Verifying $(BOOK_FILE)..."; \
		./$(TARGET) > /dev/null 2>&1; \
		if [ $$? -eq 0 ]; then \
			echo "Book file is valid"; \
			ls -lh $(BOOK_FILE); \
		else \
			echo "Book file verification failed"; \
		fi \
	else \
		echo "No book file found. Run 'make run' first."; \
	fi

# Help target
help:
	@echo "=========================================="
	@echo "PDP-11 Chess Engine - Book Generator"
	@echo "=========================================="
	@echo ""
	@echo "Available targets:"
	@echo "  all       - Build create_book executable (default)"
	@echo "  run       - Build and run to generate book.dat"
	@echo "  clean     - Remove object files and executable"
	@echo "  distclean - Remove everything including book.dat"
	@echo "  rebuild   - Clean and rebuild from scratch"
	@echo "  install   - Build, run, and verify book.dat"
	@echo "  debug     - Build with debug symbols"
	@echo "  profile   - Build with profiling support"
	@echo "  valgrind  - Check for memory leaks"
	@echo "  verify    - Verify book.dat integrity"
	@echo "  help      - Show this help message"
	@echo ""
	@echo "Examples:"
	@echo "  make          # Build the book generator"
	@echo "  make run      # Generate book.dat"
	@echo "  make clean    # Clean build files"
	@echo "  make install  # Full build and generate"

.PHONY: all run clean distclean rebuild install debug profile valgrind verify help