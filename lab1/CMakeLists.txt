cmake_minimum_required(VERSION 3.29)
project(fibers)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(HEADERS
    Channels/Channel.hpp
    Fibers/Fiber.hpp
    Fibers/Context.hpp
    Fibers/FiberState.hpp
    Fibers/IFiber.hpp
    Scheduler/Scheduler.hpp
    Constants/Constants.hpp
    Scheduler/ContextSwitcher.hpp)

set(SOURCES
    Channels/Channel.cpp
    Fibers/Fiber.cpp
    Scheduler/Scheduler.cpp
    Scheduler/ContextSwitcher.cpp)

find_package(Boost REQUIRED COMPONENTS filesystem)

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES} main.cpp)

target_link_libraries( ${PROJECT_NAME} PRIVATE Boost::filesystem)