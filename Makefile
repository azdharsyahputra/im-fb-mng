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

.PHONY: all test demo quickdemo integration subtests clean
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

# Cross-subsystem integration tests (WORKSTREAM E). Standalone program with its own
# main(); links only the subsystem .c it exercises + -lm. Lives under tests/ so the
# main `test`/`all` globs (which scan match/ etc.) never pull it in.
INTEGSRC := tests/test_integration.c \
            match/sim/match_state.c match/sim/match_lifecycle.c match/sim/detailed_engine.c \
            match/sim/resolver_dat.c match/sim/resolver_eval.c \
            match/formula/formula_eval.c match/formula/formula_registry.c

integration: tests/test_integration
	./tests/test_integration

tests/test_integration: $(INTEGSRC)
	$(CC) $(CFLAGS) $^ -o $@ -lm

# Per-subsystem standalone tests (own main(); live under tests/ so the globbed build
# never pulls them in).
tests/test_formula_parse: tests/test_formula_parse.c match/formula/formula_eval.c match/formula/formula_parse.c match/formula/formula_registry.c
	$(CC) $(CFLAGS) $^ -o $@ -lm
tests/test_resolver_settings: tests/test_resolver_settings.c match/model/resolver_settings.c match/formula/formula_eval.c match/formula/formula_registry.c
	$(CC) $(CFLAGS) $^ -o $@ -lm
tests/test_match_loop: tests/test_match_loop.c match/engine/match_loop.c match/sim/match_state.c match/sim/match_lifecycle.c match/sim/detailed_engine.c
	$(CC) $(CFLAGS) $^ -o $@ -lm
tests/test_movement: tests/test_movement.c match/movement/contest.c match/sim/resolver_eval.c
	$(CC) $(CFLAGS) $^ -o $@ -lm

subtests: tests/test_formula_parse tests/test_resolver_settings tests/test_match_loop tests/test_movement tests/test_integration
	./tests/test_formula_parse && ./tests/test_resolver_settings && ./tests/test_match_loop && ./tests/test_movement && ./tests/test_integration

clean:
	rm -f $(OBJ) libcm2008.a tests/test_reconstruction tests/test_integration \
	      tests/test_formula_parse tests/test_resolver_settings tests/test_match_loop tests/test_movement
