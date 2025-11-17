#pragma once

#include "fiber.h"
#include "fl_type.h"

#include <pthread.h>

typedef enum {
    FL_CH_OPEN,
    FL_CH_CLOSED,
} fl_chan_state;

typedef struct fl_channel {
    fl_value_t* buffer;

    size_t capacity; // емкость буфера

    // указатели для кольцевого буфера
    size_t head;
    size_t tail;
    size_t count;

    fl_fiber* waiting_sender; // отправляющий данные файбер
    fl_fiber* waiting_receiver; // ожидающий данные файбер

    fl_chan_state state; // состояние канала

    pthread_mutex_t lock;

    // условные переменные
    pthread_cond_t  can_send;
    pthread_cond_t  can_recv;
} fl_channel;

fl_channel* fl_chan_create(size_t capacity);
void fl_chan_destroy(fl_channel* ch);

// Отправить значение (блокируется, если буфер полон)
void fl_chan_send(fl_channel* ch, fl_value_t value);

// Получить значение (блокируется, если буфер пуст)
fl_value_t fl_chan_recv(fl_channel* ch);

// Закрыть канал
void fl_chan_close(fl_channel* ch);