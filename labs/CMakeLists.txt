cmake_minimum_required(VERSION 3.29)

project(fibers_arm64 C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_OSX_ARCHITECTURES "arm64")

enable_language(ASM)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lpthread")

add_executable(fibers_arm64
        fiber.c
        constants.h
        main.c
        ctx_arm64.S
)

add_executable(fibers_stop_arm64
        fiber.c
        heavy_calc.c
        constants.h
        ctx_arm64.S
)

add_executable(fibers_channels
        fiber.c
        channel.c
        channels_main.c
        fl_type.h
        constants.h
        ctx_arm64.S
)

add_executable(fiber_channels_overflow
        fiber.c
        channel.c
        channels_main_overflow.c
        fl_type.h
        constants.h
        ctx_arm64.S
)

add_executable(fibers_dangerous
        fiber.c
        channel.c
        channels_dangerous_main.c
        fl_type.h
        constants.h
        ctx_arm64.S
)

add_executable(channels_broken_fiber
        fiber.c
        channel.c
        main_channels_broken_fiber.c
        fl_type.h
        constants.h
        ctx_arm64.S
)
