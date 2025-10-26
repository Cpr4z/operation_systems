#include "Fiber.hpp"

void Fiber::run() {
    m_impl->run();
}

void Fiber::stop() {
    m_impl->stop();
}

void Fiber::resume() {
    m_impl->resume();
}

void Fiber::yield() {
    m_impl->yield();
}

FiberState Fiber::getState() {
    return m_impl->getState();
}