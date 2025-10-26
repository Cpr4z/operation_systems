#include "ContextSwitcher.hpp"

extern "C" {
void saveContext(void* ctx, void* stackPtr) {
    __asm__ __volatile__ (
        // сохраняем регистры x0–x30
            "stp x0, x1, [sp, -16]!;"
            "stp x2, x3, [sp, -16]!;"
            "stp x4, x5, [sp, -16]!;"
            "stp x6, x7, [sp, -16]!;"
            "stp x8, x9, [sp, -16]!;"
            "stp x10, x11, [sp, -16]!;"
            "stp x12, x13, [sp, -16]!;"
            "stp x14, x15, [sp, -16]!;"
            "stp x16, x17, [sp, -16]!;"
            "stp x18, x19, [sp, -16]!;"
            "stp x20, x21, [sp, -16]!;"
            "stp x22, x23, [sp, -16]!;"
            "stp x24, x25, [sp, -16]!;"
            "stp x26, x27, [sp, -16]!;"
            "stp x28, x29, [sp, -16]!;"
            "stp x30, lr, [sp, -16]!;"

            // Сохранение sp (stack pointer)
            "mov x1, sp;"
            "str x1, [sp, -8]!;"  // сохраняем sp в контекст

            // Сохранение текущего значения pc (program counter)
            "adr x2, .;"
            "str x2, [sp, -16]!"   // сохраняем pc в контекст

            : "=r"(ctx)  // выводим указатель на контекст
            :           // нет входных аргументов
            : "memory", "x1", "x2"  // указываем использованные регистры
            );

    // сохраняем стек
    __asm__ __volatile__(
            "mov x9, sp\n\t"      // скопировать SP в обычный регистр
            "str x9, [%0]\n\t"    // записать x9 по адресу stackPtr
            :
            : "r"(stackPtr)
            : "memory", "x9"
            );
}

void restoreContext(void* ctx, void* stackPtr) {
    __asm__ __volatile__(
            "ldr x9, [%0]\n\t"    // прочитать сохранённый SP
            "mov sp, x9\n\t"      // записать в SP
            :
            : "r"(stackPtr)
            : "memory", "x9"
            );

    __asm__ __volatile__(
            "ldp x0, x1, [%0, #0];"
            "ldp x2, x3, [%0, #16];"
            "ldp x4, x5, [%0, #32];"
            "ldp x6, x7, [%0, #48];"
            "ldp x8, x9, [%0, #64];"
            "ldp x10, x11, [%0, #80];"
            "ldp x12, x13, [%0, #96];"
            "ldp x14, x15, [%0, #112];"
            "ldp x16, x17, [%0, #128];"
            "ldp x18, x19, [%0, #144];"
            "ldp x20, x21, [%0, #160];"
            "ldp x22, x23, [%0, #176];"
            "ldp x24, x25, [%0, #192];"
            "ldp x26, x27, [%0, #208];"
            "ldp x28, x29, [%0, #224];"
            "ldr x30, [%0, #240];"  // LR
            "ldr x1, [%0, #248];"   // SP
            "mov sp, x1;"
            "ldr x2, [%0, #256];"   // PC
            "br x2;"                // переходим по адресу PC
            :
            : "r"(ctx)
            : "memory", "x1", "x2"
            );
}

};
