#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* Feature Flags */


/* DATA */
typedef struct {    // whatever data we are sending over RF



} current_data_t;

/* Buffer Sizes */
#define RING_BUFFER_SIZE 4096
#define SD_CHUNK_SIZE 2048

/* Shutdown */
#define STATUS_PIN_HYST_MS 5



/* RF */
#define RF_SEND_INTERVAL_MS 10
#define RF_PACKET_SIZE 100  // needs recalc after data is confirmed

#endif /* APP_CONFIG_H */