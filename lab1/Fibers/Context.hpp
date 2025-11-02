#pragma once

#include <cstdint>

struct alignas(16) Context {
//    uint64_t x[12];
    uint64_t x19, x20, x21, x22, x23, x24, x25, x26, x27;
//    uint64_t fp; // x29
//    uint64_t lr; // x30
//    uint64_t sp; // stack pointer
//    uint64_t pc; // program counter
    uint64_t sp;
    uint64_t pc;
};
