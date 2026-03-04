#include "scheduler.h"
#include "paging.h"

Scheduler* Scheduler::instance = nullptr;
volatile uint32_t pit_ticks = 0;

class SchedulerTimer : public InterruptHandler {
public:
    SchedulerTimer(InterruptManager* mgr)
        : InterruptHandler(32, mgr) {}
    uint32_t HandleInterrupt(uint32_t esp) override {
        pit_ticks++;
        return Scheduler::GetInstance()->Schedule(esp);
    }
};

Scheduler::Scheduler(InterruptManager* interrupts, TaskStateSegment* tss)
    : process_count(0), current_index(-1), tss(tss)
{
    instance = this;
    for (int i = 0; i < MAX_PROCESSES; i++) processes[i] = nullptr;
    new SchedulerTimer(interrupts);
}

Scheduler* Scheduler::GetInstance() {
    return instance;
}

void Scheduler::AddProcess(Process* p) {
    if (process_count >= MAX_PROCESSES) return;
    p->state = PROCESS_READY;
    processes[process_count++] = p;
}

uint32_t Scheduler::Schedule(uint32_t esp) {
    int  orig_index = current_index;
    bool was_running = false;

    if (current_index >= 0) {
        Process* cur = processes[current_index];
        if (cur->state == PROCESS_RUNNING) {
            was_running        = true;
            cur->state         = PROCESS_READY;
            cur->cpu_state.esp = esp;
        }
    }

    int found = current_index;
    for (int i = 1; i <= process_count; i++) {
        int idx = (current_index + i) % process_count;
        if (processes[idx]->state == PROCESS_READY) {
            found = idx;
            break;
        }
    }

    if (found >= 0 && processes[found]->state == PROCESS_READY) {
        current_index = found;
        Process* next = processes[current_index];
        next->state   = PROCESS_RUNNING;
        if (next->page_dir)
            PagingManager::SwitchDirectory(next->page_dir);
        tss->SetKernelStack(next->KernelStackTop());
        if (!was_running && found == orig_index)
            return esp;
        return next->cpu_state.esp;
    }

    return esp;
}

Process* Scheduler::GetActive() {
    if (current_index < 0) return nullptr;
    return processes[current_index];
}

Process* Scheduler::FindProcess(int pid) {
    for (int i = 0; i < process_count; i++)
        if (processes[i] && (int)processes[i]->pid == pid)
            return processes[i];
    return nullptr;
}

void Scheduler::WakeParent(int child_pid) {
    for (int i = 0; i < process_count; i++) {
        if (processes[i] && processes[i]->state == PROCESS_BLOCKED
                         && processes[i]->wait_pid == child_pid) {
            processes[i]->state    = PROCESS_READY;
            processes[i]->wait_pid = -1;
        }
    }
}

int Scheduler::NextPID() {
    int max = 0;
    for (int i = 0; i < process_count; i++)
        if (processes[i] && (int)processes[i]->pid > max)
            max = (int)processes[i]->pid;
    return max + 1;
}
