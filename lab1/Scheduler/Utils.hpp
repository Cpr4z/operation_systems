#pragma once

//#include <mach/mach.h>

#include <csignal>
#include <sys/time.h>

#include <Fibers/Fiber.hpp>
#include "Scheduler.hpp"

static Fiber*     g_currentFiber = nullptr;
static Scheduler* g_scheduler    = nullptr;

extern "C" void fiberTickHandler(int) {
    Fiber* f = g_currentFiber;
    Scheduler* sch = g_scheduler;
    if (!f || !sch) return;

    // Прерываем только если сейчас этот файбер реально "бежит" и есть запрос на стоп
    if (f->getState() == FiberState::running && f->stopRequested()) {
        // Сохраняем контекст файбера и переносим исполнение в планировщик
        if (saveContext(&f->ctx()) == 0) {
            f->setState(FiberState::stopped);
            restoreContext(&sch->schedulerCtx());
        }
        // Если ret == 1 — это возврат сюда после возобновления; ничего делать не нужно.
    }
}

void initFiberPreemption() {
    struct sigaction sa{};
    sa.sa_handler = fiberTickHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &sa, nullptr);

    itimerval timer{};
    timer.it_interval.tv_usec = 1000;  // 1 мс период
    timer.it_value.tv_usec = 1000;
    setitimer(ITIMER_REAL, &timer, nullptr);
}
