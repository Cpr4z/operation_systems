#pragma once

// std
#include <memory>
#include <shared_mutex>
#include <vector>

#include "Fibers/Fiber.hpp"

class Scheduler
{
public:
    template<typename Func>
    requires std::is_invocable_v<Func>
    FiberId createFiber(Func&& func) {
        FiberId id = m_lastId.fetch_add(1, std::memory_order_relaxed);
        auto fiber = std::make_unique<Fiber>(std::forward<Func>(func));
        {
            std::unique_lock lock(m_mutex);
            m_fibers.emplace(id, std::move(fiber));
        }
        return m_lastId;
    }

    template<typename T>
    T getResult(FiberId id) {
        Fiber* fiber = getFiber(id);
        if (!fiber) {
            return {};
        }
        return fiber->getResult<T>();
    }

    template<typename... Args>
    void invokeFiber(FiberId id, Args&&... args) {
        Fiber* fiber = getFiber(id);
        if (!fiber) {
            return;
        }
        fiber->invoke(std::forward<Args>(args)...);
    }

    template<typename... Args>
    void switchFiber(FiberId from, FiberId to, Args&&...args) {
        Fiber* fiber_from = getFiber(from);
        Fiber* fiber_to = getFiber(to);
        if (!fiber_from || !fiber_to) {
            return;
        }
        FiberState state_from = fiber_from->getState();
        FiberState state_to = fiber_to->getState();
        if (state_from != FiberState::running) {
            return;
        }
        if ((state_to != FiberState::stopped) && (state_to != FiberState::created)) {
            return;
        }

        fiber_from->stop();
        if (state_to == FiberState::created) {
            fiber_to->invoke(std::forward<Args>(args)...);
        } else {
            fiber_to->resume();
        }
    }

    Fiber* getFiber(FiberId id);
    void stopFiber(FiberId id);

private:
    std::unordered_map<FiberId, std::unique_ptr<Fiber>> m_fibers;
    std::shared_mutex m_mutex;
    std::atomic<FiberId> m_lastId = 0;
};


