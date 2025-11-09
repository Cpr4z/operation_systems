#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

typedef void (*fl_fiber_fn)(void*);

typedef enum {
    FL_CREATED, // создан
    FL_RUNNING, // запущен
    FL_PAUSED, // остановлен, можно возобновить
    FL_FINISHED // файбер закончил работу, возобновить больше нельзя
} fl_state;

typedef struct fl_context {
    // x19–x28 — callee-saved регистры ARM64:
    // по соглашению вызова их нужно сохранять, если функция их изменяет.
    uint64_t x19, x20;
    uint64_t x21, x22;
    uint64_t x23, x24;
    uint64_t x25, x26;
    uint64_t x27, x28;
    uint64_t fp; // fp (x29) — frame pointer (указатель на текущий фрейм стека)
    uint64_t lr; // lr (x30) — link register (адрес возврата, т.е. куда пойдёт ret)
    uint64_t sp; // sp — указатель стека (stack pointer)
} fl_context;

struct fl_executor;

typedef struct fl_fiber {
    fl_context  ctx;
    void*       stack;
    size_t      stack_size;
    fl_fiber_fn entry; // исполняемая функция
    void*       arg; // аргументы для исполняемой файбером функции
    fl_state    state; // состояние файбера
    struct fl_executor* exec;
} fl_fiber;

typedef struct fl_executor {
    fl_context sched_ctx; // предыдущий контекст до выполнения файбера
    fl_fiber*  current;
} fl_executor;

extern __thread fl_executor* g_exec_tls;

void fl_ctx_switch(fl_context* oldc, const fl_context* newc) __attribute__((noinline));
void fl_ctx_make  (fl_context* c, void* stack_top, void (*trampoline)(void));

void       fl_executor_init(fl_executor* e);
fl_fiber*  fl_fiber_create (fl_executor* e, fl_fiber_fn fn, void* arg, size_t stack_sz);
void       fl_fiber_resume (fl_executor* e, fl_fiber* f);
void       fl_fiber_yield  (void);
void       fl_fiber_destroy(fl_fiber* f);

void fl_fiber_trampoline(void);
int  fl_fiber_finished(const fl_fiber* f);