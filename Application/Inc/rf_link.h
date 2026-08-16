#ifndef RF_TELEMETRY_H
#define RF_TELEMETRY_H

#include "ring_buffer.h"



void rf_telemetry_init(void);
void rf_telemetry_poll(void);
void rf_telemetry_suspend(void);
void rf_telemetry_resume(void);

#endif /* RF_TELEMETRY_H */