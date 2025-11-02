#include "fiber.h"
#include <stdio.h>
#include <unistd.h>

typedef struct {
    long start;
    long end;
    long partial_sum;
} heavy_args;

static void heavy_calc(void* arg) {
    heavy_args* a = (heavy_args*)arg;
    long sum = a->partial_sum;

    for (long i = a->start; i < a->end; ++i) {
        sum += i;

        if (i % 1000000 == 0) {
            printf("[fiber] reached %ld, partial sum=%ld\n", i, sum);
            a->start = i + 1;
            a->partial_sum = sum;

            fl_fiber_yield();
        }
    }

    a->partial_sum = sum;
    printf("[fiber] finished, result=%ld\n", sum);
}

int main() {
    fl_executor ex;
    fl_executor_init(&ex);

    heavy_args args = {.start = 0, .end = 5e6, .partial_sum = 0};
    fl_fiber* f = fl_fiber_create(&ex, heavy_calc, &args, 128 * 1024);

    printf("=== Heavy computation with pause/resume ===\n");

    for (int step = 0; step < 10; ++step) {
        if (f && !fl_fiber_finished(f)) {
            printf("\n[main] Resume fiber (step %d)\n", step);
            fl_fiber_resume(&ex, f);
        } else {
            break;
        }
        usleep(300000);
    }

    if (f && fl_fiber_finished(f)) {
        printf("\n[main] Fiber done! Final sum=%ld\n", args.partial_sum);
        fl_fiber_destroy(f);
    }

    return 0;
}
