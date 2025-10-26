#pragma once

#include <functional>
#include <memory>
#include <future>
#include <any>

#include <iostream>

#include <boost/noncopyable.hpp>

#include <Constants/Constants.hpp>

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
public:
    using traits = function_traits<typename std::remove_reference_t<Func>>;
    using ReturnT = typename traits::return_type;
    using ArgsTuple = typename traits::args_tuple;

    explicit FiberWrapper(Func&& f) : m_func(std::forward<Func>(f))
    {
        size_t aligned_size = ((sizeof(Context) + 15) / 16) * 16;
        m_context = std::aligned_alloc(16, aligned_size);
        m_stack = malloc(constants::STACK_SIZE * constants::BITS_IN_KILOBYTE);
    }

    ~FiberWrapper() override
    {
        free(m_stack);
        free(m_context);
    }

    void run() override
    {
        if (m_state == FiberState::running) {
            return;
        }

        if (m_state == FiberState::created || m_state == FiberState::stopped) {
            saveContext(m_stack, m_context);
        }
        if constexpr (std::is_void_v<ReturnT>) {
            m_func();
        } else {
            m_result = m_func();
        }
        m_state = FiberState::completed;
        saveContext(m_stack, m_context);
    }

    void stop() override {
        if (m_state != FiberState::stopped) {
            saveContext(m_stack, m_context);
            m_state = FiberState::stopped;
        }
    }

    void resume() override {
        if (m_state != FiberState::stopped) {
            return;
        }
        m_state = FiberState::running;
        restoreContext(m_stack, m_context);
    }

    void yield() override {

    }

    void invoke(std::vector<std::any> args) override {
        callFromAny(args, std::make_index_sequence<std::tuple_size_v<ArgsTuple>>{});
    }

    [[nodiscard]] FiberState getState() const override {
        return m_state;
    }

    [[nodiscard]] FiberId getId() const { return m_id; }

    std::any getResult() const override {
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
    void* m_stack = nullptr; // область памяти, для сохранения информации о stack pointer
    void* m_context = nullptr;
    FiberId m_id = 0; // айдишник файбера в контексте планировщика
    std::optional<OptionalReturnT> m_result;
//    std::thread m_parent_thread;
    // добавить родительский поток в котором запущен файбер
    // возможно нужно хранить вообще родительский поток из которого был запущен файбер
    FiberState m_state = FiberState::created;
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
    T getResult() {
        try {
            return std::any_cast<T>(m_impl->getResult());
        } catch (const std::bad_any_cast& e) {
            std::cout << e.what() << std::endl;
        }
    }

    template<typename... Args>
    void invoke(Args&&... args) {
        std::vector<std::any> params{
            std::any(std::forward<Args>(args))...
        };
        m_impl->invoke(params);
    }

    FiberState getState();
private:
    std::unique_ptr<IFiber> m_impl;
};


