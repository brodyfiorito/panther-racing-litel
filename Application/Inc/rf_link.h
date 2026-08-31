#ifndef RF_LINK_H
#define RF_LINK_H

#include "ring_buffer.h"

#define RF_DMA_BUFFER_SIZE 256u
#define RF_PACKET_MAX 16u

typedef struct {
    uint16_t uart_error;


} rf_link_stats_t;


void rf_link_init(ring_buffer_t *rb);
void rf_link_poll(void);
void rf_link_get_stats(rf_link_stats_t *out);


#endif /* RF_LINK_H */