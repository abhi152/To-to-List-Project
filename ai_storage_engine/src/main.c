#define _GNU_SOURCE

#include "io_engine.h"

#include <stdio.h>
#include <stdlib.h>

static void print_stats(const char *label, const io_engine_stats_t *stats)
{
    double total_mb = (double)stats->total_bytes_read / (1024.0 * 1024.0);
    double speed_mbps = stats->elapsed_sec > 0.0
        ? total_mb / stats->elapsed_sec
        : 0.0;

    printf("\n--- %s ---\n", label);
    printf("Total Data Read : %.2f MB (%zu bytes)\n", total_mb, stats->total_bytes_read);
    printf("Time Taken      : %.4f seconds\n", stats->elapsed_sec);
    printf("Read Speed      : %.2f MB/s\n", speed_mbps);
}

int main(int argc, char *argv[])
{
    const char *filename = (argc > 1) ? argv[1] : "test_data.bin";

    io_engine_t *engine = NULL;
    if (io_engine_init(&engine, filename,
                       IO_ENGINE_DEFAULT_CHUNK_SIZE,
                       IO_ENGINE_DEFAULT_ALIGNMENT,
                       IO_ENGINE_DEFAULT_QUEUE_DEPTH) != 0) {
        perror("Failed to initialize io_engine");
        return 1;
    }

    printf("Starting io_uring async read on: %s (queue depth: %d)\n",
           filename, IO_ENGINE_DEFAULT_QUEUE_DEPTH);

    io_engine_stats_t stats;
    if (io_engine_read_all(engine, &stats) != 0) {
        perror("io_engine_read_all failed");
        io_engine_destroy(engine);
        return 1;
    }

    print_stats("io_uring Asynchronous Benchmark Results", &stats);
    io_engine_destroy(engine);
    return 0;
}
