#include "Scheduler.hpp"

//static Scheduler* g_sched = nullptr;
//static IFiber*    g_current = nullptr;

extern "C" void fiberTickHandler(int, siginfo_t*, void*) {
    if (!g_sched || !g_current)
        return;

    IFiber* f = g_current;
    if (!f)
        return;

    // если файбер запущен и есть запрос на остановку
    if (f->getState() == FiberState::running && f->stopRequested()) {
        // сохраняем контекст файбера и возвращаемся в планировщик
        if (saveContext(f->context()) == 0) {
            f->setState(FiberState::stopped);
            restoreContext(&g_sched->schedCtx());
        }
    }
}

void Scheduler::initPreemption() {
    g_sched = this;

    struct sigaction sa{};
    //        sa.sa_sigaction = &Scheduler::onSignal; // используем sa_sigaction
    sa.sa_sigaction = fiberTickHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGALRM, &sa, nullptr);

    itimerval t{};
    t.it_interval.tv_usec = 1000; // 1 мс
    t.it_value.tv_usec    = 1000;
    setitimer(ITIMER_REAL, &t, nullptr);
}

//Fiber* Scheduler::getFiber(FiberId id){
//    std::unique_lock lock(m_mutex);
//    const auto it = m_fibers.find(id);
//    return it != m_fibers.end() ? it->second.get() : nullptr;
//}

//void Scheduler::stopFiber(FiberId id) {
//    std::unique_lock lock(m_mutex);
//    Fiber* fiber = getFiber(id);
//    if (fiber) {
//        fiber->stop();
//    }
//}

//void Scheduler::resumeFiber(FiberId id) {
//    std::unique_lock lock(m_mutex);
//    Fiber* fiber = getFiber(id);
//    if (fiber) {
//        fiber->resume();
//    }
//}