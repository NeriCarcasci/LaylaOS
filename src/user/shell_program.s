.code32
.equ NET_SHARED_BASE, 0x00600000
.equ NET_BUF_STATUS,   NET_SHARED_BASE + 0
.equ NET_BUF_DATA_LEN, NET_SHARED_BASE + 4
.equ NET_BUF_DATA,     NET_SHARED_BASE + 8
.equ NET_BUF_MAX_DATA, 4088

.section .text
.global _start

_start:
    movb $'/', cwd_buf
    movb $0, cwd_buf+1

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

    mov $cmd_cd, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_cd

    mov $cmd_pwd, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_pwd

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

    mov $cmd_nano, %edi
    call strcmp_prefix
    test %eax, %eax
    je do_nano

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
    mov $line_buf+2, %esi
    mov $arg_buf, %edi
    call extract_arg
    jc do_ls_cwd

    mov $arg_buf, %esi
    mov $path_buf1, %edi
    call make_abs_path
    mov $path_buf1, %ebx
    jmp do_ls_call

do_ls_cwd:
    mov $cwd_buf, %ebx

do_ls_call:
    mov $40, %eax
    int $0x80
    jmp prompt_loop

do_cd:
    mov $line_buf+2, %esi
    mov $arg_buf, %edi
    call extract_arg
    jc do_cd_root

    movb arg_buf, %al
    cmp $'.', %al
    jne do_cd_normal
    movb arg_buf+1, %al
    cmp $0, %al
    je do_cd_done
    cmp $'.', %al
    jne do_cd_normal
    movb arg_buf+2, %al
    cmp $0, %al
    jne do_cd_normal
    mov $cwd_buf, %esi
    mov $path_buf1, %edi
    call copy_cstr
    mov $path_buf1, %edi
    call path_up_one
    jmp do_cd_validate

do_cd_normal:
    mov $arg_buf, %esi
    mov $path_buf1, %edi
    call make_abs_path
    jmp do_cd_validate

do_cd_root:
    movb $'/', path_buf1
    movb $0, path_buf1+1

do_cd_validate:
    mov $41, %eax
    mov $path_buf1, %ebx
    int $0x80
    cmp $1, %eax
    jne do_cd_fail

    mov $path_buf1, %esi
    mov $cwd_buf, %edi
    call copy_cstr

do_cd_done:
    mov $4, %eax
    mov $1, %ebx
    mov $msg_cd_to, %ecx
    mov $msg_cd_to_len, %edx
    int $0x80

    mov $cwd_buf, %ecx
    call cstrlen
    mov $4, %eax
    mov $1, %ebx
    int $0x80

    mov $4, %eax
    mov $1, %ebx
    mov $msg_nl, %ecx
    mov $1, %edx
    int $0x80
    jmp prompt_loop

do_cd_fail:
    mov $4, %eax
    mov $1, %ebx
    mov $msg_cd_fail, %ecx
    mov $msg_cd_fail_len, %edx
    int $0x80
    jmp prompt_loop

do_pwd:
    mov $cwd_buf, %ecx
    call cstrlen
    mov $4, %eax
    mov $1, %ebx
    int $0x80

    mov $4, %eax
    mov $1, %ebx
    mov $msg_nl, %ecx
    mov $1, %edx
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
    movb line_buf+4, %al
    cmp $'\n', %al
    je do_ping_usage
    cmp $0, %al
    je do_ping_usage
    cmp $'\r', %al
    je do_ping_usage
    cmp $' ', %al
    jne do_ping_usage

    mov $line_buf+5, %esi
    call parse_ip
    jc do_ping_usage

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

do_ping_usage:
    mov $4, %eax
    mov $1, %ebx
    mov $msg_ping_usage, %ecx
    mov $msg_ping_usage_len, %edx
    int $0x80
    jmp prompt_loop

do_ping_pong:
    mov $4, %eax
    mov $1, %ebx
    mov $msg_pong, %ecx
    mov $msg_pong_len, %edx
    int $0x80
    jmp prompt_loop

