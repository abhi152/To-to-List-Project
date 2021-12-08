#define _GNU_SOURCE

#include "io_engine.h"
#include "ring_buffer.h"

#include <errno.h>
#include <fcntl.h>
#include <liburing.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

struct io_engine {
    int              fd;
    struct io_uring    ring;
    void           **buffers;
    ring_buffer_t    free_slots;
    size_t           chunk_size;
    size_t           alignment;
    int              queue_depth;
    off_t            file_size;
};

static double elapsed_sec(struct timespec start, struct timespec end)
{
    return (end.tv_sec - start.tv_sec) +
           (end.tv_nsec - start.tv_nsec) / 1e9;
}

int io_engine_init(io_engine_t **engine, const char *path,
                   size_t chunk_size, size_t alignment, int queue_depth)
{
    if (!engine || !path || chunk_size == 0 || alignment == 0 || queue_depth <= 0) {
        errno = EINVAL;
        return -1;
    }

    io_engine_t *e = calloc(1, sizeof(*e));
    if (!e) {
        return -1;
    }

    e->chunk_size = chunk_size;
    e->alignment = alignment;
    e->queue_depth = queue_depth;

    e->fd = open(path, O_RDONLY | O_DIRECT);
    if (e->fd < 0) {
        free(e);
        return -1;
    }

    e->file_size = lseek(e->fd, 0, SEEK_END);
    if (e->file_size <= 0) {
        close(e->fd);
        free(e);
        return -1;
    }
    lseek(e->fd, 0, SEEK_SET);

    if (io_uring_queue_init(queue_depth, &e->ring, 0) < 0) {
        close(e->fd);
        free(e);
        return -1;
    }

    e->buffers = calloc((size_t)queue_depth, sizeof(void *));
    if (!e->buffers) {
        io_uring_queue_exit(&e->ring);
        close(e->fd);
        free(e);
        return -1;
    }

    for (int i = 0; i < queue_depth; i++) {
        if (posix_memalign(&e->buffers[i], alignment, chunk_size) != 0) {
            for (int j = 0; j < i; j++) {
                free(e->buffers[j]);
            }
            free(e->buffers);
            io_uring_queue_exit(&e->ring);
            close(e->fd);
            free(e);
            return -1;
        }
    }

    if (ring_buffer_init(&e->free_slots, (size_t)queue_depth) != 0) {
        for (int i = 0; i < queue_depth; i++) {
            free(e->buffers[i]);
        }
        free(e->buffers);
        io_uring_queue_exit(&e->ring);
        close(e->fd);
        free(e);
        return -1;
    }

    *engine = e;
    return 0;
}

int io_engine_read_all(io_engine_t *engine, io_engine_stats_t *stats)
{
    if (!engine || !stats) {
        errno = EINVAL;
        return -1;
    }

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    off_t current_offset = 0;
    size_t total_bytes_read = 0;
    int in_flight = 0;

    while (current_offset < engine->file_size || in_flight > 0) {
        while (in_flight < engine->queue_depth && current_offset < engine->file_size) {
            struct io_uring_sqe *sqe = io_uring_get_sqe(&engine->ring);
            if (!sqe) {
                break;
            }

            int buf_idx;
            if (ring_buffer_pop(&engine->free_slots, &buf_idx) != 0) {
                break;
            }

            io_uring_prep_read(sqe, engine->fd, engine->buffers[buf_idx],
                               engine->chunk_size, current_offset);
            io_uring_sqe_set_data(sqe, (void *)(long)buf_idx);

            current_offset += (off_t)engine->chunk_size;
            in_flight++;
        }

        io_uring_submit(&engine->ring);

        struct io_uring_cqe *cqe;
        if (io_uring_wait_cqe(&engine->ring, &cqe) < 0) {
            return -1;
        }

        if (cqe->res < 0) {
            fprintf(stderr, "Async read error: %d\n", cqe->res);
        } else {
            total_bytes_read += (size_t)cqe->res;
        }

        int buf_idx = (int)(long)io_uring_cqe_get_data(cqe);
        ring_buffer_push(&engine->free_slots, buf_idx);

        io_uring_cqe_seen(&engine->ring, cqe);
        in_flight--;
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);

    memset(stats, 0, sizeof(*stats));
    stats->fd = engine->fd;
    stats->chunk_size = engine->chunk_size;
    stats->alignment = engine->alignment;
    stats->queue_depth = engine->queue_depth;
    stats->file_size = engine->file_size;
    stats->total_bytes_read = total_bytes_read;
    stats->elapsed_sec = elapsed_sec(start_time, end_time);

    return 0;
}

void io_engine_destroy(io_engine_t *engine)
{
    if (!engine) {
        return;
    }

    if (engine->buffers) {
        for (int i = 0; i < engine->queue_depth; i++) {
            free(engine->buffers[i]);
        }
        free(engine->buffers);
    }

    ring_buffer_destroy(&engine->free_slots);
    io_uring_queue_exit(&engine->ring);

    if (engine->fd >= 0) {
        close(engine->fd);
    }

    free(engine);
}
