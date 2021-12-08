# Compiler and flags
CC       := gcc
CFLAGS   := -Wall -Wextra -O2 -Iinclude
LDFLAGS  := -luring

# Directories
SRC_DIR  := src
BIN_DIR  := bin
BENCH_DIR := benchmarks

# Library objects (shared by main and benchmark)
LIB_SRCS := $(SRC_DIR)/io_engine.c $(SRC_DIR)/ring_buffer.c
LIB_OBJS := $(LIB_SRCS:.c=.o)

# Programs
MAIN_BIN    := $(BIN_DIR)/ai_storage_engine
BENCHMARK_BIN := $(BIN_DIR)/benchmark_vs_posix

.PHONY: all clean run benchmark test-data dirs

all: dirs $(MAIN_BIN) $(BENCHMARK_BIN)

dirs:
	@mkdir -p $(BIN_DIR)

# Link main program
$(MAIN_BIN): $(SRC_DIR)/main.c $(LIB_OBJS) | dirs
	$(CC) $(CFLAGS) -o $@ $(SRC_DIR)/main.c $(LIB_OBJS) $(LDFLAGS)

# Link benchmark program
$(BENCHMARK_BIN): $(BENCH_DIR)/benchmark_vs_posix.c $(LIB_OBJS) | dirs
	$(CC) $(CFLAGS) -o $@ $(BENCH_DIR)/benchmark_vs_posix.c $(LIB_OBJS) $(LDFLAGS)

# Compile .c -> .o
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Convenience targets
run: $(MAIN_BIN)
	./$(MAIN_BIN) test_data.bin

benchmark: $(BENCHMARK_BIN)
	./$(BENCHMARK_BIN) test_data.bin

# Create a 256 MB test file aligned for O_DIRECT reads
test-data:
	dd if=/dev/urandom of=test_data.bin bs=1M count=256 status=progress

clean:
	rm -rf $(BIN_DIR) $(SRC_DIR)/*.o
