# Standalone build for the picker differential reference (HYPOTHESIS impl).
# Not wired into the production engine. `make test` builds and runs the tests.
#
# One-liner equivalent:
#   cc -std=c99 -O2 -Wall -Wextra -o /tmp/test_picker_ref picker_ref.c test_picker_ref.c && /tmp/test_picker_ref
CC      ?= cc
CFLAGS  ?= -std=c99 -O2 -Wall -Wextra
BIN     := test_picker_ref

.PHONY: test clean
test: $(BIN)
	./$(BIN)

$(BIN): picker_ref.c test_picker_ref.c picker_ref.h
	$(CC) $(CFLAGS) -o $(BIN) picker_ref.c test_picker_ref.c

clean:
	rm -f $(BIN)
