# cm2008-runtime — headless runtime-research front-end (C11, native macOS).
CC     ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -O2
.PHONY: all clean
all: cm2008-runtime
cm2008-runtime: src/cm2008_runtime.c
	$(CC) $(CFLAGS) $^ -o $@
clean:
	rm -f cm2008-runtime
