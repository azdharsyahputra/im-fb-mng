# Buildable reconstruction — keeps the project compiling as modules land.
CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -O2
# auto-discover every reconstructed module (new generated schemas join automatically)
# Exclude standalone reference subsystems (own main + own Makefile, e.g. picker_ref).
ALLC    := $(shell find finances players match tactics staff competitions clubs scouting -name '*.c' -not -path '*/picker_ref/*' 2>/dev/null)
# library excludes files with their own main() (the demo)
SRC     := $(filter-out match/sim/sim_demo.c match/sim/quick_demo.c,$(ALLC))
OBJ     := $(SRC:.c=.o)
SIMSRC  := match/sim/sim_core.c match/sim/movement.c match/sim/defending.c match/sim/events.c

.PHONY: all test demo quickdemo clean
all: libcm2008.a

demo: match/sim/cmdemo
	./match/sim/cmdemo

match/sim/cmdemo: match/sim/sim_demo.c $(SIMSRC)
	$(CC) $(CFLAGS) $^ -o $@ -lm

quickdemo: match/sim/cmquick
match/sim/cmquick: match/sim/quick_demo.c match/sim/resolver.c match/sim/resolver_dat.c
	$(CC) $(CFLAGS) $^ -o $@ -lm

libcm2008.a: $(OBJ)
	ar rcs $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: tests/test_reconstruction
	./tests/test_reconstruction

tests/test_reconstruction: tests/test_reconstruction.c $(SRC)
	$(CC) $(CFLAGS) $^ -o $@ -lm

clean:
	rm -f $(OBJ) libcm2008.a tests/test_reconstruction
