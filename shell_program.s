.code32
.section .text
.global _start

_start:
prompt_loop:
    mov $4,      %eax
    mov $1,      %ebx
    mov $prompt, %ecx
    mov $7,      %edx
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

    mov $cmd_uname, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_uname

    mov $cmd_whoami, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_whoami

    mov $cmd_reboot, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_reboot

    mov $cmd_echo, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_echo

    mov $cmd_rm, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_rm

    mov $cmd_mkdir, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_mkdir

    mov $cmd_touch, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_touch

    mov $cmd_cp, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_cp

    mov $cmd_mv, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_mv

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
    mov $ls_path, %ebx
    mov $89, %eax
    int $0x80
    jmp prompt_loop

do_meminfo:
    mov $4,            %eax
    mov $1,            %ebx
    mov $msg_meminfo,  %ecx
    mov $msg_meminfo_len, %edx
    int $0x80
    jmp prompt_loop

do_uname:
    mov $4,           %eax
    mov $1,           %ebx
    mov $msg_uname,   %ecx
    mov $msg_uname_len, %edx
    int $0x80
    jmp prompt_loop

do_whoami:
    mov $4,            %eax
    mov $1,            %ebx
    mov $msg_whoami,   %ecx
    mov $msg_whoami_len, %edx
    int $0x80
    jmp prompt_loop

do_reboot:
    mov $88, %eax
    int $0x80
    jmp prompt_loop

do_echo:
    movb line_buf+4, %al
    cmp $' ', %al
    jne do_echo_done
    mov $line_buf+5, %ecx
    xor %edx, %edx
10:
    movb (%ecx,%edx), %al
    cmp $'\n', %al
    je 11f
    cmp $0, %al
    je 11f
    inc %edx
    jmp 10b
11:
    inc %edx
    mov $4, %eax
    mov $1, %ebx
    int $0x80
do_echo_done:
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

# rm <path>  →  syscall 6 (sys_unlink), path at line_buf+3
do_rm:
    mov $line_buf+3, %ebx
    xor %ecx, %ecx
20:
    movb (%ebx,%ecx), %al
    cmp $'\n', %al
    je 21f
    cmp $0, %al
    je 21f
    inc %ecx
    jmp 20b
21:
    movb $0, (%ebx,%ecx)
    mov $6, %eax
    int $0x80
    jmp prompt_loop

# mkdir <dir>  →  syscall 39, path at line_buf+6
do_mkdir:
    mov $line_buf+6, %ebx
    xor %ecx, %ecx
30:
    movb (%ebx,%ecx), %al
    cmp $'\n', %al
    je 31f
    cmp $0, %al
    je 31f
    inc %ecx
    jmp 30b
31:
    movb $0, (%ebx,%ecx)
    mov $39, %eax
    int $0x80
    jmp prompt_loop

# touch <path>  →  syscall 5 (sys_open, flags=1), path at line_buf+6
do_touch:
    mov $line_buf+6, %ebx
    xor %ecx, %ecx
40:
    movb (%ebx,%ecx), %al
    cmp $'\n', %al
    je 41f
    cmp $0, %al
    je 41f
    inc %ecx
    jmp 40b
41:
    movb $0, (%ebx,%ecx)
    mov $1, %ecx
    mov $5, %eax
    int $0x80
    jmp prompt_loop

# cp <src> <dst>  →  syscall 90, ebx=src, ecx=dst
# src at line_buf+3, separated from dst by a space
do_cp:
    mov $line_buf+3, %esi
    mov %esi, %ebx            # save src start
50:
    movb (%esi), %al
    cmp $'\n', %al
    je do_cp_done
    cmp $0, %al
    je do_cp_done
    cmp $' ', %al
    je 51f
    inc %esi
    jmp 50b
51:
    movb $0, (%esi)           # null-terminate src
    inc %esi
    mov %esi, %ecx            # ecx = dst start
52:
    movb (%esi), %al
    cmp $'\n', %al
    je 53f
    cmp $0, %al
    je 53f
    inc %esi
    jmp 52b
53:
    movb $0, (%esi)           # null-terminate dst
    mov $90, %eax
    int $0x80
do_cp_done:
    jmp prompt_loop

# mv <src> <dst>  →  syscall 91, ebx=src, ecx=dst
do_mv:
    mov $line_buf+3, %esi
    mov %esi, %ebx
60:
    movb (%esi), %al
    cmp $'\n', %al
    je do_mv_done
    cmp $0, %al
    je do_mv_done
    cmp $' ', %al
    je 61f
    inc %esi
    jmp 60b
61:
    movb $0, (%esi)
    inc %esi
    mov %esi, %ecx
62:
    movb (%esi), %al
    cmp $'\n', %al
    je 63f
    cmp $0, %al
    je 63f
    inc %esi
    jmp 62b
63:
    movb $0, (%esi)
    mov $91, %eax
    int $0x80
do_mv_done:
    jmp prompt_loop

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

prompt:      .ascii "frodo$ "
ls_path:     .asciz "/"
cmd_help:    .asciz "help"
cmd_clear:   .asciz "clear"
cmd_exit:    .asciz "exit"
cmd_ls:      .asciz "ls"
cmd_meminfo: .asciz "meminfo"
cmd_uname:   .asciz "uname"
cmd_whoami:  .asciz "whoami"
cmd_reboot:  .asciz "reboot"
cmd_echo:    .asciz "echo"
cmd_rm:      .asciz "rm "
cmd_mkdir:   .asciz "mkdir "
cmd_touch:   .asciz "touch "
cmd_cp:      .asciz "cp "
cmd_mv:      .asciz "mv "

msg_help:    .ascii "Commands: help clear ls meminfo uname whoami echo reboot exit rm mkdir touch cp mv\n"
msg_help_len = . - msg_help

msg_clear:
    .ascii "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
msg_clear_len = . - msg_clear

msg_meminfo: .ascii "PMM pool: 128 frames (512KB)\n"
msg_meminfo_len = . - msg_meminfo

msg_uname:   .ascii "Frodo OS 1.0 i386\n"
msg_uname_len = . - msg_uname

msg_whoami:  .ascii "root\n"
msg_whoami_len = . - msg_whoami

msg_bye:     .ascii "Farewell, Ring-bearer.\n"
msg_bye_len  = . - msg_bye

msg_unkn:    .ascii "Unknown command. Try 'help'.\n"
msg_unkn_len = . - msg_unkn

.section .bss
line_buf: .space 256
