#include "ContextSwitcher.hpp"

extern "C" {
int saveContext(Context* ctx) {
    int ret = 0;
    __asm__ __volatile__ (
            "stp x19, x20, [%1, #0];"
            "stp x21, x22, [%1, #16];"
            "stp x23, x24, [%1, #32];"
            "stp x25, x26, [%1, #48];"
            "stp x27, x28, [%1, #64];"
            "str x29, [%1, #80];"
            "str x30, [%1, #88];"
            "mov x2, sp;"
            "str x2, [%1, #96];"     // SP
            "adr x3, 1f;"            // PC «сразу после»
            "str x3, [%1, #104];"    // PC
            "mov %w0, wzr;"          // ret = 0
            "b 2f;"
            "1:"
            "mov %w0, #1;"           // возвращаемся сюда при restore -> ret = 1
            "2:"
            : "=&r"(ret)
            : "r"(ctx)
            : "memory", "x2", "x3",
                "x19","x20","x21","x22","x23","x24","x25","x26","x27","x28"
            );
    return ret;
}

void restoreContext(Context* ctx)
{
    __asm__ __volatile__ (
        // восстановление всех сохранённых регистров
            "ldp x19, x20, [%0, #0];\n"
            "ldp x21, x22, [%0, #16];\n"
            "ldp x23, x24, [%0, #32];\n"
            "ldp x25, x26, [%0, #48];\n"
            "ldp x27, x28, [%0, #64];\n"
            "ldr x29, [%0, #80];\n"    // FP (frame pointer)
            "ldr x30, [%0, #88];\n"    // LR (link register)
            "ldr x1,  [%0, #96];\n"    // SP
            "mov sp, x1;\n"
            "ldr x2,  [%0, #104];\n"   // PC
            "br  x2;\n"                // ← безвозвратный прыжок по PC
            :
            : "r"(ctx)
            : "memory", "x1", "x2",
    "x19","x20","x21","x22","x23","x24","x25","x26","x27","x28"
            );

    __builtin_unreachable();
}
};
