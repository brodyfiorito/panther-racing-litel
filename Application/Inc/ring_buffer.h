#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include "app_config.h"

typedef struct {
    uint8_t  data[RING_BUFFER_SIZE];
    uint32_t head;
    uint32_t tail;
    uint32_t count;
} ring_buffer_t;

void ring_buffer_init(ring_buffer_t *rb);
bool ring_buffer_push(ring_buffer_t *rb, const current_data_t *data, uint8_t len);
bool ring_buffer_pop(ring_buffer_t *rb, uint8_t *out, uint32_t max_len, uint8_t *actual_len);
uint32_t ring_buffer_count(ring_buffer_t *rb);


#endif /* RING_BUFFER_H */