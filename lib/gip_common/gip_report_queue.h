#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum report size (large enough for both wired and wireless)
#define GIP_REPORT_QUEUE_MAX_SIZE 512

// Report queue item
typedef struct {
    uint8_t report[GIP_REPORT_QUEUE_MAX_SIZE];
    uint16_t len;
} gip_report_queue_item_t;

// Report queue structure (opaque)
typedef struct gip_report_queue gip_report_queue_t;

/**
 * Create a new report queue
 * 
 * @return Pointer to new queue, or NULL on failure
 */
gip_report_queue_t* gip_report_queue_create(void);

/**
 * Destroy a report queue
 * 
 * @param queue Queue to destroy
 */
void gip_report_queue_destroy(gip_report_queue_t* queue);

/**
 * Check if queue is empty
 * 
 * @param queue Queue to check
 * @return true if empty, false otherwise
 */
bool gip_report_queue_empty(gip_report_queue_t* queue);

/**
 * Get the front item from the queue (without removing it)
 * 
 * @param queue Queue to get from
 * @return Pointer to front item, or NULL if empty
 */
const gip_report_queue_item_t* gip_report_queue_front(gip_report_queue_t* queue);

/**
 * Remove the front item from the queue
 * 
 * @param queue Queue to pop from
 */
void gip_report_queue_pop(gip_report_queue_t* queue);

/**
 * Add an item to the queue
 * 
 * @param queue Queue to add to
 * @param report Report data
 * @param len Report length
 * @return true if added successfully, false if queue is full
 */
bool gip_report_queue_push(gip_report_queue_t* queue, const void* report, uint16_t len);

#ifdef __cplusplus
}
#endif
