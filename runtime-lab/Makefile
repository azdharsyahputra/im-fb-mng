# cm2008-lab — runtime experiment lab (native macOS ARM64 / x86_64, C11).
# Reuses the repo's VERIFIED resolver parser + LCG (../match/sim) — no duplication.
CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -O2
REPO    := ..
INCL    := -Iinclude -I$(REPO)/match/sim
LDLIBS  := -lm

# Reused repo sources (parser + grid API; resolver_eval.h is header-only LCG).
REUSE   := $(REPO)/match/sim/resolver_dat.c

# Lab sources (every module except test mains).
LABSRC  := src/main.c \
           src/model/lab_model.c \
           src/context/lab_context.c \
           src/sampler/lab_sampler.c \
           src/experiment/lab_experiment.c \
           src/trace/lab_trace.c \
           src/statistics/lab_statistics.c

# All lab sources (top-level helpers + every module directory).
LABSRC  := $(wildcard src/*.c src/model/*.c src/context/*.c src/sampler/*.c src/experiment/*.c src/trace/*.c src/statistics/*.c)

.PHONY: all test clean
all: cm2008-lab

cm2008-lab: $(LABSRC) $(REUSE)
	$(CC) $(CFLAGS) $(INCL) $^ -o $@ $(LDLIBS)

# Test program links the lab modules (minus main.c) + the reused parser.
TESTLAB := $(filter-out src/main.c,$(LABSRC))
test: tests/lab_tests
	./tests/lab_tests

tests/lab_tests: tests/lab_tests.c $(TESTLAB) $(REUSE)
	$(CC) $(CFLAGS) $(INCL) $^ -o $@ $(LDLIBS)

clean:
	rm -f cm2008-lab tests/lab_tests
