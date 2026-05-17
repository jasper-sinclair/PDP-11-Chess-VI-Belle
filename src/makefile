CC = gcc

SFLAGS = -Wall -Wextra -Wpedantic -std=c23
OFLAGS = -O3 -march=native -flto=auto -DNDEBUG
LFLAGS = -static

SRC = \
	$(wildcard *.c) \
	$(wildcard create_book/*.c)

OBJ = $(SRC:.c=.o)

BIN = chess6_mingw

$(BIN): $(OBJ)
	$(CC) $(OBJ) $(LFLAGS) -o $@
	@rm -f $(OBJ)

%.o: %.c
	$(CC) $(SFLAGS) $(OFLAGS) -c $< -o $@

clean:
	@rm -f $(OBJ) $(BIN)