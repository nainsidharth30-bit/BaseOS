ENTRY(_start)

SECTIONS
{
    /* Safe memory pool start for BaseOS */
    . = 0x41400000;

    _kernel_start = .;

    .text : {
        KEEP(*(.text.boot))
        *(.text*)
    }

    .rodata : { 
        *(.rodata*) 
    }

    .data : { 
        *(.data*) 
    }

    .bss (NOLOAD) : {
        . = ALIGN(16);
        _bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(16);
        _bss_end = .;
    }

    /* Isolated Boot Stack (16 KB) */
    .stack (NOLOAD) : {
        . = ALIGN(16);
        . += 0x4000;
        _stack_top = .;
    }

    . = ALIGN(4096);
    _kernel_end = .;

    /* Guarantees binary won't spill into DMA pool (0x42000000) or Framebuffer (0x5E000000) */
    ASSERT(_kernel_end < 0x42000000, "CRITICAL BUILD ERROR: BaseOS image exceeds allocated 12 MB region!")
}