#ifndef __PROCESS_H
#define __PROCESS_H

#include "types.h"

class PageDirectory;

enum ProcessState {
    PROCESS_RUNNING,
    PROCESS_READY,
    PROCESS_BLOCKED,
    PROCESS_DEAD
};

struct CPUState {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
    uint32_t esp;
    uint32_t ss;
} __attribute__((packed));

class Process {
public:
    Process(uint32_t entry_point, uint32_t pid);
    ~Process();

    uint32_t      pid;
    ProcessState  state;
    CPUState      cpu_state;
    PageDirectory* page_dir;
    int           exit_code;
    int           parent_pid;
    int           wait_pid;

    static const uint32_t KERNEL_STACK_SIZE = 4096;
    uint8_t kernel_stack[KERNEL_STACK_SIZE];

    uint32_t KernelStackTop() const;
};

#endif
