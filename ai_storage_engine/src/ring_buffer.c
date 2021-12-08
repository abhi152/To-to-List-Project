#include "ring_buffer.h"

#include <errno.h>
#include <stdlib.h>

int ring_buffer_init(ring_buffer_t *rb, size_t capacity)
{
    if (!rb || capacity == 0) {
        errno = EINVAL;
        return -1;
    }

    rb->slots = malloc(capacity * sizeof(int));
    if (!rb->slots) {
        return -1;
    }

    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;

    for (size_t i = 0; i < capacity; i++) {
        rb->slots[i] = (int)i;
    }
    rb->count = capacity;

    return 0;
}

void ring_buffer_destroy(ring_buffer_t *rb)
{
    if (!rb) {
        return;
    }

    free(rb->slots);
    rb->slots = NULL;
    rb->capacity = 0;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

int ring_buffer_push(ring_buffer_t *rb, int value)
{
    if (!rb || ring_buffer_is_full(rb)) {
        errno = ENOSPC;
        return -1;
    }

    rb->slots[rb->tail] = value;
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->count++;
    return 0;
}

int ring_buffer_pop(ring_buffer_t *rb, int *value)
{
    if (!rb || !value || ring_buffer_is_empty(rb)) {
        errno = ENODATA;
        return -1;
    }

    *value = rb->slots[rb->head];
    rb->head = (rb->head + 1) % rb->capacity;
    rb->count--;
    return 0;
}

size_t ring_buffer_count(const ring_buffer_t *rb)
{
    return rb ? rb->count : 0;
}

int ring_buffer_is_empty(const ring_buffer_t *rb)
{
    return !rb || rb->count == 0;
}

int ring_buffer_is_full(const ring_buffer_t *rb)
{
    return rb && rb->count == rb->capacity;
}
