.macro ISR_NOERRCODE num
.global isr_\num
isr_\num:
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    push %ss
    mov %esp, %eax
    push %eax
    xor %eax, %eax
    movb $\num, %al
    push %eax
    call _ZN16InterruptManager15HandleInterruptEhj
    add $8, %esp
    mov %eax, %esp
    pop %ss
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    iret
.endm

ISR_NOERRCODE 32
ISR_NOERRCODE 33
ISR_NOERRCODE 34
ISR_NOERRCODE 35
ISR_NOERRCODE 36
ISR_NOERRCODE 37
ISR_NOERRCODE 38
ISR_NOERRCODE 39
ISR_NOERRCODE 40
ISR_NOERRCODE 41
ISR_NOERRCODE 42
ISR_NOERRCODE 43
ISR_NOERRCODE 44
ISR_NOERRCODE 45
ISR_NOERRCODE 46
ISR_NOERRCODE 47

.global ignore_interrupt_request
ignore_interrupt_request:
    pusha
    popa
    iret
