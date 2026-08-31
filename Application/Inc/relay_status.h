#ifndef RELAY_STATUS_H
#define RELAY_STATUS_H

#include <stdint.h>

typedef struct {
    uint16_t frames_dropped;
    uint16_t frames_rx;


} relay_status_t;

void relay_logging_collect(relay_status_t *out); // reads relay sttas

#endif /* RELAY_STATUS_H */