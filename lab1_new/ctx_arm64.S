// clang -c -arch arm64 ctx_arm64.S -o ctx_arm64.o
.text
.p2align 2

// void fl_ctx_switch(fl_context* old, const fl_context* new)
.globl _fl_ctx_switch
_fl_ctx_switch:
    // x0 = old, x1 = new

    // save callee-saved + fp/lr + sp
    stp x19, x20, [x0, #0]
    stp x21, x22, [x0, #16]
    stp x23, x24, [x0, #32]
    stp x25, x26, [x0, #48]
    stp x27, x28, [x0, #64]
    stp x29, x30, [x0, #80]
    mov x2, sp
    str x2, [x0, #96]        // ← так сохраняем SP

    // restore new
    ldp x19, x20, [x1, #0]
    ldp x21, x22, [x1, #16]
    ldp x23, x24, [x1, #32]
    ldp x25, x26, [x1, #48]
    ldp x27, x28, [x1, #64]
    ldp x29, x30, [x1, #80]
    ldr x2, [x1, #96]
    mov sp, x2               // ← так восстанавливаем SP
    ret

// void fl_ctx_make(fl_context* c, void* stack_top, void (*trampoline)(void))
.globl _fl_ctx_make
_fl_ctx_make:
    // x0 = ctx*, x1 = stack_top, x2 = trampoline
    mov  x3, #0
    stp  x3, x3, [x0, #0]
    stp  x3, x3, [x0, #16]
    stp  x3, x3, [x0, #32]
    stp  x3, x3, [x0, #48]
    stp  x3, x3, [x0, #64]
    stp  x3, x2, [x0, #80]   // lr(x30) = trampoline
    str  x1,     [x0, #96]   // sp      = stack_top (выровнен на 16!)
    ret