#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Depth is set low due to latency concerns, we don't really care about anything not current
#define RB_CAPACITY 16u
#define RB_MASK     (RB_CAPACITY - 1u)

typedef struct {
    uint32_t id;
    uint8_t  data[8];
} can_frame_t;

typedef struct {
    can_frame_t slots[RB_CAPACITY];

    // unsigned subtraction preserves even through rollover
    _Atomic uint32_t head;      // only producer changes
    _Atomic uint32_t tail;      // only consumer changes

    _Atomic uint32_t dropped;   /* frames lost to overrun, saturating */
} ring_buffer_t;


static inline void ring_buffer_init(ring_buffer_t *rb) {
    memset(rb, 0, sizeof(*rb));
}

// Producer: pushes things into the ring buffer, overwiritng the oldest entry
static inline void ring_buffer_push(ring_buffer_t *rb, uint32_t id, const uint8_t *data) {
    uint32_t h = __atomic_load_n(&rb->head, __ATOMIC_RELAXED);
    can_frame_t *s = &rb->slots[h & RB_MASK];

    s->id = id;
    memcpy(s->data, data, 8);

    // Release memory ahead of incoming data
    __atomic_store_n(&rb->head, h + 1u, __ATOMIC_RELEASE);
}

// Consumer: gives data at tail of buffer, returns false if empty
static inline bool ring_buffer_pop(ring_buffer_t *rb, can_frame_t *out) {
    uint32_t h = __atomic_load_n(&rb->head, __ATOMIC_ACQUIRE);
    uint32_t t = __atomic_load_n(&rb->tail, __ATOMIC_RELAXED);

    uint32_t used = h - t;
    if (used == 0u) {
        return false;
    }

    /* Overrun: the ISR lapped us. Skip ahead, leaving one slot of margin so
       we never read the slot currently being written. */
    if (used > RB_CAPACITY - 1u) {
        uint32_t lost = used - (RB_CAPACITY - 1u);
        uint32_t d = __atomic_load_n(&rb->dropped, __ATOMIC_RELAXED);
        if (d + lost < d) { d = UINT32_MAX; } else { d += lost; }
        __atomic_store_n(&rb->dropped, d, __ATOMIC_RELAXED);

        t = h - (RB_CAPACITY - 1u);
    }

    *out = rb->slots[t & RB_MASK];
    __atomic_store_n(&rb->tail, t + 1u, __ATOMIC_RELEASE);
    return true;
}

static inline uint32_t ring_buffer_dropped_take(ring_buffer_t *rb) {
    return __atomic_exchange_n(&rb->dropped, 0u, __ATOMIC_RELAXED);
}

#endif /* RING_BUFFER_H */