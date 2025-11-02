#include "fiber.h"
#include "channel.h"
#include "fl_type.h"
#include <stdio.h>

static void producer(void* arg) {
    fl_channel* ch = (fl_channel*)arg;
    for (int i = 0; i < 5; ++i) {
        printf("[producer] send %d\n", i);
        fl_chan_send(ch, fl_val_int(i));
        fl_fiber_yield();
    }
    fl_chan_close(ch);
}

static void consumer(void* arg) {
    fl_channel* ch = (fl_channel*)arg;
    while (1) {
        fl_value_t v = fl_chan_recv(ch);
        if (v.type == FL_TYPE_NONE) break;
        switch (v.type) {
            case FL_TYPE_INT:    printf("[cons] int: %lld\n", v.as.i); break;
            case FL_TYPE_DOUBLE: printf("[cons] dbl: %.2f\n", v.as.d); break;
            case FL_TYPE_PTR:    printf("[cons] str: %s\n", (char*)v.as.p); break;
            default: break;
        }
    }
}

int main() {
    fl_executor ex;
    fl_executor_init(&ex);

    fl_channel* ch = fl_chan_create(2);

    fl_fiber* prod = fl_fiber_create(&ex, producer, ch, 64*1024);
    fl_fiber* cons = fl_fiber_create(&ex, consumer, ch, 64*1024);

    while (1) {
        int alive = 0;
        if (cons && !fl_fiber_finished(cons)) { alive = 1; fl_fiber_resume(&ex, cons); }
        if (prod && !fl_fiber_finished(prod)) { alive = 1; fl_fiber_resume(&ex, prod); }

        if (prod && fl_fiber_finished(prod)) { fl_fiber_destroy(prod); prod = NULL; }
        if (cons && fl_fiber_finished(cons)) { fl_fiber_destroy(cons); cons = NULL; }

        if (!alive) break;
    }

    fl_chan_destroy(ch);
    return 0;
}