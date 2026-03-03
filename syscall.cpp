#include "syscall.h"
#include "scheduler.h"
#include "process.h"
#include "vga.h"
#include "keyboard_buffer.h"
#include "fat32.h"
#include "terminal.h"

extern FAT32* global_fat32;

struct SyscallFrame {
    uint32_t interrupt_number;
    uint32_t saved_esp;
    uint32_t ss, gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp_ignored;
    uint32_t ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags;
    uint32_t user_esp, user_ss;
} __attribute__((packed));

static uint32_t sys_write(uint32_t fd, const void* buf, uint32_t n) {
    uint32_t addr = (uint32_t)buf;
    if (addr < 0x00400000 || addr + n > 0x00D00000)
        return (uint32_t)-1;
    if (fd == 1 || fd == 2) {
        const char* p = (const char*)buf;
        Terminal* term = Terminal::GetActive();
        if (term) {
            term->SetShellOutput(p, n);
        } else {
            for (uint32_t i = 0; i < n; i++) VGA::PutChar(p[i]);
        }
        return n;
    }
    return (uint32_t)-1;
}

static uint32_t sys_read(uint32_t fd, void* buf, uint32_t n, uint32_t /*esp*/) {
    if (fd != 0) return (uint32_t)-1;

    if (KeyboardBuffer::IsEmpty()) {
        Process* p = Scheduler::GetInstance()->GetActive();
        if (p) {
            p->state    = PROCESS_BLOCKED;
            p->wait_pid = -2;
        }
        __asm__ volatile("sti");
        while (KeyboardBuffer::IsEmpty())
            __asm__ volatile("hlt");
        __asm__ volatile("cli");
        if (p) {
            p->state    = PROCESS_RUNNING;
            p->wait_pid = -1;
        }
    }

    uint32_t bytes_read = 0;
    char* dst = (char*)buf;
    while (bytes_read < n) {
        char c;
        if (!KeyboardBuffer::Dequeue(&c)) break;
        dst[bytes_read++] = c;
        if (c == '\n') break;
    }
    return bytes_read;
}

static uint32_t sys_getpid() {
    Process* p = Scheduler::GetInstance()->GetActive();
    if (p) return p->pid;
    return 0;
}

static uint32_t sys_waitpid(int pid, int* status, int /*options*/, uint32_t /*esp*/) {
    Process* target = Scheduler::GetInstance()->FindProcess(pid);
    if (!target) return (uint32_t)-1;

    Process* cur = Scheduler::GetInstance()->GetActive();
    if (cur) {
        cur->state    = PROCESS_BLOCKED;
        cur->wait_pid = pid;
    }

    __asm__ volatile("sti");
    while (target->state != PROCESS_DEAD)
        __asm__ volatile("hlt");
    __asm__ volatile("cli");

    if (cur) {
        cur->state    = PROCESS_RUNNING;
        cur->wait_pid = -1;
    }

    if (status) *status = target->exit_code;
    return (uint32_t)pid;
}

static uint32_t sys_exec(const char* path) {
    if (!path || !global_fat32) return (uint32_t)-1;

    uint32_t addr = (uint32_t)path;
    if (addr < 0x00400000 || addr >= 0x00D00000) return (uint32_t)-1;

    uint8_t* load_buf = (uint8_t*)0x00400000;
    uint32_t size = 0;
    if (!global_fat32->ReadFile(path, load_buf, &size))
        return (uint32_t)-1;

    int new_pid = Scheduler::GetInstance()->NextPID();
    Process* child = new Process(0x00400000, (uint32_t)new_pid);
    Process* caller = Scheduler::GetInstance()->GetActive();
    if (caller) child->parent_pid = (int)caller->pid;

    Scheduler::GetInstance()->AddProcess(child);
    return (uint32_t)new_pid;
}

uint32_t SyscallDispatch(uint32_t esp) {
    SyscallFrame* frame = (SyscallFrame*)esp;
    uint32_t num = frame->eax;

    switch (num) {
    case 1: {
        Process* p = Scheduler::GetInstance()->GetActive();
        if (p) {
            p->exit_code = (int)frame->ebx;
            p->state     = PROCESS_DEAD;
            Scheduler::GetInstance()->WakeParent((int)p->pid);
        }
        return Scheduler::GetInstance()->Schedule(esp);
    }
    case 3:
        frame->eax = sys_read(frame->ebx, (void*)frame->ecx, frame->edx, esp);
        break;
    case 4:
        frame->eax = sys_write(frame->ebx, (const void*)frame->ecx, frame->edx);
        break;
    case 7:
        frame->eax = sys_waitpid((int)frame->ebx, (int*)frame->ecx, frame->edx, esp);
        break;
    case 11:
        frame->eax = sys_exec((const char*)frame->ebx);
        break;
    case 20:
        frame->eax = sys_getpid();
        break;
    default:
        frame->eax = (uint32_t)-1;
        break;
    }

    return esp;
}
