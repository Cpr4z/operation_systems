#include "fiber.h"

#include "constants.h"

#include <stdio.h>
#include <assert.h>

// функция переключения контекста
extern void fl_ctx_switch(fl_context*, const fl_context*) __attribute__((noinline));
// функция инициализации контекста и последующего запуска файбера
extern void fl_ctx_make  (fl_context*, void*, void (*)(void)) __attribute__((noinline));

// __thread - создание этой переменной для каждого потока отдельное
// каждому потоку отдельный executor
__thread fl_executor* g_exec_tls = NULL;

// выравниваем по 16-ти байтной границе
static inline void* align16_down(void* p) {
    uintptr_t v = (uintptr_t)p;
    v &= ~((uintptr_t)0xF);
    return (void*)v;
}

// обнуляем executor
void fl_executor_init(fl_executor* e) {
    memset(e, 0, sizeof(*e));
    g_exec_tls = e;
}

fl_fiber* fl_fiber_create(fl_executor* e, fl_fiber_fn fn, void* arg, size_t stack_sz) {
    fl_fiber* f = (fl_fiber*)calloc(1, sizeof(fl_fiber));
    f->stack_size = stack_sz ? stack_sz : FIBER_STACK_SIZE;
    f->stack = malloc(f->stack_size);
    assert(f->stack && "no memory for stack");

    f->entry = fn;
    f->arg   = arg;
    f->state = FL_CREATED;
    f->exec  = e;

    // получаем верхушку стека
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
    assert(f->state != FL_FINISHED && "cannot resume finished fiber");
    assert(f->state != FL_RUNNING && "fiber is already running");

    e->current = f;
    f->state   = FL_RUNNING;
    // приостанавливаем текущий контекст и выполняем контекст то, который был запущен до этого
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
    // приостанавливаем текущий контекст и выполняем контекст то, который был запущен до этого
    fl_ctx_switch(&f->ctx, &e->sched_ctx);
    __asm__ __volatile__("" ::: "memory");
}

void fl_fiber_trampoline(void) {
    fl_executor* e = g_exec_tls;
    fl_fiber* f = e->current;
    f->entry(f->arg);
    f->state = FL_FINISHED;
    e->current = NULL;
    fl_ctx_switch(&f->ctx, &e->sched_ctx);
}

fl_fiber* fl_fiber_current(void) {
    fl_executor* e = g_exec_tls;
    if (!e) return NULL;
    return e->current;
}

int fl_fiber_finished(const fl_fiber* f) { return f->state == FL_FINISHED; }