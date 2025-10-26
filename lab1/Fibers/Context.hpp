#pragma once

#include <cstdint>

struct Context {
    uint64_t x[31];
    uint64_t sp;
    uint64_t pc;
    uint64_t lr;
};
