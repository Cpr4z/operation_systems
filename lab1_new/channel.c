#include "channel.h"

#include <stdlib.h>
#include <assert.h>

fl_channel* fl_chan_create(size_t capacity) {
    fl_channel* ch = calloc(1, sizeof(fl_channel));
    ch->buffer = calloc(capacity, sizeof(fl_value_t));
    ch->capacity = capacity;
    return ch;
}

void fl_chan_destroy(fl_channel* ch) {
    if (!ch) return;
    free(ch->buffer);
    free(ch);
}

void fl_chan_close(fl_channel* ch) {
    ch->closed = 1;
}

void fl_chan_send(fl_channel* ch, fl_value_t value) {
    fl_executor* e = g_exec_tls;
    assert(e && "fl_chan_send called outside fiber context");

    while (ch->count == ch->capacity) {
        ch->waiting_sender = e->current;
        fl_fiber_yield();
    }

    ch->buffer[ch->tail] = value;
    ch->tail = (ch->tail + 1) % ch->capacity;
    ch->count++;
}

fl_value_t fl_chan_recv(fl_channel* ch) {
    fl_executor* e = g_exec_tls;
    assert(e && "fl_chan_recv called outside fiber context");

    while (ch->count == 0 && !ch->closed) {
        ch->waiting_receiver = e->current;
        fl_fiber_yield();
    }

    if (ch->count == 0 && ch->closed)
        return (fl_value_t){FL_TYPE_NONE, {}};

    fl_value_t val = ch->buffer[ch->head];
    ch->head = (ch->head + 1) % ch->capacity;
    ch->count--;

    return val;
}
