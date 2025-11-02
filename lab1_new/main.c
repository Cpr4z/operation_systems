#include "fiber.h"
#include <stdio.h>

static void worker(void* arg) {
    const char* name = (const char*)arg;
    for (int i = 0; i < 3; ++i) {
        printf("[%s] step %d (tid %zu)\n", name, i, (size_t)pthread_self());
        fl_fiber_yield();
    }
}

int main() {
    fl_executor ex;
    fl_executor_init(&ex);

    fl_fiber* a = fl_fiber_create(&ex, worker, "A", 64*1024);
    fl_fiber* b = fl_fiber_create(&ex, worker, "B", 64*1024);

    for (;;) {
        int alive = 0;
        if (a && !fl_fiber_finished(a)) { alive = 1; fl_fiber_resume(&ex, a); }
        if (b && !fl_fiber_finished(b)) { alive = 1; fl_fiber_resume(&ex, b); }

        if (a && fl_fiber_finished(a)) { fl_fiber_destroy(a); a = NULL; }
        if (b && fl_fiber_finished(b)) { fl_fiber_destroy(b); b = NULL; }
        if (!alive) break;
    }
    return 0;
}