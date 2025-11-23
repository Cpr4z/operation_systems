#include "channel.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

//fl_channel* fl_chan_create(size_t capacity) {
//    fl_channel* ch = calloc(1, sizeof(fl_channel));
//    if (!ch) {
//        return NULL;
//    }
//
//    ch->buffer = calloc(capacity, sizeof(fl_value_t));
//    if (!ch->buffer) {
//        free(ch);
//        return NULL;
//    }
//
//    ch->capacity = capacity;
//    ch->head = 0;
//    ch->tail = 0;
//    ch->count = 0;
//    ch->waiting_sender = NULL;
//    ch->waiting_receiver = NULL;
//    ch->state = FL_CH_OPEN;
//
//    pthread_mutex_init(&ch->lock, NULL);
//    pthread_cond_init(&ch->can_send, NULL);
//    pthread_cond_init(&ch->can_recv, NULL);
//
//    return ch;
//}

channel_wrapper fl_chan_create(size_t capacity) {
    channel_wrapper wrapper;
    wrapper.alive = 0;
    fl_channel* ch = calloc(1, sizeof(fl_channel));
    if (!ch) {
        return wrapper;
    }

    ch->buffer = calloc(capacity, sizeof(fl_value_t));
    if (!ch->buffer) {
        free(ch);
        return wrapper;
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
    wrapper.channel = ch;
    wrapper.alive = 1;

    return wrapper;
}

void fl_chan_destroy(channel_wrapper* ch_wrp) {
    if (!ch_wrp || !ch_wrp->alive || !ch_wrp->channel) {
        return;
    }

    fl_channel* ch = ch_wrp->channel;

    pthread_mutex_destroy(&ch->lock);
    pthread_cond_destroy(&ch->can_send);
    pthread_cond_destroy(&ch->can_recv);

    free(ch->buffer);
    free(ch);

    ch_wrp->alive = 0;
}

void fl_chan_close(channel_wrapper* ch_wrp) {
    if (!ch_wrp || !ch_wrp->alive || !ch_wrp->channel) {
        return;
    }

    fl_channel* ch = ch_wrp->channel;

    pthread_mutex_lock(&ch->lock);
    ch->state = FL_CH_CLOSED;

    pthread_cond_broadcast(&ch->can_send);
    pthread_cond_broadcast(&ch->can_recv);
    pthread_mutex_unlock(&ch->lock);
}

void fl_chan_send(channel_wrapper* ch_wrp, fl_value_t value) {
    if (!ch_wrp || !ch_wrp->alive || !ch_wrp->channel) {
        return;
    }

    fl_channel* ch = ch_wrp->channel;

    fl_executor* e = g_exec_tls;
    assert(e && e->current);
    assert(e->current->state == FL_RUNNING);

    pthread_mutex_lock(&ch->lock);

    if (ch->state == FL_CH_CLOSED) {
        pthread_mutex_unlock(&ch->lock);
        printf("[send] Cannot send, channel is closed!\n");
        return;
    }

    while (ch->count == ch->capacity && ch->state == FL_CH_OPEN) {
        assert(ch->waiting_sender == NULL);
        ch->waiting_sender = e->current;

        pthread_mutex_unlock(&ch->lock);
        fl_fiber_yield();

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
        return;
    }

    ch->buffer[ch->tail] = value;
    ch->tail = (ch->tail + 1) % ch->capacity;
    ch->count++;

    pthread_cond_signal(&ch->can_recv);

    pthread_mutex_unlock(&ch->lock);
}

fl_value_t fl_chan_recv(channel_wrapper* ch_wrp) {
    if (!ch_wrp || !ch_wrp->alive || !ch_wrp->channel) {
        return (fl_value_t){FL_TYPE_NONE, {0}};
    }

    fl_channel* ch = ch_wrp->channel;

    fl_executor* e = g_exec_tls;
    assert(e && e->current);
    assert(e->current->state == FL_RUNNING);

    pthread_mutex_lock(&ch->lock);

    if (ch->state == FL_CH_CLOSED) {
        pthread_mutex_unlock(&ch->lock);
        printf("[receive] Cannot send, channel is closed!\n");
        return (fl_value_t){FL_TYPE_NONE, {0}};
    }

    while (ch->count == 0 && ch->state == FL_CH_OPEN) {
        assert(ch->waiting_receiver == NULL);
        ch->waiting_receiver = e->current;

        pthread_mutex_unlock(&ch->lock);
        fl_fiber_yield();

        if (!ch) {
            printf("[receive] Channel has been destroyed while fiber scrolling");
            return (fl_value_t){FL_TYPE_NONE, {0}};
        }
        pthread_mutex_lock(&ch->lock);
        ch->waiting_receiver = NULL;
    }

    if (ch->count == 0 && ch->state == FL_CH_CLOSED) {
        pthread_mutex_unlock(&ch->lock);
        return (fl_value_t){FL_TYPE_NONE, {0}};
    }

    fl_value_t val = ch->buffer[ch->head];
    ch->head = (ch->head + 1) % ch->capacity;
    ch->count--;

    pthread_cond_signal(&ch->can_send);
    pthread_mutex_unlock(&ch->lock);

    return val;
}
