#ifndef CAN_INGEST_H
#define CAN_INGEST_H

#include "ring_buffer.h"
#include "app_config.h"




void can_ingest_init(ring_buffer_t *rb);
void can_ingest_poll(void);
void can_ingest_get_current(current_data_t *out);



#endif // CAN_INGEST_H