do_nano:
    mov $line_buf+4, %esi
    mov $arg_buf, %edi
    call extract_arg
    jc do_nano_usage

    mov $arg_buf, %esi
    mov $path_buf1, %edi
    call make_abs_path

    mov $93, %eax
    mov $path_buf1, %ebx
    mov $nano_buf, %ecx
    mov $2047, %edx
    int $0x80
    test %eax, %eax
    jl do_nano_empty
    mov %eax, nano_len
    mov %eax, %ecx
    movb $0, nano_buf(%ecx)
    jmp do_nano_paint

do_nano_empty:
    movl $0, nano_len
    movb $0, nano_buf

do_nano_paint:
    mov $4, %eax
    mov $1, %ebx
    mov $msg_clear_nano, %ecx
    mov $msg_clear_nano_len, %edx
    int $0x80

    mov $4, %eax
    mov $1, %ebx
    mov $msg_nano_header, %ecx
    mov $msg_nano_header_len, %edx
    int $0x80

    mov $path_buf1, %ecx
    call cstrlen
    mov $4, %eax
    mov $1, %ebx
    mov $path_buf1, %ecx
    int $0x80

    mov $4, %eax
    mov $1, %ebx
    mov $msg_nl, %ecx
    mov $1, %edx
    int $0x80

    mov $4, %eax
    mov $1, %ebx
    mov $msg_nano_help, %ecx
    mov $msg_nano_help_len, %edx
    int $0x80

    mov nano_len, %edx
    test %edx, %edx
    jz do_nano_enter_raw
    mov $4, %eax
    mov $1, %ebx
    mov $nano_buf, %ecx
    int $0x80

do_nano_enter_raw:
    mov $94, %eax
    mov $1, %ebx
    int $0x80

do_nano_loop:
    mov $3, %eax
    mov $0, %ebx
    mov $key_buf, %ecx
    mov $1, %edx
    int $0x80
    cmp $1, %eax
    jne do_nano_loop

    movzbl key_buf, %eax
    cmp $24, %al               # Ctrl+X
    je do_nano_exit
    cmp $19, %al               # Ctrl+S
    je do_nano_save
    cmp $8, %al
    je do_nano_backspace
    cmp $127, %al
    je do_nano_backspace
    cmp $'\r', %al
    jne do_nano_keep_char
    movb $'\n', key_buf

do_nano_keep_char:
    movzbl key_buf, %eax
    cmp $'\n', %al
    je do_nano_append
    cmp $'\t', %al
    je do_nano_append
    cmp $32, %al
    jb do_nano_loop

do_nano_append:
    mov nano_len, %ecx
    cmp $2047, %ecx
    jae do_nano_loop
    movb key_buf, %al
    movb %al, nano_buf(%ecx)
    inc %ecx
    mov %ecx, nano_len
    movb $0, nano_buf(%ecx)

    mov $4, %eax
    mov $1, %ebx
    mov $key_buf, %ecx
    mov $1, %edx
    int $0x80
    jmp do_nano_loop

do_nano_backspace:
    mov nano_len, %ecx
    test %ecx, %ecx
    jz do_nano_loop
    dec %ecx
    mov %ecx, nano_len
    movb $0, nano_buf(%ecx)
    movb $8, key_buf

    mov $4, %eax
    mov $1, %ebx
    mov $key_buf, %ecx
    mov $1, %edx
    int $0x80
    jmp do_nano_loop

do_nano_save:
    mov nano_len, %edx
    mov $92, %eax
    mov $path_buf1, %ebx
    mov $nano_buf, %ecx
    int $0x80
    cmp $0, %eax
    jne do_nano_fail
    jmp do_nano_loop

do_nano_exit:
    mov $94, %eax
    xor %ebx, %ebx
    int $0x80
    jmp prompt_loop

do_nano_usage:
    mov $4, %eax
    mov $1, %ebx
    mov $msg_nano_usage, %ecx
    mov $msg_nano_usage_len, %edx
    int $0x80
    jmp prompt_loop

do_nano_fail:
    mov $94, %eax
    xor %ebx, %ebx
    int $0x80

    mov $4, %eax
    mov $1, %ebx
    mov $msg_nano_fail, %ecx
    mov $msg_nano_fail_len, %edx
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
    mov $line_buf+2, %esi
    mov $arg_buf, %edi
    call extract_arg
    jc prompt_loop

    mov $arg_buf, %esi
    mov $path_buf1, %edi
    call make_abs_path

    mov $path_buf1, %ebx
    mov $6, %eax
    int $0x80
    jmp prompt_loop

