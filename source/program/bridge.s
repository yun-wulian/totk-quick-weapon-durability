// SPDX-License-Identifier: GPL-2.0-only
.section .text.quick_durability_bridge,"ax",%progbits
.balign 4
.global quick_durability_bridge
.hidden quick_durability_bridge
.type quick_durability_bridge,%function
quick_durability_bridge:
    // x0..x4 are the unchanged LayoutEx::setMessageString arguments.
    // At this one callsite, x19 is the slide list and x29 its caller's frame.
    stp x29, x30, [sp, #-16]!
    mov x5, x19
    mov x6, x29
    mov x29, sp
    bl quick_durability_set_name
    ldp x29, x30, [sp], #16
    ret
.size quick_durability_bridge, .-quick_durability_bridge
