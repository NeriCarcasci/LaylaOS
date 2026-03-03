.code32
.section .text
.global _start

_start:
prompt_loop:
    mov $4,      %eax
    mov $1,      %ebx
    mov $prompt, %ecx
    mov $2,      %edx
    int $0x80

    mov $3,        %eax
    mov $0,        %ebx
    mov $line_buf, %ecx
    mov $255,      %edx
    int $0x80

    test %eax, %eax
    je prompt_loop

    mov $line_buf, %esi

    mov $cmd_help, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_help

    mov $cmd_clear, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_clear

    mov $cmd_exit, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_exit

    mov $cmd_ls, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_ls

    mov $cmd_meminfo, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_meminfo

    mov $4,         %eax
    mov $1,         %ebx
    mov $msg_unkn,  %ecx
    mov $msg_unkn_len, %edx
    int $0x80

    jmp prompt_loop

do_help:
    mov $4,          %eax
    mov $1,          %ebx
    mov $msg_help,   %ecx
    mov $msg_help_len, %edx
    int $0x80
    jmp prompt_loop

do_clear:
    mov $4,           %eax
    mov $1,           %ebx
    mov $msg_clear,   %ecx
    mov $msg_clear_len, %edx
    int $0x80
    jmp prompt_loop

do_ls:
    mov $4,         %eax
    mov $1,         %ebx
    mov $msg_ls,    %ecx
    mov $msg_ls_len, %edx
    int $0x80
    jmp prompt_loop

do_meminfo:
    mov $4,            %eax
    mov $1,            %ebx
    mov $msg_meminfo,  %ecx
    mov $msg_meminfo_len, %edx
    int $0x80
    jmp prompt_loop

do_exit:
    mov $4,         %eax
    mov $1,         %ebx
    mov $msg_bye,   %ecx
    mov $msg_bye_len, %edx
    int $0x80
    mov $1,  %eax
    xor %ebx, %ebx
    int $0x80

# strcmp_prefix: compare %edi (literal) against %esi (input)
# returns 0 in %eax if %edi is a prefix of %esi, 1 otherwise
strcmp_prefix:
    push %esi
    push %edi
1:
    movb (%edi), %al
    cmp $0, %al
    je   2f
    movb (%esi), %bl
    cmp %al, %bl
    jne  3f
    inc %esi
    inc %edi
    jmp 1b
2:
    xor %eax, %eax
    pop %edi
    pop %esi
    ret
3:
    mov $1, %eax
    pop %edi
    pop %esi
    ret

.section .data

prompt:      .ascii "> "
cmd_help:    .asciz "help"
cmd_clear:   .asciz "clear"
cmd_exit:    .asciz "exit"
cmd_ls:      .asciz "ls"
cmd_meminfo: .asciz "meminfo"

msg_help:    .ascii "Commands: help clear ls meminfo exit\n"
msg_help_len = . - msg_help

msg_clear:
    .ascii "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
msg_clear_len = . - msg_clear

msg_ls:      .ascii "(ls: no disk attached)\n"
msg_ls_len   = . - msg_ls

msg_meminfo: .ascii "PMM pool: 128 frames (512KB)\n"
msg_meminfo_len = . - msg_meminfo

msg_bye:     .ascii "Goodbye\n"
msg_bye_len  = . - msg_bye

msg_unkn:    .ascii "Unknown command\n"
msg_unkn_len = . - msg_unkn

.section .bss
line_buf: .space 256
