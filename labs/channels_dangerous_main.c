#include "channel.h"
#include "fiber.h"
#include "constants.h"
#include "fl_type.h"

#include <stdio.h>

const static int BUFFER_SIZE = 10;

static void dangerous_producer(void* arg) {
    fl_channel* ch = (fl_channel*)arg;

    for (int i = 0; i < BUFFER_SIZE; ++i) {
        printf("[producer %p] trying send %d\n", (void*)fl_fiber_current(), i);
        fl_chan_send(ch, fl_val_int(i));
        printf("[producer %p] sent %d\n", (void*)fl_fiber_current(), i);
    }

    printf("[producer %p] closing channel\n", (void*)fl_fiber_current());
    fl_chan_close(ch);

    printf("[producer %p] TRY send after close (should fail)\n", (void*)fl_fiber_current());
    fl_chan_send(ch, fl_val_int(777));
}

static void dangerous_consumer(void* arg) {
    fl_channel* ch = (fl_channel*)arg;

    while (1) {
        printf("[consumer %p] trying recv\n", (void*)fl_fiber_current());
        fl_value_t v = fl_chan_recv(ch);

        if (v.type == FL_TYPE_NONE) {
            printf("[consumer %p] channel closed -> exit\n", (void*)fl_fiber_current());
            break;
        }

        printf("[consumer %p] received %lld\n", (void*)fl_fiber_current(), v.as.i);
        fl_fiber_yield();
    }

    printf("[consumer %p] TRY recv after close (should return NONE)\n", (void*)fl_fiber_current());
    fl_value_t v = fl_chan_recv(ch);
    if (v.type == FL_TYPE_NONE) {
        printf("[consumer %p] OK: returned NONE after close\n", (void*)fl_fiber_current());
    }
}

int main() {
    fl_executor ex;
    fl_executor_init(&ex);

    fl_channel* channel = fl_chan_create(BUFFER_SIZE);

    fl_fiber* dangerous_prod = fl_fiber_create(&ex, dangerous_producer, channel, FIBER_STACK_SIZE);
    fl_fiber* dangerous_cons = fl_fiber_create(&ex, dangerous_consumer, channel, FIBER_STACK_SIZE);

    fl_fiber* all[] = { dangerous_prod, dangerous_cons };

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