# mkdir <dir>  →  syscall 39, path at line_buf+6
do_mkdir:
    mov $line_buf+5, %esi
    mov $arg_buf, %edi
    call extract_arg
    jc prompt_loop

    mov $arg_buf, %esi
    mov $path_buf1, %edi
    call make_abs_path

    mov $path_buf1, %ebx
    mov $39, %eax
    int $0x80
    jmp prompt_loop

# touch <path>  →  syscall 5 (sys_open, flags=1), path at line_buf+6
do_touch:
    mov $line_buf+5, %esi
    mov $arg_buf, %edi
    call extract_arg
    jc prompt_loop

    mov $arg_buf, %esi
    mov $path_buf1, %edi
    call make_abs_path

    mov $path_buf1, %ebx
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

    push %ecx
    mov %ebx, %esi
    mov $path_buf1, %edi
    call make_abs_path

    pop %esi
    mov $path_buf2, %edi
    call make_abs_path

    mov $path_buf1, %ebx
    mov $path_buf2, %ecx
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

    push %ecx
    mov %ebx, %esi
    mov $path_buf1, %edi
    call make_abs_path

    pop %esi
    mov $path_buf2, %edi
    call make_abs_path

    mov $path_buf1, %ebx
    mov $path_buf2, %ecx
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

# cstrlen: compute C-string length in %edx
# in:  %ecx = pointer
# out: %edx = length, preserves %ecx
cstrlen:
    push %ecx
    xor %edx, %edx
cstrlen_loop:
    movb (%ecx,%edx), %al
    cmp $0, %al
    je cstrlen_done
    inc %edx
    jmp cstrlen_loop
cstrlen_done:
    pop %ecx
    ret

# copy_cstr: copy NUL-terminated string
# in: %esi=src, %edi=dst
copy_cstr:
copy_cstr_loop:
    movb (%esi), %al
    movb %al, (%edi)
    inc %esi
    inc %edi
    cmp $0, %al
    jne copy_cstr_loop
    ret

# extract_arg: read first argument token from command tail
# in:  %esi = pointer to command tail
#      %edi = destination buffer
# out: CF=0 success, CF=1 no argument
extract_arg:
extract_arg_skip:
    movb (%esi), %al
    cmp $' ', %al
    jne extract_arg_start
    inc %esi
    jmp extract_arg_skip

extract_arg_start:
    cmp $'\n', %al
    je extract_arg_fail
    cmp $0, %al
    je extract_arg_fail
    cmp $'\r', %al
    je extract_arg_fail

    xor %ecx, %ecx
extract_arg_copy:
    movb (%esi), %al
    cmp $'\n', %al
    je extract_arg_trim
    cmp $0, %al
    je extract_arg_trim
    cmp $'\r', %al
    je extract_arg_trim
    cmp $255, %ecx
    jae extract_arg_trim
    movb %al, (%edi,%ecx)
    inc %esi
    inc %ecx
    jmp extract_arg_copy

extract_arg_trim:
    test %ecx, %ecx
    jz extract_arg_fail

    mov %ecx, %edx
extract_arg_trim_loop:
    test %edx, %edx
    jz extract_arg_fail
    movb -1(%edi,%edx), %al
    cmp $' ', %al
    jne extract_arg_done
    dec %edx
    jmp extract_arg_trim_loop

extract_arg_done:
    movb $0, (%edi,%edx)
    clc
    ret

extract_arg_fail:
    stc
    ret

# make_abs_path: convert relative path to absolute using cwd_buf
# in:  %esi = source path
#      %edi = destination buffer
make_abs_path:
    movb (%esi), %al
    cmp $'/', %al
    je make_abs_copy_src

    xor %ecx, %ecx
make_abs_copy_cwd:
    movb cwd_buf(%ecx), %al
    movb %al, (%edi,%ecx)
    cmp $0, %al
    je make_abs_cwd_done
    inc %ecx
    cmp $255, %ecx
    jb make_abs_copy_cwd
    movb $0, 255(%edi)
    jmp make_abs_strip

