#include "channel.h"
#include "fiber.h"
#include "constants.h"
#include "fl_type.h"

#include <stdio.h>

const static int BUFFER_SIZE = 5;

void broken_producer(void* args) {
    channel_wrapper* ch_wrp = (channel_wrapper*)args;

    for (int i = 0; i < BUFFER_SIZE; ++i) {
        if (i == 3) {
            fl_fiber_destroy(fl_fiber_current());
            continue;
        }
        printf("[producer %p] trying send %d\n", (void*)fl_fiber_current(), i);
        fl_chan_send(ch_wrp, fl_val_int(i));
        printf("[producer %p] sent %d\n", (void*)fl_fiber_current(), i);
    }

}

void broken_consumer(void* args) {
    channel_wrapper* ch_wrp = (channel_wrapper*)args;
    while (1) {
        printf("[consumer %p] trying recv\n", (void*)fl_fiber_current());
        fl_value_t v = fl_chan_recv(ch_wrp);

        if (v.type == FL_TYPE_NONE) {
            printf("[consumer %p] channel closed -> exit\n", (void*)fl_fiber_current());
            break;
        }

        printf("[consumer %p] received %lld\n", (void*)fl_fiber_current(), v.as.i);
        fl_fiber_yield();
    }

    printf("[consumer %p] TRY recv after close (should return NONE)\n", (void*)fl_fiber_current());
    fl_value_t v = fl_chan_recv(ch_wrp);
    if (v.type == FL_TYPE_NONE) {
        printf("[consumer %p] OK: returned NONE after close\n", (void*)fl_fiber_current());
        return;
    }
}

int main() {
    fl_executor ex;
    fl_executor_init(&ex);
    channel_wrapper channelWrapper = fl_chan_create(BUFFER_SIZE);
    if (!channelWrapper.alive) {
        printf("Channel creating error\n");
        return -1;
    }

    fl_fiber* broken_prod = fl_fiber_create(&ex, broken_producer, &channelWrapper, FIBER_STACK_SIZE);
    fl_fiber* broken_cons = fl_fiber_create(&ex, broken_consumer, &channelWrapper, FIBER_STACK_SIZE);

    fl_fiber* all[] = { broken_prod, broken_cons };

    while (1) {
        int alive = 0;

        for (int i = 0; i < 2; ++i) {
            if (all[i] && !fl_fiber_finished(all[i])) {
                alive = 1;
                fl_fiber_resume(&ex, all[i]);
            }
        }

        for (int i = 0; i < 2; ++i) {
            if (all[i] && fl_fiber_finished(all[i])) {
                fl_fiber_destroy(all[i]);
                all[i] = NULL;
            }
        }

        if (!alive) break;
    }


    return 0;
}