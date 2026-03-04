#include "syscall.h"
#include "scheduler.h"
#include "process.h"
#include "vga.h"
#include "keyboard_buffer.h"
#include "fat32.h"
#include "terminal.h"
#include "net_shared.h"
#include "net_hooks.h"

extern FAT32* global_fat32;

struct SyscallFrame {
    uint32_t ss, gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp_ignored;
    uint32_t ebx, edx, ecx, eax;
    uint32_t eip, cs, eflags;
    uint32_t user_esp, user_ss;
} __attribute__((packed));

static uint8_t  udp_rx_buf[2048];
static uint32_t udp_rx_len = 0;
static bool     udp_rx_ready = false;

static uint8_t  tcp_rx_buf[2048];
static uint32_t tcp_rx_len = 0;
static bool     tcp_rx_ready = false;

static bool IsRing3Range(uint32_t addr, uint32_t len) {
    if (len == 0) return true;

    uint32_t end = addr + len;
    if (end < addr)
        return false;

    if (addr >= NET_SHARED_BASE_ADDR && end <= NET_SHARED_BASE_ADDR + 4096)
        return true;

    return addr >= 0x00400000 && end <= 0x00D00000;
}

static bool ValidateNetShared(uint32_t data_len) {
    if (data_len > NET_BUF_MAX_DATA)
        return false;
    if (!IsRing3Range(NET_BUF_STATUS_ADDR, sizeof(uint32_t)))
        return false;
    if (!IsRing3Range(NET_BUF_DATA_LEN_ADDR, sizeof(uint32_t)))
        return false;
    if (!IsRing3Range(NET_BUF_DATA_ADDR, data_len))
        return false;
    return true;
}

static IPv4Address PackedToIPv4(uint32_t packed) {
    return IPv4Address::FromBytes((packed >> 24) & 0xFF,
                                  (packed >> 16) & 0xFF,
                                  (packed >> 8) & 0xFF,
                                  packed & 0xFF);
}

static uint32_t ms_to_ticks(uint32_t ms) {
    return ms / 10 + 1;
}

static bool TimedOut(uint32_t start, uint32_t wait_ticks) {
    return (uint32_t)(pit_ticks - start) >= wait_ticks;
}

SyscallUDPSocket::SyscallUDPSocket(IPv4Handler* ipv4, uint16_t local_port)
    : UDPSocket(ipv4, local_port) {
}

void SyscallUDPSocket::OnReceive(IPv4Address src_ip, uint16_t src_port,
                                 uint8_t* data, uint32_t size) {
    (void)src_ip;
    (void)src_port;

    uint32_t copy = size;
    if (copy > sizeof(udp_rx_buf))
        copy = sizeof(udp_rx_buf);

    for (uint32_t i = 0; i < copy; i++)
        udp_rx_buf[i] = data[i];

    udp_rx_len = copy;
    udp_rx_ready = true;
}

SyscallTCPSocket::SyscallTCPSocket(IPv4Handler* ipv4, uint16_t local_port)
    : TCPSocket(ipv4, local_port) {
}

void SyscallTCPSocket::OnReceive(uint8_t* data, uint32_t size) {
    uint32_t copy = size;
    if (copy > sizeof(tcp_rx_buf))
        copy = sizeof(tcp_rx_buf);

    for (uint32_t i = 0; i < copy; i++)
        tcp_rx_buf[i] = data[i];

    tcp_rx_len = copy;
    tcp_rx_ready = true;
}

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

static uint32_t sys_open(const char* path, uint32_t flags) {
    if (!global_fat32) return (uint32_t)-1;
    uint32_t addr = (uint32_t)path;
    if (addr < 0x00400000 || addr >= 0x00D00000) return (uint32_t)-1;
    if (flags == 1)
        global_fat32->CreateFile(path);
    return 3;
}

static uint32_t sys_unlink(const char* path) {
    if (!global_fat32) return (uint32_t)-1;
    uint32_t addr = (uint32_t)path;
    if (addr < 0x00400000 || addr >= 0x00D00000) return (uint32_t)-1;
    return global_fat32->DeleteFile(path) ? 0 : (uint32_t)-1;
}

static uint32_t sys_mkdir(const char* path) {
    if (!global_fat32) return (uint32_t)-1;
    uint32_t addr = (uint32_t)path;
    if (addr < 0x00400000 || addr >= 0x00D00000) return (uint32_t)-1;
    return global_fat32->MakeDir(path) ? 0 : (uint32_t)-1;
}

