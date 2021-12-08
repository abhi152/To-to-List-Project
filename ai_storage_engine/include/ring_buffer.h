#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stddef.h>
#include <stdint.h>

/*
 * Fixed-capacity ring buffer for tracking buffer slot indices.
 * Used by the I/O engine to hand out and reclaim aligned read buffers.
 */
typedef struct {
    int *slots;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
} ring_buffer_t;

int  ring_buffer_init(ring_buffer_t *rb, size_t capacity);
void ring_buffer_destroy(ring_buffer_t *rb);

int  ring_buffer_push(ring_buffer_t *rb, int value);
int  ring_buffer_pop(ring_buffer_t *rb, int *value);

size_t ring_buffer_count(const ring_buffer_t *rb);
int    ring_buffer_is_empty(const ring_buffer_t *rb);
int    ring_buffer_is_full(const ring_buffer_t *rb);

#endif /* RING_BUFFER_H */
