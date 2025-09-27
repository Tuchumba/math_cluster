CC ?= gcc
CSTD ?= -std=c11
WARN := -Wall -Wextra -Wpedantic -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wno-unused-parameter
OPT  ?= -O2
HARDEN := -fstack-protector-strong -D_FORTIFY_SOURCE=3 -fPIE -pie
LDFLAGS += -Wl,-z,relro,-z,now -lpthread -lm

BIN := bin
BUILD := build

LIBSRC := lib/net.c lib/common.c lib/integral.c lib/manager_impl.c lib/worker_impl.c
LIBOBJ := $(patsubst lib/%.c,$(BUILD)/%.o,$(LIBSRC))
LIB := $(BUILD)/libdistr.a

all: $(BIN)/manager $(BIN)/worker

$(BIN) $(BUILD):
	mkdir -p $@

$(LIB): $(LIBOBJ) | $(BUILD)
	$(AR) rcs $@ $^

$(BUILD)/%.o: lib/%.c lib/distr.h | $(BUILD)
	$(CC) $(CFLAGS) -Ilib -c $< -o $@

$(BIN)/manager: manager.c $(LIB) | $(BIN)
	$(CC) $(CFLAGS) -Ilib -o $@ manager.c $(LIB) $(LDFLAGS)

$(BIN)/worker: worker.c $(LIB) | $(BIN)
	$(CC) $(CFLAGS) -Ilib -o $@ worker.c $(LIB) $(LDFLAGS)

$(BIN)/test_manager: test_manager.c $(LIB) | $(BIN)
	$(CC) $(CFLAGS) -Ilib -o $@ test_manager.c $(LIB) $(LDFLAGS)

$(BIN)/test_worker: test_worker.c $(LIB) | $(BIN)
	$(CC) $(CFLAGS) -Ilib -o $@ test_worker.c $(LIB) $(LDFLAGS)

clean:
	rm -rf $(BUILD) $(BIN) tests/out

test: all
	bash scripts/test.sh

.PHONY: bench
bench: all
	bash scripts/bench.sh