static uint32_t sys_ls(const char* path) {
    uint32_t addr = (uint32_t)path;
    if (addr < 0x00400000 || addr >= 0x00D00000)
        return 1;

    if (!global_fat32) {
        const char* no_disk = "(no disk)\n";
        Terminal* term = Terminal::GetActive();
        if (term) {
            term->SetShellOutput(no_disk, 10);
        } else {
            VGA::Print(no_disk);
        }
        return 1;
    }

    return global_fat32->ListDirectory(path) ? 0 : 1;
}

static uint32_t sys_cp(const char* src, const char* dst) {
    if (!global_fat32) return (uint32_t)-1;
    if ((uint32_t)src < 0x00400000 || (uint32_t)src >= 0x00D00000) return (uint32_t)-1;
    if ((uint32_t)dst < 0x00400000 || (uint32_t)dst >= 0x00D00000) return (uint32_t)-1;
    uint8_t* buf = new uint8_t[65536];
    if (!buf) return (uint32_t)-1;
    uint32_t size = 0;
    if (!global_fat32->ReadFile(src, buf, &size)) {
        delete[] buf;
        return (uint32_t)-1;
    }
    bool ok = global_fat32->WriteFile(dst, buf, size);
    delete[] buf;
    return ok ? 0 : (uint32_t)-1;
}

