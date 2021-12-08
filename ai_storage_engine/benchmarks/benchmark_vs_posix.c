#define _GNU_SOURCE

#include "io_engine.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define CHUNK_SIZE (64 * 1024)
#define ALIGNMENT  4096

static double elapsed_sec(struct timespec start, struct timespec end)
{
    return (end.tv_sec - start.tv_sec) +
           (end.tv_nsec - start.tv_nsec) / 1e9;
}

static int posix_read_benchmark(const char *path, size_t *bytes_read, double *elapsed)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open (posix benchmark)");
        return -1;
    }

    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size <= 0) {
        perror("lseek");
        close(fd);
        return -1;
    }
    lseek(fd, 0, SEEK_SET);

    void *buffer = NULL;
    if (posix_memalign(&buffer, ALIGNMENT, CHUNK_SIZE) != 0) {
        perror("posix_memalign");
        close(fd);
        return -1;
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    size_t total = 0;
    off_t offset = 0;
    while (offset < file_size) {
        ssize_t n = pread(fd, buffer, CHUNK_SIZE, offset);
        if (n < 0) {
            perror("pread");
            free(buffer);
            close(fd);
            return -1;
        }
        if (n == 0) {
            break;
        }
        total += (size_t)n;
        offset += n;
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    *bytes_read = total;
    *elapsed = elapsed_sec(start, end);

    free(buffer);
    close(fd);
    return 0;
}

static void print_result(const char *label, size_t bytes, double elapsed)
{
    double mb = (double)bytes / (1024.0 * 1024.0);
    double mbps = elapsed > 0.0 ? mb / elapsed : 0.0;

    printf("%-18s | %8.2f MB | %8.4f s | %8.2f MB/s\n",
           label, mb, elapsed, mbps);
}

int main(int argc, char *argv[])
{
    const char *filename = (argc > 1) ? argv[1] : "test_data.bin";

    printf("Benchmark file: %s\n\n", filename);
    printf("%-18s | %10s | %10s | %12s\n", "Method", "Data", "Time", "Speed");
    printf("-------------------+------------+------------+--------------\n");

    size_t posix_bytes = 0;
    double posix_time = 0.0;
    if (posix_read_benchmark(filename, &posix_bytes, &posix_time) == 0) {
        print_result("POSIX pread", posix_bytes, posix_time);
    }

    io_engine_t *engine = NULL;
    if (io_engine_init(&engine, filename,
                       IO_ENGINE_DEFAULT_CHUNK_SIZE,
                       IO_ENGINE_DEFAULT_ALIGNMENT,
                       IO_ENGINE_DEFAULT_QUEUE_DEPTH) != 0) {
        perror("io_engine_init");
        return 1;
    }

    io_engine_stats_t stats;
    if (io_engine_read_all(engine, &stats) != 0) {
        perror("io_engine_read_all");
        io_engine_destroy(engine);
        return 1;
    }

    print_result("io_uring O_DIRECT", stats.total_bytes_read, stats.elapsed_sec);
    io_engine_destroy(engine);

    return 0;
}
