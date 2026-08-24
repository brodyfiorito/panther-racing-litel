#ifndef CAN_INGEST_H
#define CAN_INGEST_H

#include "ring_buffer.h"

typedef struct {
    volatile uint16_t frames_rx;        // frames that made it into the ring_buffer and have been sent
    volatile uint16_t frames_dropped;   // frames dropped due to full FIFO
} can_ingest_stats_t;




void can_ingest_init(FDCAN_HandleTypeDef *hfdcan, ring_buffer_t *rb);
void can_ingest_poll(void);
void can_ingest_get_stats(can_ingest_stats_t *out);



#endif /* CAN_INGEST_H */