static uint32_t sys_mv(const char* src, const char* dst) {
    if (!global_fat32) return (uint32_t)-1;
    if ((uint32_t)src < 0x00400000 || (uint32_t)src >= 0x00D00000) return (uint32_t)-1;
    if ((uint32_t)dst < 0x00400000 || (uint32_t)dst >= 0x00D00000) return (uint32_t)-1;
    uint8_t* buf = new uint8_t[65536];
    if (!buf) return (uint32_t)-1;
    uint32_t size = 0;
    if (!global_fat32->ReadFile(src, buf, &size)) {
        delete[] buf;
        return (uint32_t)-1;
    }
    bool ok = global_fat32->WriteFile(dst, buf, size);
    delete[] buf;
    if (!ok) return (uint32_t)-1;
    global_fat32->DeleteFile(src);
    return 0;
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

static uint32_t sys_net_send_udp(uint32_t dst_ip_packed, uint16_t dst_port, uint16_t src_port) {
    if (!global_nic || !global_udp)
        return (uint32_t)-1;

    uint32_t len = NET_BUF_DATA_LEN;
    if (len > NET_BUF_MAX_DATA)
        len = NET_BUF_MAX_DATA;

    if (!ValidateNetShared(len))
        return (uint32_t)-1;

    IPv4Address dst_ip = PackedToIPv4(dst_ip_packed);
    return global_udp->Send(dst_ip, dst_port, src_port, NET_BUF_DATA, len) ? 0 : (uint32_t)-1;
}

static uint32_t sys_net_recv_udp(uint32_t timeout_ms) {
    if (!ValidateNetShared(NET_BUF_MAX_DATA)) {
        NET_BUF_STATUS = 1;
        NET_BUF_DATA_LEN = 0;
        return 1;
    }

    if (!udp_rx_ready && timeout_ms != 0) {
        uint32_t wait_ticks = ms_to_ticks(timeout_ms);
        uint32_t start = pit_ticks;

        Process* p = Scheduler::GetInstance()->GetActive();
        if (p) {
            p->state = PROCESS_BLOCKED;
            p->wait_pid = -3;
        }

        __asm__ volatile("sti");
        while (!udp_rx_ready && !TimedOut(start, wait_ticks))
            __asm__ volatile("hlt");
        __asm__ volatile("cli");

        if (p) {
            p->state = PROCESS_RUNNING;
            p->wait_pid = -1;
        }
    }

    if (udp_rx_ready) {
        uint32_t copy = udp_rx_len;
        if (copy > NET_BUF_MAX_DATA)
            copy = NET_BUF_MAX_DATA;

        for (uint32_t i = 0; i < copy; i++)
            NET_BUF_DATA[i] = udp_rx_buf[i];

        NET_BUF_DATA_LEN = copy;
        NET_BUF_STATUS = 0;
        udp_rx_ready = false;
        udp_rx_len = 0;
        return 0;
    }

    NET_BUF_DATA_LEN = 0;
    NET_BUF_STATUS = 1;
    return 1;
}

static uint32_t sys_net_tcp_connect(uint32_t dst_ip_packed, uint16_t dst_port) {
    if (!global_nic || !global_tcp)
        return (uint32_t)-1;

    IPv4Address dst_ip = PackedToIPv4(dst_ip_packed);
    if (!global_tcp->Connect(dst_ip, dst_port))
        return (uint32_t)-1;

    uint32_t wait_ticks = ms_to_ticks(5000);
    uint32_t start = pit_ticks;

    Process* p = Scheduler::GetInstance()->GetActive();
    if (p) {
        p->state = PROCESS_BLOCKED;
        p->wait_pid = -4;
    }

    __asm__ volatile("sti");
    while (global_tcp->State() != TCP_ESTABLISHED && !TimedOut(start, wait_ticks))
        __asm__ volatile("hlt");
    __asm__ volatile("cli");

    if (p) {
        p->state = PROCESS_RUNNING;
        p->wait_pid = -1;
    }

    return (global_tcp->State() == TCP_ESTABLISHED) ? 0 : (uint32_t)-1;
}

static uint32_t sys_net_tcp_send() {
    if (!global_nic || !global_tcp)
        return (uint32_t)-1;

    uint32_t len = NET_BUF_DATA_LEN;
    if (len > NET_BUF_MAX_DATA)
        len = NET_BUF_MAX_DATA;

    if (!ValidateNetShared(len))
        return (uint32_t)-1;

    return global_tcp->Send(NET_BUF_DATA, len) ? 0 : (uint32_t)-1;
}

static uint32_t sys_net_tcp_recv(uint32_t timeout_ms) {
    if (!ValidateNetShared(NET_BUF_MAX_DATA)) {
        NET_BUF_STATUS = 1;
        NET_BUF_DATA_LEN = 0;
        return 1;
    }

    if (!tcp_rx_ready && timeout_ms != 0) {
        uint32_t wait_ticks = ms_to_ticks(timeout_ms);
        uint32_t start = pit_ticks;

        Process* p = Scheduler::GetInstance()->GetActive();
        if (p) {
            p->state = PROCESS_BLOCKED;
            p->wait_pid = -5;
        }

        __asm__ volatile("sti");
        while (!tcp_rx_ready && !TimedOut(start, wait_ticks))
            __asm__ volatile("hlt");
        __asm__ volatile("cli");

        if (p) {
            p->state = PROCESS_RUNNING;
            p->wait_pid = -1;
        }
    }

    if (tcp_rx_ready) {
        uint32_t copy = tcp_rx_len;
        if (copy > NET_BUF_MAX_DATA)
            copy = NET_BUF_MAX_DATA;

        for (uint32_t i = 0; i < copy; i++)
            NET_BUF_DATA[i] = tcp_rx_buf[i];

        NET_BUF_DATA_LEN = copy;
        NET_BUF_STATUS = 0;
        tcp_rx_ready = false;
        tcp_rx_len = 0;
        return 0;
    }

    NET_BUF_DATA_LEN = 0;
    NET_BUF_STATUS = 1;
    return 1;
}

static uint32_t sys_net_tcp_close() {
    if (global_tcp)
        global_tcp->Close();
    return 0;
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
    case 5:
        frame->eax = sys_open((const char*)frame->ebx, frame->ecx);
        break;
    case 6:
        frame->eax = sys_unlink((const char*)frame->ebx);
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
    case 39:
        frame->eax = sys_mkdir((const char*)frame->ebx);
        break;
    case 40:
        frame->eax = sys_ls((const char*)frame->ebx);
        break;
    case 50:
        frame->eax = sys_net_send_udp(frame->ebx, (uint16_t)frame->ecx, (uint16_t)frame->edx);
        break;
    case 51:
        frame->eax = sys_net_recv_udp(frame->ebx);
        break;
    case 52:
        frame->eax = sys_net_tcp_connect(frame->ebx, (uint16_t)frame->ecx);
        break;
    case 53:
        frame->eax = sys_net_tcp_send();
        break;
    case 54:
        frame->eax = sys_net_tcp_recv(frame->ebx);
        break;
    case 55:
        frame->eax = sys_net_tcp_close();
        break;
    case 90:
        frame->eax = sys_cp((const char*)frame->ebx, (const char*)frame->ecx);
        break;
    case 91:
        frame->eax = sys_mv((const char*)frame->ebx, (const char*)frame->ecx);
        break;
    case 88:
        __asm__ volatile("out %0, $0x64" : : "a"((uint8_t)0xFE));
        while (1) __asm__ volatile("hlt");
        break;
    default:
        frame->eax = (uint32_t)-1;
        break;
    }

    return esp;
}
