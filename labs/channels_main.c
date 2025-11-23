#include "channel.h"
#include "constants.h"
#include "fiber.h"
#include "fl_type.h"

#include <stdio.h>

static int producer_finished_once = 0;

static void small_producer(void* arg) {
    channel_wrapper* ch_wrp = (channel_wrapper*)arg;
    for (int i = 0; i < 10; ++i) {
        printf("[producer %p] trying send %d\n", (void*)fl_fiber_current(), i);

        fl_chan_send(ch_wrp, fl_val_int(i));

        printf("[producer %p] sent %d\n", (void*)fl_fiber_current(), i);

        fl_fiber_yield();
    }

    printf("[producer %p] done\n", (void*)fl_fiber_current());
}

static void consumer(void* arg) {
    channel_wrapper* ch_wrp = (channel_wrapper*)arg;
    while (1) {
        printf("[consumer %p] trying recv\n", (void*)fl_fiber_current());

        fl_value_t v = fl_chan_recv(ch_wrp);

        if (v.type == FL_TYPE_NONE) {
            printf("[consumer %p] channel closed -> exit\n",
                   (void*)fl_fiber_current());
            break;
        }

        printf("[consumer %p] received %lld\n",
               (void*)fl_fiber_current(), v.as.i);

        fl_fiber_yield();
    }
}

int main() {
    fl_executor ex;
    fl_executor_init(&ex);

    channel_wrapper channelWrapper = fl_chan_create(2);
    if (!channelWrapper.alive) {
        printf("Channel creating error\n");
        return -1;
    }

    fl_fiber* prod = fl_fiber_create(&ex, small_producer, &channelWrapper, FIBER_STACK_SIZE);
    fl_fiber* cons = fl_fiber_create(&ex, consumer,       &channelWrapper, FIBER_STACK_SIZE);

    int channel_closed = 0;

    while (1) {
        int alive = 0;

        if (prod && !fl_fiber_finished(prod)) {
            alive = 1;
            fl_fiber_resume(&ex, prod);
        }

        if (cons && !fl_fiber_finished(cons)) {
            alive = 1;
            fl_fiber_resume(&ex, cons);
        }

        int prod_finished = prod && fl_fiber_finished(prod);
        if (prod_finished && !channel_closed) {

            if (!producer_finished_once) {
                producer_finished_once = 1;
                continue;
            }

            printf("[main] producer finished -> closing channel\n");
            fl_chan_close(&channelWrapper);
            channel_closed = 1;
        }

        if (prod && fl_fiber_finished(prod)) {
            fl_fiber_destroy(prod);
            prod = NULL;
        }
        if (cons && fl_fiber_finished(cons)) {
            fl_fiber_destroy(cons);
            cons = NULL;
        }

        if (!alive)
            break;
    }

    fl_chan_destroy(&channelWrapper);
    return 0;
}