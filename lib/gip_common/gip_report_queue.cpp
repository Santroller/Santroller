#include "gip_report_queue.h"
#include <cstring>

// Max queued reports. Backed by a fixed static array (no malloc/free per push/pop) -
// dynamic allocation here previously caused multi-ms latency spikes during active
// GIP chunk exchange, coinciding with the report queue's push/pop calls.
#define GIP_REPORT_QUEUE_CAPACITY 8

struct gip_report_queue {
    gip_report_queue_item_t items[GIP_REPORT_QUEUE_CAPACITY];
    uint8_t head = 0;
    uint8_t count = 0;
};

extern "C" {

gip_report_queue_t* gip_report_queue_create()
{
    return new gip_report_queue_t();
}

void gip_report_queue_destroy(gip_report_queue_t* queue)
{
    if (queue) {
        delete queue;
    }
}

bool gip_report_queue_empty(gip_report_queue_t* queue)
{
    if (!queue) {
        return true;
    }
    return queue->count == 0;
}

const gip_report_queue_item_t* gip_report_queue_front(gip_report_queue_t* queue)
{
    if (!queue || queue->count == 0) {
        return nullptr;
    }
    return &queue->items[queue->head];
}

void gip_report_queue_pop(gip_report_queue_t* queue)
{
    if (queue && queue->count > 0) {
        queue->head = (queue->head + 1) % GIP_REPORT_QUEUE_CAPACITY;
        queue->count--;
    }
}

bool gip_report_queue_push(gip_report_queue_t* queue, const void* data, uint16_t len)
{
    if (!queue || !data || len == 0 || len > GIP_REPORT_QUEUE_MAX_SIZE || queue->count >= GIP_REPORT_QUEUE_CAPACITY) {
        return false;
    }
    
    uint8_t tail = (queue->head + queue->count) % GIP_REPORT_QUEUE_CAPACITY;
    gip_report_queue_item_t &item = queue->items[tail];
    memcpy(item.report, data, len);
    item.len = len;
    queue->count++;
    return true;
}

bool gip_report_queue_push_front(gip_report_queue_t* queue, const void* data, uint16_t len)
{
    if (!queue || !data || len == 0 || len > GIP_REPORT_QUEUE_MAX_SIZE || queue->count >= GIP_REPORT_QUEUE_CAPACITY) {
        return false;
    }

    queue->head = (queue->head + GIP_REPORT_QUEUE_CAPACITY - 1) % GIP_REPORT_QUEUE_CAPACITY;
    gip_report_queue_item_t &item = queue->items[queue->head];
    memcpy(item.report, data, len);
    item.len = len;
    queue->count++;
    return true;
}

} // extern "C"
