#include "gip_report_queue.h"
#include <queue>
#include <cstring>

// C++ implementation using std::queue
struct gip_report_queue {
    std::queue<gip_report_queue_item_t> queue;
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
    return queue->queue.empty();
}

const gip_report_queue_item_t* gip_report_queue_front(gip_report_queue_t* queue)
{
    if (!queue || queue->queue.empty()) {
        return nullptr;
    }
    return &queue->queue.front();
}

void gip_report_queue_pop(gip_report_queue_t* queue)
{
    if (queue && !queue->queue.empty()) {
        queue->queue.pop();
    }
}

bool gip_report_queue_push(gip_report_queue_t* queue, const void* data, uint16_t len)
{
    if (!queue || !data || len == 0 || len > GIP_REPORT_QUEUE_MAX_SIZE) {
        return false;
    }
    
    queue->queue.emplace();
    gip_report_queue_item_t &item = queue->queue.back();
    memcpy(item.report, data, len);
    item.len = len;
    return true;
}

} // extern "C"
