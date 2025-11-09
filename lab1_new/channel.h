#pragma once
#include "fiber.h"
#include "fl_type.h"

#include <stddef.h>

typedef struct fl_channel {
    fl_value_t* buffer;

    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;

    fl_fiber* waiting_sender;
    fl_fiber* waiting_receiver;

    int closed;
} fl_channel;

fl_channel* fl_chan_create(size_t capacity);
void fl_chan_destroy(fl_channel* ch);

// Отправить значение (блокируется, если буфер полон)
void fl_chan_send(fl_channel* ch, fl_value_t value);

// Получить значение (блокируется, если буфер пуст)
fl_value_t fl_chan_recv(fl_channel* ch);

// Закрыть канал
void fl_chan_close(fl_channel* ch);