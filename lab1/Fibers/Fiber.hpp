#pragma once

// std
#include <any>
#include <functional>
#include <iostream>
#include <memory>

// boost
#include <boost/noncopyable.hpp>

#include "Constants/Constants.hpp"
#include "Scheduler/ContextSwitcher.hpp"
#include "IFiber.hpp"

namespace {
    template<typename T>
    struct function_traits;

    template<typename R, typename... Args>
    struct function_traits<R(Args...)>
    {
        using return_type = R;
        using args_tuple = std::tuple<Args...>;
    };

    template<typename R, typename... Args>
    struct function_traits<R(*)(Args...)> : function_traits<R(Args...)> {};

    template<typename R, typename... Args>
    struct function_traits<std::function<R(Args...)>> : function_traits<R(Args...)> {};

    template<typename T>
    struct function_traits : function_traits<decltype(&T::operator())> {};

    template<typename C, typename R, typename... Args>
    struct function_traits<R(C::*)(Args...) const> : function_traits<R(Args...)> {};

    template<typename C, typename R, typename... Args>
    struct function_traits<R(C::*)(Args...)> : function_traits<R(Args...)> {};
}

template<typename Func>
requires std::is_invocable_v<Func>
class FiberWrapper : public IFiber {
private:
    using traits = function_traits<typename std::remove_reference_t<Func>>;
    using ReturnT = typename traits::return_type;
    using ArgsTuple = typename traits::args_tuple;

public:
    explicit FiberWrapper(Func&& f) : m_func(std::forward<Func>(f))
    {
        m_context = new Context;
        m_stack.reset(::operator new(constants::STACK_SIZE * constants::BITS_IN_KILOBYTE,
                std::align_val_t(16)));
    }

    void prepareForStart() override {
        auto* top = static_cast<std::byte*>(m_stack.get())
                    + constants::STACK_SIZE * constants::BITS_IN_KILOBYTE;
        m_context->sp  = reinterpret_cast<uint64_t>(top);
        m_context->pc  = reinterpret_cast<uint64_t>(&FiberWrapper::trampoline);
        // передадим указатель this в x19 (callee-saved) — restoreContext его восстановит
        m_context->x19 = reinterpret_cast<uint64_t>(this);
    }

    ~FiberWrapper() override
    {
        delete m_context;
    }

    void run() override
    {
        // если файбер уже запущен, то ничего не делаем
        if (m_state == FiberState::running) {
            return;
        }

        // если файбер находится в состоянии "создан" или "остановлен", то сохраняем контекст
        if (m_state == FiberState::created || m_state == FiberState::stopped) {
            if (saveContext(m_context) == 0) {
                // сюда зайдём первый раз
                m_state = FiberState::running;
            } else {
                // сюда попадём после resume()
                return; // выходим, контекст уже восстановлен
            }
        }
        try {
            if constexpr (std::is_void_v<ReturnT>) {
                m_func();
            } else {
                m_result = m_func();
            }
        } catch (...) {

        }
        m_state = FiberState::completed;
        saveContext(m_context);
    }

    static void trampoline() {
        register void* self_ptr asm("x19");
        auto* self = reinterpret_cast<FiberWrapper*>(self_ptr);
//        FiberWrapper* self = static_cast<FiberWrapper*>(getCurrentFiberOpaque());
        try {
            if constexpr (std::is_void_v<ReturnT>) {
                self->m_func();
            } else {
                self->m_result = self->m_func();
            }
            self->m_state = FiberState::completed;
        } catch (...) {
            self->m_state = FiberState::completed;
        }
        if (saveContext(self->m_context) == 0) {
            restoreContext(getSchedulerCtx()); // non-returning
        }
        for(;;) {}
    }

    void stop() override {
        m_stopRequested.store(true, std::memory_order_relaxed);
//        if (m_state != FiberState::stopped) {
//            if (saveContext(m_context) == 0) {
//                m_state = FiberState::stopped;
//                return; // выйдем в планировщик
//            }
//        }
    }

    void resume() override {
//        if (m_state != FiberState::stopped) {
//            return;
//        }
//        m_state = FiberState::running;
//        restoreContext(m_context);
    }

    void yield() override {

    }

    static void setCurrentFiberOpaque(void* p) { s_currentFiber = p; }
    static void* getCurrentFiberOpaque()       { return s_currentFiber; }
    static void  setSchedulerCtx(Context* c)   { s_schedCtx = c; }
    static Context* getSchedulerCtx()          { return s_schedCtx; }

    void invoke(std::vector<std::any> args) override {
        callFromAny(args, std::make_index_sequence<std::tuple_size_v<ArgsTuple>>{});
    }

    Context* context()  override       { return m_context; }
    bool stopRequested()  const override { return m_stopRequested.load(std::memory_order_relaxed); }
    void setState(FiberState s) override { m_state = s; }
    [[nodiscard]] FiberState getState() const override { return m_state; }

    [[nodiscard]] FiberId getId() const { return m_id; }

    [[nodiscard]] std::any getResult() const override {
        if constexpr (std::is_void_v<ReturnT>) {
            return {};
        } else {
            return m_result.value();
        }
    }

private:
    template<std::size_t... I>
    void callFromAny(const std::vector<std::any>& args, std::index_sequence<I...>) {
        if constexpr (std::is_void_v<ReturnT>) {
            std::invoke(m_func, std::any_cast<std::tuple_element_t<I, ArgsTuple>>(args[I])...);
        } else {
            m_result = std::invoke(m_func, std::any_cast<std::tuple_element_t<I, ArgsTuple>>(args[I])...);
        }
    }

    using OptionalReturnT = std::conditional_t<
            std::is_void_v<ReturnT>,
            std::monostate,
            ReturnT>;

private:
    Func m_func;
//    void* m_stack = nullptr; // область памяти, для сохранения информации о stack pointer
//    void* m_context = nullptr;
    alignas(16) Context* m_context = nullptr;
//    Context m_context;
    std::unique_ptr<void, void(*)(void*)>   m_stack{nullptr, +[](void* p){ ::operator delete(p, std::align_val_t(16)); }};
    FiberId m_id = 0; // айдишник файбера в контексте планировщика
    std::optional<OptionalReturnT> m_result;
//    std::thread m_parent_thread;
    // добавить родительский поток в котором запущен файбер
    // возможно нужно хранить вообще родительский поток из которого был запущен файбер
    FiberState m_state = FiberState::created;
    std::atomic<bool> m_stopRequested{false};
    static inline void*    s_currentFiber = nullptr;
    static inline Context* s_schedCtx     = nullptr;
};


// нужно добавить функционал обработки исключений при выполнении функции которую мы передали в файбер
class Fiber: public boost::noncopyable  {
public:
    template<typename Func>
    explicit Fiber(Func&& f) : m_impl(std::make_unique<FiberWrapper<Func>>(std::forward<Func>(f))) {}

    void run();
    void stop();
    void resume();
    void yield();

    template<typename T>
    T getResult() const {
        try {
            return std::any_cast<T>(m_impl->getResult());
        } catch (const std::bad_any_cast& e) {
            std::cout << e.what() << std::endl;
        }
        return {};
    }

    template<typename... Args>
    void invoke(Args&&... args) {
        std::vector<std::any> params{
            std::any(std::forward<Args>(args))...
        };
        m_impl->invoke(params);
    }

    FiberState getState();
    IFiber* getImpl() const { return m_impl.get(); };
private:
    std::unique_ptr<IFiber> m_impl;
};


