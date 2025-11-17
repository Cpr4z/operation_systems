#include "channel.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

fl_channel* fl_chan_create(size_t capacity) {
    fl_channel* ch = calloc(1, sizeof(fl_channel));
    if (!ch) {
        return NULL;
    }

    ch->buffer = calloc(capacity, sizeof(fl_value_t));
    if (!ch->buffer) {
        free(ch);
        return NULL;
    }

    ch->capacity = capacity;
    ch->head = 0;
    ch->tail = 0;
    ch->count = 0;
    ch->waiting_sender = NULL;
    ch->waiting_receiver = NULL;
    ch->state = FL_CH_OPEN;

    pthread_mutex_init(&ch->lock, NULL);
    pthread_cond_init(&ch->can_send, NULL);
    pthread_cond_init(&ch->can_recv, NULL);

    return ch;
}

void fl_chan_destroy(fl_channel* ch) {
    if (!ch) return;

    pthread_mutex_destroy(&ch->lock);
    pthread_cond_destroy(&ch->can_send);
    pthread_cond_destroy(&ch->can_recv);

    free(ch->buffer);
    free(ch);
}

void fl_chan_close(fl_channel* ch) {
    if (!ch) return;
    pthread_mutex_lock(&ch->lock);
    ch->state = FL_CH_CLOSED;

    pthread_cond_broadcast(&ch->can_send);
    pthread_cond_broadcast(&ch->can_recv);
    pthread_mutex_unlock(&ch->lock);
}

void fl_chan_send(fl_channel* ch, fl_value_t value) {
    fl_executor* e = g_exec_tls;
    assert(e && e->current);
    assert(e->current->state == FL_RUNNING);

    pthread_mutex_lock(&ch->lock);

    if (ch->state == FL_CH_CLOSED) {
        pthread_mutex_unlock(&ch->lock);
        printf("[send] Cannot send, channel is closed!\n");
//        assert(0 && "send to closed channel");
        return;
    }

    while (ch->count == ch->capacity && ch->state == FL_CH_OPEN) {
        assert(ch->waiting_sender == NULL);
        ch->waiting_sender = e->current;

        pthread_mutex_unlock(&ch->lock);
        fl_fiber_yield();

        // если за время прокрутки файберов канал стал невалидным
        if (!ch) {
            printf("[send] Channel has been destroyed while fiber scrolling");
            return;
        }
        pthread_mutex_lock(&ch->lock);

        ch->waiting_sender = NULL;
    }

    if (ch->state != FL_CH_OPEN) {
        pthread_mutex_unlock(&ch->lock);
        printf("[send] Cannot send, channel is closed!\n");
//        assert(0 && "send failed, channel not open");
    }

    ch->buffer[ch->tail] = value;
    ch->tail = (ch->tail + 1) % ch->capacity;
    ch->count++;

    pthread_cond_signal(&ch->can_recv);

    pthread_mutex_unlock(&ch->lock);
}

fl_value_t fl_chan_recv(fl_channel* ch) {
    fl_executor* e = g_exec_tls;
    assert(e && e->current);
    assert(e->current->state == FL_RUNNING);

    pthread_mutex_lock(&ch->lock);

    if (ch->state == FL_CH_CLOSED) {
        pthread_mutex_unlock(&ch->lock);
        printf("[receive] Cannot send, channel is closed!\n");
    }

    while (ch->count == 0 && ch->state == FL_CH_OPEN) {
        assert(ch->waiting_receiver == NULL);
        ch->waiting_receiver = e->current;

        pthread_mutex_unlock(&ch->lock);
        fl_fiber_yield();

        if (!ch) {
            printf("[receive] Channel has been destroyed while fiber scrolling");
        }
        // Добавить проверку на то, что канал после всей прокрутки файберов еще жив
        pthread_mutex_lock(&ch->lock);
        ch->waiting_receiver = NULL;
    }

    if (ch->count == 0 && ch->state == FL_CH_CLOSED) {
        pthread_mutex_unlock(&ch->lock);
        return (fl_value_t){FL_TYPE_NONE, {0}};
    }

    // добавить проверки на то, что канал еще не закрыт или не разрушен
    fl_value_t val = ch->buffer[ch->head];
    ch->head = (ch->head + 1) % ch->capacity;
    ch->count--;

    pthread_cond_signal(&ch->can_send);
    pthread_mutex_unlock(&ch->lock);

    return val;
}