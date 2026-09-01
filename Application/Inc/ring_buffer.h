/*
 * ring_buffer.h - lock-free SPSC ring for CAN frames
 *
 * One producer (ISR, push only), one consumer (main loop, peek/drop/*_take
 * only). No locking exists for a second of either.
 *
 * head/tail are free-running counters, never masked in place; occupancy is the
 * unsigned difference, correct across rollover. RB_CAPACITY must be a power of
 * two. peek() masks interrupts during the slot copy so a push burst can't tear
 * the frame being read.
 *
 * push() never checks fullness - it overwrites the oldest slot and advances.
 * Overrun detection is entirely in peek(): a lap skips tail forward to leave one
 * slot of margin and adds the loss to 'dropped'. Newest data wins, by design.
 * Don't add a fullness check to push() without reconciling it against that
 * margin.
 *
 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "stm32h5xx.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// depth chosen to absorb short ingest bursts; overrun intentionally discards oldest since only current values matter
#define RB_CAPACITY 256u
#define RB_MASK     (RB_CAPACITY - 1u)
_Static_assert((RB_CAPACITY & RB_MASK) == 0u, "RB_CAPACITY must be a power of two");

typedef struct {
    uint32_t id;
    uint8_t data[8];
    uint8_t len;     // actual # of bytes used
    uint32_t ts;     // capture time, us in the TIM2 base, wraps at 71.58 min
} can_frame_t;

typedef struct {
    can_frame_t slots[RB_CAPACITY];

    // unsigned subtraction preserves even through rollover
    uint32_t head;      // only producer changes
    uint32_t tail;      // only consumer changes

    uint32_t dropped;   // frames lost to overrun, saturating
    uint32_t hwm;       // high-water-mark, max occupancy seen since last read
} ring_buffer_t;


static inline void ring_buffer_init(ring_buffer_t *rb) {
    memset(rb, 0, sizeof(*rb));
}

// Producer: pushes things into the ring buffer, advances unconditionally
static inline void ring_buffer_push(ring_buffer_t *rb, uint32_t id, const uint8_t *data, uint8_t len, uint32_t ts_us) {

    if (len > 8u) len = 8u;

    uint32_t h = __atomic_load_n(&rb->head, __ATOMIC_RELAXED);
    can_frame_t *s = &rb->slots[h & RB_MASK];

    s->id = id;
    s->len = len;
    s->ts = ts_us;
    memcpy(s->data, data, len);

    // Release memory ahead of incoming data
    __atomic_store_n(&rb->head, h + 1u, __ATOMIC_RELEASE);
}

// See current tail of the ring buffer
static inline bool ring_buffer_peek(ring_buffer_t *rb, can_frame_t *out) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    // Interrupts off, so no atomics needed
    uint32_t h = rb->head;
    uint32_t t = rb->tail;
    uint32_t used = h - t;

    if (used > rb->hwm) {
        rb->hwm = used;
    }

    bool ok = false;

    if (used != 0u) {
        if (used > RB_CAPACITY - 1u) {
            uint32_t lost = used - (RB_CAPACITY - 1u);
            uint32_t d = rb->dropped;
            rb->dropped = (d + lost < d) ? UINT32_MAX : d + lost;
            t = h - (RB_CAPACITY - 1u);
            rb->tail = t;
        }
        *out = rb->slots[t & RB_MASK];
        ok = true;
    }

    __set_PRIMASK(primask);
    return ok;
}

// Remove oldest element from the ring buffer
static inline void ring_buffer_drop(ring_buffer_t *rb) {
    uint32_t t = __atomic_load_n(&rb->tail, __ATOMIC_RELAXED);
    uint32_t h = __atomic_load_n(&rb->head, __ATOMIC_ACQUIRE);

    if (h - t == 0u) return;   // Nothing to drop

    __atomic_store_n(&rb->tail, t + 1u, __ATOMIC_RELEASE);
}

static inline uint32_t ring_buffer_dropped_take(ring_buffer_t *rb) {
    return __atomic_exchange_n(&rb->dropped, 0u, __ATOMIC_RELAXED);
}

static inline uint32_t ring_buffer_hwm_take(ring_buffer_t *rb) {
    return __atomic_exchange_n(&rb->hwm, 0u, __ATOMIC_RELAXED);
}

#endif /* RING_BUFFER_H */