# Buildable reconstruction — keeps the project compiling as modules land.
CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -O2
# auto-discover every reconstructed module (new generated schemas join automatically)
SRC     := $(shell find finances players match tactics staff competitions clubs scouting -name '*.c' 2>/dev/null)
OBJ     := $(SRC:.c=.o)

.PHONY: all test clean
all: libcm2008.a

libcm2008.a: $(OBJ)
	ar rcs $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: tests/test_reconstruction
	./tests/test_reconstruction

tests/test_reconstruction: tests/test_reconstruction.c $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(OBJ) libcm2008.a tests/test_reconstruction
