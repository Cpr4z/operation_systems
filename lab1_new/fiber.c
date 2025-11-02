#include "fiber.h"

#include <stdio.h>
#include <assert.h>

extern void fl_ctx_switch(fl_context*, const fl_context*);
extern void fl_ctx_make  (fl_context*, void*, void (*)(void));

__thread fl_executor* g_exec_tls = NULL;

static inline void* align16_down(void* p) {
    uintptr_t v = (uintptr_t)p;
    v &= ~((uintptr_t)0xF);
    return (void*)v;
}

void fl_executor_init(fl_executor* e) {
    memset(e, 0, sizeof(*e));
    g_exec_tls = e;
}

fl_fiber* fl_fiber_create(fl_executor* e, fl_fiber_fn fn, void* arg, size_t stack_sz) {
    fl_fiber* f = (fl_fiber*)calloc(1, sizeof(fl_fiber));
    f->stack_size = stack_sz ? stack_sz : (64 * 1024);
    f->stack = malloc(f->stack_size);
    assert(f->stack && "no memory for stack");

    f->entry = fn;
    f->arg   = arg;
    f->state = FL_CREATED;
    f->exec  = e;

    void* top = (char*)f->stack + f->stack_size;
    top = align16_down(top);
    fl_ctx_make(&f->ctx, top, fl_fiber_trampoline);
    __asm__ __volatile__("" ::: "memory");
    return f;
}

void fl_fiber_destroy(fl_fiber* f) {
    if (!f) return;
    free(f->stack);
    free(f);
}

void fl_fiber_resume(fl_executor* e, fl_fiber* f) {
    e->current = f;
    f->state   = FL_RUNNING;
    fl_ctx_switch(&e->sched_ctx, &f->ctx);
    __asm__ __volatile__("" ::: "memory");
}

void fl_fiber_yield(void) {
    fl_executor* e = g_exec_tls;
    if (!e) return;
    fl_fiber* f = e->current;
    if (!f) { return; }
    f->state = FL_PAUSED;
    e->current = NULL;
    fl_ctx_switch(&f->ctx, &e->sched_ctx);
    __asm__ __volatile__("" ::: "memory");
}

void fl_fiber_trampoline(void) {
    fl_executor* e = g_exec_tls;
    fl_fiber* f = e->current;
    f->entry(f->arg);
    f->state = FL_STOPPED;
    e->current = NULL;
    fl_ctx_switch(&f->ctx, &e->sched_ctx);
    __builtin_unreachable();
}

int fl_fiber_finished(const fl_fiber* f) { return f->state == FL_STOPPED; }