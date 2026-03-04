#include "process.h"
#include "paging.h"

uint32_t Process::KernelStackTop() const {
    return (uint32_t)(kernel_stack + KERNEL_STACK_SIZE);
}

Process::~Process() {
    if (page_dir && page_dir != PagingManager::GetKernelDirectory())
        delete page_dir;
}

Process::Process(uint32_t entry_point, uint32_t pid) {
    this->pid        = pid;
    this->state      = PROCESS_READY;
    this->exit_code  = 0;
    this->parent_pid = -1;
    this->wait_pid   = -1;

    uint32_t user_stack_top = 0x00C00000 + pid * 0x10000;

    uint32_t* kstack = (uint32_t*)KernelStackTop();

    // CPU-pushed iret frame (ring-change: 5 dwords)
    *(--kstack) = 0x23;           // user SS  (user data | RPL=3)
    *(--kstack) = user_stack_top; // user ESP
    *(--kstack) = 0x202;          // EFLAGS: IF=1, reserved bit 1
    *(--kstack) = 0x1B;           // user CS  (user code | RPL=3)
    *(--kstack) = entry_point;    // user EIP

    // pusha frame: pusha pushes EAX,ECX,EDX,EBX,ESP,EBP,ESI,EDI
    // popa pops  : EDI,ESI,EBP,(skip),EBX,EDX,ECX,EAX
    // push in reverse-popa order so the stack top is EDI
    *(--kstack) = 0;  // EAX (pusha pushed first = at highest addr)
    *(--kstack) = 0;  // ECX
    *(--kstack) = 0;  // EDX
    *(--kstack) = 0;  // EBX
    *(--kstack) = 0;  // old ESP (popa skips)
    *(--kstack) = 0;  // EBP
    *(--kstack) = 0;  // ESI
    *(--kstack) = 0;  // EDI (pusha pushed last = at lowest addr = top of stack)

    // Segment-register frame: stub does push %ds/%es/%fs/%gs/%ss
    // pop order: pop %ss, pop %gs, pop %fs, pop %es, pop %ds
    // so SS must be at the top (lowest addr) → push DS first, SS last
    *(--kstack) = 0x23;  // DS (last popped)
    *(--kstack) = 0x23;  // ES
    *(--kstack) = 0x23;  // FS
    *(--kstack) = 0x23;  // GS
    *(--kstack) = 0x10;  // SS (first popped, kernel data - in ring 0 at that point)

    cpu_state.esp = (uint32_t)kstack;
    cpu_state.ss  = 0x10;

    if (PagingManager::GetKernelDirectory()) {
        page_dir = PageDirectory::CreateUser();
        for (uint32_t addr = 0x00400000; addr < 0x00500000; addr += 0x1000)
            page_dir->MapPage(addr, addr, PAGE_USER_RW);
        uint32_t stack_base = 0x00C00000 + pid * 0x10000;
        for (uint32_t addr = stack_base - 0x10000; addr < stack_base; addr += 0x1000)
            page_dir->MapPage(addr, addr, PAGE_USER_RW);
    } else {
        page_dir = nullptr;
    }
}
