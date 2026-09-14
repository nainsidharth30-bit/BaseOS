.section .text.boot
.global _start

_start:
    /* ARM64 Header */
    b       real_start
    .word   0
    .quad   0x80000
    .quad   0x0000000002000000
    .quad   0
    .quad   0
    .quad   0
    .quad   0
    .ascii  "ARM\x64"
    .word   0

real_start:
    /* Force x0 to point to DTB at 0x40000000 */
    ldr     x0, =0x40000000
    mov     x19, x0

    /* Initialize Boot Stack */
    ldr     x1, =_stack_top
    mov     sp, x1

    /* Pass DTB pointer to kernel_main(uintptr_t dtb_ptr) */
    mov     x0, x19
    bl      bkernel_main

hang:
    wfe
    b       hang