make_abs_cwd_done:
    cmp $1, %ecx
    jbe make_abs_append_src
    movb $'/', (%edi,%ecx)
    inc %ecx
    movb $0, (%edi,%ecx)

make_abs_append_src:
    xor %edx, %edx
make_abs_append_loop:
    movb (%esi,%edx), %al
    movb %al, (%edi,%ecx)
    cmp $0, %al
    je make_abs_strip
    inc %edx
    inc %ecx
    cmp $255, %ecx
    jb make_abs_append_loop
    movb $0, 255(%edi)
    jmp make_abs_strip

make_abs_copy_src:
    xor %ecx, %ecx
make_abs_copy_src_loop:
    movb (%esi,%ecx), %al
    movb %al, (%edi,%ecx)
    cmp $0, %al
    je make_abs_strip
    inc %ecx
    cmp $255, %ecx
    jb make_abs_copy_src_loop
    movb $0, 255(%edi)

make_abs_strip:
    xor %ecx, %ecx
make_abs_find_end:
    movb (%edi,%ecx), %al
    cmp $0, %al
    je make_abs_end_found
    inc %ecx
    jmp make_abs_find_end

make_abs_end_found:
    cmp $1, %ecx
    jbe make_abs_done

make_abs_strip_loop:
    movb -1(%edi,%ecx), %al
    cmp $'/', %al
    jne make_abs_done
    movb $0, -1(%edi,%ecx)
    dec %ecx
    cmp $1, %ecx
    ja make_abs_strip_loop

make_abs_done:
    ret

# path_up_one: move path in %edi to its parent directory
path_up_one:
    xor %ecx, %ecx
path_up_find_end:
    movb (%edi,%ecx), %al
    cmp $0, %al
    je path_up_end_found
    inc %ecx
    jmp path_up_find_end

path_up_end_found:
    cmp $1, %ecx
    jbe path_up_done
    dec %ecx

path_up_scan:
    cmp $0, %ecx
    je path_up_root
    movb (%edi,%ecx), %al
    cmp $'/', %al
    je path_up_cut
    dec %ecx
    jmp path_up_scan

path_up_root:
    movb $'/', (%edi)
    movb $0, 1(%edi)
    jmp path_up_done

path_up_cut:
    cmp $0, %ecx
    je path_up_root
    movb $0, (%edi,%ecx)

path_up_done:
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
cmd_cd:      .asciz "cd"
cmd_pwd:     .asciz "pwd"
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
cmd_nano:    .asciz "nano"
cmd_ping:    .asciz "ping"
cmd_nc:      .asciz "nc "

msg_help:    .ascii "Commands: help clear ls cd pwd nano meminfo uname whoami echo reboot exit rm mkdir touch cp mv ping nc\n"
msg_help_len = . - msg_help

msg_clear:
    .ascii "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
msg_clear_len = . - msg_clear

msg_clear_nano:
    .ascii "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
msg_clear_nano_len = . - msg_clear_nano

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

msg_ping_usage:    .ascii "usage: ping <ip>\n"
msg_ping_usage_len = . - msg_ping_usage

msg_ping_payload: .ascii "ping frodo\n"

msg_cd_to: .ascii "cd -> "
msg_cd_to_len = . - msg_cd_to

msg_cd_fail: .ascii "cd: not a directory\n"
msg_cd_fail_len = . - msg_cd_fail

msg_nano_header: .ascii "-- nano -- "
msg_nano_header_len = . - msg_nano_header

msg_nano_usage: .ascii "usage: nano <path>\n"
msg_nano_usage_len = . - msg_nano_usage

msg_nano_help: .ascii "Ctrl+S save | Ctrl+X exit\n\n"
msg_nano_help_len = . - msg_nano_help

msg_nano_fail: .ascii "\n[nano write failed]\n"
msg_nano_fail_len = . - msg_nano_fail

msg_nl: .ascii "\n"

msg_unkn:    .ascii "Unknown command. Try 'help'.\n"
msg_unkn_len = . - msg_unkn

.section .bss
line_buf: .space 256
arg_buf:  .space 256
path_buf1: .space 256
path_buf2: .space 256
cwd_buf:  .space 256
nano_buf: .space 2048
nano_len: .space 4
key_buf:  .space 1
nc_ip:    .space 4
nc_port:  .space 4
