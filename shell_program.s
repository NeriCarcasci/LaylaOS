.code32
.equ NET_SHARED_BASE, 0x00600000
.equ NET_BUF_STATUS,   NET_SHARED_BASE + 0
.equ NET_BUF_DATA_LEN, NET_SHARED_BASE + 4
.equ NET_BUF_DATA,     NET_SHARED_BASE + 8
.equ NET_BUF_MAX_DATA, 4088

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

    mov $cmd_ping, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_ping

    mov $cmd_nc, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_nc

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

do_ping:
    mov $line_buf+5, %esi
    call parse_ip
    jc do_ping_timeout

    mov %eax, %ebx

    cld
    mov $msg_ping_payload, %esi
    mov $NET_BUF_DATA, %edi
    mov $11, %ecx
    rep movsb

    movl $11, NET_BUF_DATA_LEN

    mov $50, %eax
    mov $7, %ecx
    mov $1234, %edx
    int $0x80

    mov $51, %eax
    mov $2000, %ebx
    int $0x80

    movl NET_BUF_STATUS, %eax
    test %eax, %eax
    je do_ping_pong

do_ping_timeout:
    mov $4, %eax
    mov $1, %ebx
    mov $msg_ping_timeout, %ecx
    mov $msg_ping_timeout_len, %edx
    int $0x80
    jmp prompt_loop

do_ping_pong:
    mov $4, %eax
    mov $1, %ebx
    mov $msg_pong, %ecx
    mov $msg_pong_len, %edx
    int $0x80
    jmp prompt_loop

do_nc:
    mov $line_buf+3, %esi
    call parse_ip
    jc do_nc_done
    mov %eax, nc_ip

    movb (%esi), %al
    cmp $' ', %al
    jne do_nc_done
    inc %esi

    call parse_uint16
    jc do_nc_done
    mov %eax, nc_port

    movb (%esi), %al
    cmp $'\n', %al
    je 70f
    cmp $0, %al
    je 70f
    cmp $'\r', %al
    je 70f
    jmp do_nc_done

70:
    mov $52, %eax
    mov nc_ip, %ebx
    mov nc_port, %ecx
    int $0x80
    cmp $0, %eax
    jne do_nc_done

    mov $3, %eax
    mov $0, %ebx
    mov $line_buf, %ecx
    mov $255, %edx
    int $0x80
    test %eax, %eax
    jle do_nc_close

    mov %eax, %edx
    cmp $NET_BUF_MAX_DATA, %edx
    jbe 71f
    mov $NET_BUF_MAX_DATA, %edx
71:
    cld
    mov $line_buf, %esi
    mov $NET_BUF_DATA, %edi
    mov %edx, %ecx
    rep movsb
    mov %edx, NET_BUF_DATA_LEN

    mov $53, %eax
    int $0x80

    mov $54, %eax
    mov $3000, %ebx
    int $0x80

    mov NET_BUF_STATUS, %eax
    test %eax, %eax
    jne do_nc_close

    mov $4, %eax
    mov $1, %ebx
    mov $NET_BUF_DATA, %ecx
    mov NET_BUF_DATA_LEN, %edx
    int $0x80

do_nc_close:
    mov $55, %eax
    int $0x80

do_nc_done:
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
strcmp_loop:
    movb (%edi), %al
    cmp $0, %al
    je strcmp_match
    movb (%esi), %bl
    cmp %al, %bl
    jne strcmp_nomatch
    inc %esi
    inc %edi
    jmp strcmp_loop
strcmp_match:
    xor %eax, %eax
    pop %edi
    pop %esi
    ret
strcmp_nomatch:
    mov $1, %eax
    pop %edi
    pop %esi
    ret

# parse_uint16: parse decimal from %esi
# out: %eax=value, %esi advanced, CF=0 success / CF=1 fail
parse_uint16:
    xor %eax, %eax
    xor %ecx, %ecx
parse_uint16_loop:
    movzbl (%esi), %edx
    cmp $'0', %dl
    jb parse_uint16_done
    cmp $'9', %dl
    ja parse_uint16_done
    imul $10, %eax, %eax
    sub $'0', %edx
    add %edx, %eax
    cmp $65535, %eax
    ja parse_uint16_fail
    inc %esi
    inc %ecx
    jmp parse_uint16_loop
parse_uint16_done:
    test %ecx, %ecx
    jz parse_uint16_fail
    clc
    ret
parse_uint16_fail:
    stc
    ret

# parse_ip: parse dotted decimal from %esi
# out: %eax = A<<24|B<<16|C<<8|D, %esi at delimiter, CF=0 success
parse_ip:
    push %ebx
    push %ecx
    push %edx

    xor %ebx, %ebx
    mov $4, %ecx
parse_ip_octet:
    push %ecx
    call parse_uint16
    pop %ecx
    jc parse_ip_fail
    cmp $255, %eax
    ja parse_ip_fail

    shl $8, %ebx
    or %eax, %ebx

    dec %ecx
    jz parse_ip_delim

    movb (%esi), %dl
    cmp $'.', %dl
    jne parse_ip_fail
    inc %esi
    jmp parse_ip_octet

parse_ip_delim:
    movb (%esi), %dl
    cmp $' ', %dl
    je parse_ip_ok
    cmp $'\n', %dl
    je parse_ip_ok
    cmp $0, %dl
    je parse_ip_ok
    cmp $'\r', %dl
    je parse_ip_ok
    jmp parse_ip_fail

parse_ip_ok:
    mov %ebx, %eax
    clc
    pop %edx
    pop %ecx
    pop %ebx
    ret

parse_ip_fail:
    stc
    pop %edx
    pop %ecx
    pop %ebx
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
cmd_ping:    .asciz "ping "
cmd_nc:      .asciz "nc "

msg_help:    .ascii "Commands: help clear ls meminfo uname whoami echo reboot exit rm mkdir touch cp mv ping nc\n"
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

msg_pong:    .ascii "pong received\n"
msg_pong_len = . - msg_pong

msg_ping_timeout:    .ascii "request timed out\n"
msg_ping_timeout_len = . - msg_ping_timeout

msg_ping_payload: .ascii "ping frodo\n"

msg_unkn:    .ascii "Unknown command. Try 'help'.\n"
msg_unkn_len = . - msg_unkn

.section .bss
line_buf: .space 256
nc_ip:    .space 4
nc_port:  .space 4
