// Modified 2026-10-25: use module-only startup with the game's rtld.
.section ".text.crt0","ax"

.global __module_start
__module_start:
    // Module-only build: the game's rtld performs relocation and calls DT_INIT.
    b exl_module_init
    .word __nx_mod0 - __module_start
.section ".rodata.mod0","a"

.hidden exl_nx_module_runtime 

.align 2
__nx_mod0:
    .ascii "MOD0"
    .word  __dynamic_start__        - __nx_mod0
    .word  __bss_start__            - __nx_mod0
    .word  __bss_end__              - __nx_mod0
    .word  __eh_frame_hdr_start__   - __nx_mod0
    .word  __eh_frame_hdr_end__     - __nx_mod0
    .word  exl_nx_module_runtime    - __nx_mod0
