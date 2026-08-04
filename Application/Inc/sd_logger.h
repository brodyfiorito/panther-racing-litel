#ifndef SD_LOGGER_H
#define SD_LOGGER_H


#include "ring_buffer.h"

typedef enum {
    SD_LOGGER_OK,
    SD_LOGGER_FAULT,
    SD_UNMOUNTED
} sd_logger_status_t;


sd_logger_status_t sd_logger_init(ring_buffer_t *rb);
void sd_logger_poll(void);
bool sd_unmount(void); // returns 0 on success



#endif /* SD_LOGGER_H */