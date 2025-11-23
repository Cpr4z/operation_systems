#include "channel.h"
#include "fiber.h"
#include "constants.h"
#include "fl_type.h"

#include <stdio.h>

const static int BUFFER_SIZE = 5;

static void producer(void* arg) {
    channel_wrapper* ch_wrp = (channel_wrapper*)arg;
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        printf("[producer %p] trying send %d\n", (void*)fl_fiber_current(), i);

        fl_chan_send(ch_wrp, fl_val_int(i));

        printf("[producer %p] sent %d\n", (void*)fl_fiber_current(), i);
    }

    fl_fiber_yield();

    printf("[producer %p] done\n", (void*)fl_fiber_current());
    fl_chan_close(ch_wrp);
}

static void producer_overflow(void* arg) {
    channel_wrapper* ch_wrp = (channel_wrapper*)arg;
    for (int i = 0; i < BUFFER_SIZE + 1; ++i) {
        printf("[producer %p] trying send %d\n", (void*)fl_fiber_current(), i);

        fl_chan_send(ch_wrp, fl_val_int(i));

        printf("[producer %p] sent %d\n", (void*)fl_fiber_current(), i);
    }

//    fl_fiber_yield();
    printf("[producer %p] done\n", (void*)fl_fiber_current());
    fl_chan_close(ch_wrp);
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

        printf("[consumer %p] received %lld\n", (void*)fl_fiber_current(), v.as.i);

        fl_fiber_yield();
    }
}

int main() {
    fl_executor ex;
    fl_executor_init(&ex);
    {
        channel_wrapper channelWrapper = fl_chan_create(BUFFER_SIZE);
        if (!channelWrapper.alive) {
            printf("Channel creating error\n");
            return -1;
        }

        fl_fiber* prod = fl_fiber_create(&ex, producer, &channelWrapper, FIBER_STACK_SIZE);
        fl_fiber* cons = fl_fiber_create(&ex, consumer, &channelWrapper, FIBER_STACK_SIZE);

        fl_fiber* all[] = { prod, cons };

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

        fl_chan_destroy(&channelWrapper);
    }

    printf("===============================================================\n");

    {
        channel_wrapper channelWrapper = fl_chan_create(BUFFER_SIZE);
        if (!channelWrapper.alive) {
            printf("Channel creating error\n");
            return -1;
        }

        fl_fiber* prod = fl_fiber_create(&ex, producer_overflow, &channelWrapper, FIBER_STACK_SIZE);
        fl_fiber* cons = fl_fiber_create(&ex, consumer, &channelWrapper, FIBER_STACK_SIZE);

        fl_fiber* all[] = { prod, cons };

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

        fl_chan_destroy(&channelWrapper);
    }

    return 0;
}