#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include "types.h"
#include "interrupts.h"
#include "process.h"
#include "tss.h"

// NOTE: no paging — Ring 3 processes share the kernel's flat physical address
// space. DPL checks and syscall argument validation are the only protection.

extern volatile uint32_t pit_ticks;

class Scheduler {
public:
    Scheduler(InterruptManager* interrupts, TaskStateSegment* tss);

    void     AddProcess(Process* p);
    uint32_t Schedule(uint32_t esp);
    Process* GetActive();
    Process* FindProcess(int pid);
    void     WakeParent(int child_pid);
    int      NextPID();

    static Scheduler* GetInstance();

private:
    static const int MAX_PROCESSES = 16;
    Process*   processes[MAX_PROCESSES];
    int        process_count;
    int        current_index;
    TaskStateSegment* tss;

    static Scheduler* instance;
};

#endif
