#pragma once

// std
#include <memory>
#include <shared_mutex>
#include <vector>

#include <thread>

#include <csignal>
#include <sys/time.h>

#include "Fibers/Fiber.hpp"

class Scheduler;

//static Scheduler* g_sched = nullptr;
//static IFiber*    g_current = nullptr;

//extern "C" void fiberTickHandler(int, siginfo_t*, void*) {
//    if (!g_sched || !g_current) return;
//    IFiber* f = g_current;
//    if (!f) {
//        return;
//    }
//
//    if (f->getState() == FiberState::running && f->stopRequested()) {
//        // сохраняем контекст файбера и возвращаемся в планировщик
//        if (saveContext(f->context()) == 0) {
//            f->setState(FiberState::stopped);
//            restoreContext(&g_sched->schedCtx());
//        }
//    }
//}

static Scheduler* g_sched = nullptr;
static IFiber*    g_current = nullptr;

class Scheduler
{
public:

    Scheduler() {
        // дать trampoline доступ к ctx планировщика
        FiberWrapper<void(*)()>::setSchedulerCtx(&m_schedCtx);
        initPreemption();
    }

    static Scheduler& instance() { static Scheduler scheduler; return scheduler; }

    template<typename Func>
    requires std::is_invocable_v<Func>
    FiberId createFiber(Func&& func) {
        FiberId id = m_lastId.fetch_add(1, std::memory_order_relaxed);
        auto fiber = std::make_unique<Fiber>(std::forward<Func>(func));
        {
            std::unique_lock lock(m_mutex);
            m_fibers.emplace(id, std::move(fiber));
        }
        return id;
    }

    template<typename T>
    T getResult(FiberId id) {
        IFiber* i_fiber = getFiber(id);
        Fiber* fiber = dynamic_cast<Fiber*>(i_fiber);
        if (!fiber) {
            return {};
        }
        return fiber->getResult<T>();
    }

    template<typename... Args>
    void invokeFiber(FiberId id, Args&&... args) {
        IFiber* fiber = getFiber(id);
        if (!fiber) {
            return;
        }

        if (fiber->getState() == FiberState::created) {
            fiber->prepareForStart();
        }

//        std::thread([this, fiber, args...]() mutable {
//            const int sw = saveContext(&m_schedCtx);
//            if (sw == 0) {
//                restoreContext(fiber->context());
//            } else {
//                std::vector<std::any> params{
//                        std::any(std::forward<Args>(args))...
//                };
//                fiber->invoke(params);
//            }
//            g_current = nullptr;
//        }).detach();

        g_current = fiber;

        const int sw = saveContext(&m_schedCtx);
        if (sw == 0) {
            restoreContext(fiber->context());
        } else {
            std::vector<std::any> params{
                    std::any(std::forward<Args>(args))...
            };
            fiber->invoke(params);
        }
        g_current = nullptr;
    }

    template<typename... Args>
    void switchFiber(FiberId from, FiberId to, Args&&...args) {
        IFiber* fiber_from = getFiber(from);
        IFiber* fiber_to = getFiber(to);
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
        m_currentFiber = nullptr;
    }

    IFiber* getFiber(FiberId id) {
//        std::unique_lock lock(m_mutex);
        const auto it = m_fibers.find(id);
        if (it != m_fibers.end()) {
            Fiber* fiber = it->second.get();
            return fiber->getImpl();
        }
        return nullptr;
//        return it != m_fibers.end() ? it->second.get() : nullptr;
    }

    void stopFiber(FiberId id) {
        IFiber* f = getFiber(id);
        if (!f) return;

        if (f == g_current) {
            raise(SIGALRM);
        }
//        else {
//            f->requestStop();
//        }
    }

    void resumeFiber(FiberId id) {
//        std::unique_lock lock(m_mutex);
        IFiber* fiber = getFiber(id);
        if (!fiber) return;
        if (fiber->getState() != FiberState::stopped) return;

        g_current = fiber;
        if (saveContext(&m_schedCtx) == 0) {
            fiber->setState(FiberState::running);
            restoreContext(fiber->context());
            __builtin_unreachable();
        }

        g_current = nullptr;
    }

    IFiber* currentFiber() const {
        return static_cast<IFiber*>(FiberWrapper<void(*)()>::getCurrentFiberOpaque());
    }

    Context& schedCtx() { return m_schedCtx; }

    void initPreemption();

    Fiber* getCurrentFiber() const { return m_currentFiber; }
    void setCurrentFiber(Fiber* fiber) { m_currentFiber = fiber; }
    Context& schedulerCtx() { return m_schedCtx; }

private:
    std::unordered_map<FiberId, std::unique_ptr<Fiber>> m_fibers;
    std::shared_mutex m_mutex;
    std::atomic<FiberId> m_lastId = 0;
    Context m_schedCtx{};
    Fiber* m_currentFiber = nullptr;
};

//static Scheduler* g_sched = nullptr;
//static IFiber*    g_current = nullptr;

//extern "C" void fiberTickHandler(int, siginfo_t*, void*) {
//    if (!g_sched || !g_current) return;
//    IFiber* f = g_current;
//    if (!f) {
//        return;
//    }
//
//    if (f->getState() == FiberState::running && f->stopRequested()) {
//        // сохраняем контекст файбера и возвращаемся в планировщик
//        if (saveContext(f->context()) == 0) {
//            f->setState(FiberState::stopped);
//            restoreContext(&g_sched->schedCtx());
//        }
//    }
//}


