#ifndef IO_ENGINE_H
#define IO_ENGINE_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define IO_ENGINE_DEFAULT_CHUNK_SIZE  (64 * 1024)   /* 64 KB */
#define IO_ENGINE_DEFAULT_ALIGNMENT   4096          /* 4 KB page alignment */
#define IO_ENGINE_DEFAULT_QUEUE_DEPTH 32

typedef struct {
    int           fd;
    size_t        chunk_size;
    size_t        alignment;
    int           queue_depth;
    off_t         file_size;
    size_t        total_bytes_read;
    double        elapsed_sec;
} io_engine_stats_t;

typedef struct io_engine io_engine_t;

/*
 * Open a file with O_DIRECT and prepare an io_uring read pipeline.
 * Returns 0 on success, -1 on failure (errno set).
 */
int io_engine_init(io_engine_t **engine, const char *path,
                   size_t chunk_size, size_t alignment, int queue_depth);

/*
 * Read the entire file asynchronously using io_uring.
 * Populates stats on success. Returns 0 on success, -1 on failure.
 */
int io_engine_read_all(io_engine_t *engine, io_engine_stats_t *stats);

void io_engine_destroy(io_engine_t *engine);

#endif /* IO_ENGINE_H */
