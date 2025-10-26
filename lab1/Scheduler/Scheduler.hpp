#pragma once

#include <memory>
#include <vector>

#include "Fibers/Fiber.hpp"

class Scheduler
{
public:
    void addFiber(std::unique_ptr<Fiber>&& fiber) {
        m_fibers.emplace_back(std::move(fiber));
    }

    void switchFiber(std::unique_ptr<Fiber>& next_fiber) {
        if (next_fiber) {
            m_current_fiber = std::move(next_fiber);
            m_current_fiber->run();
        }
    }

//    void resumeAll() {
//
//    }

    void run() {
        for (auto& fiber: m_fibers) {
            switchFiber(fiber);
        }
    }

    const std::unique_ptr<Fiber>& getCurrentFiber() const { return m_current_fiber;}

    void resumeCurrentFiber() {
        if (m_current_fiber->getState() == FiberState::stopped) {
            m_current_fiber->resume();
        }
    }

private:
    std::vector<std::unique_ptr<Fiber>> m_fibers;
    std::unique_ptr<Fiber> m_current_fiber;
};


