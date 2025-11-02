#pragma once

// std
#include <any>

#include "FiberState.hpp"

using FiberId = size_t;

class IFiber {
public:
    virtual ~IFiber() = default;

    virtual void run() = 0;
    virtual void stop() = 0;
    virtual void resume() = 0;
    virtual void yield() = 0;
    virtual void invoke(std::vector<std::any> args) = 0;
    [[nodiscard]] virtual FiberState getState() const = 0;
    [[nodiscard]] virtual std::any getResult() const = 0;

    virtual Context*      context() = 0;          // нужен планировщику для restoreContext
    virtual void          prepareForStart() = 0;  // инициализация SP/PC (+передача self)
    virtual void          setState(FiberState s) = 0;
    virtual bool          stopRequested() const = 0;
};
