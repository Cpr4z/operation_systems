#include "Scheduler.hpp"

Fiber* Scheduler::getFiber(FiberId id){
    std::unique_lock lock(m_mutex);
    const auto it = m_fibers.find(id);
    return it != m_fibers.end() ? it->second.get() : nullptr;
}

void Scheduler::stopFiber(FiberId id) {
    std::unique_lock lock(m_mutex);
    Fiber* fiber = getFiber(id);
    if (fiber) {
        fiber->stop();
    }
}