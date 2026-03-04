# Layla OS Booklet

This booklet is a guided course for your current kernel implementation.

Read in order.

## Phase 0 — Learning Method

| Chapter | Topic |
|---------|-------|
| [0.1](./0.1-how-to-learn-this-codebase.md) | How to learn this codebase effectively |

## Phase 1 — CPU Foundations

| Chapter | Topic |
|---------|-------|
| [1.1](./1.1-gdt.md) | Global Descriptor Table (GDT) |
| [1.2](./1.2-idt-and-pic.md) | Interrupts — IDT & PIC |
| [1.3](./1.3-tss-and-ring-transitions.md) | TSS and Ring transitions |

## Phase 2 — Hardware Communication

| Chapter | Topic |
|---------|-------|
| [2.1](./2.1-port-io.md) | Port I/O |
| [2.2](./2.2-keyboard-and-mouse.md) | PS/2 Keyboard & Mouse |
| [2.3](./2.3-pci-bus.md) | PCI Bus Enumeration |
| [2.4](./2.4-driver-model-and-device-binding.md) | Driver model and PCI device binding |

## Phase 3 — Boot and Runtime Wiring

| Chapter | Topic |
|---------|-------|
| [3.1](./3.1-boot-loader-linker-and-entry.md) | Boot loader, linker, and kernel entry |
| [3.2](./3.2-kernel-main-initialization-walkthrough.md) | Full `kernelMain` initialization order |

## Phase 4 — Memory System

| Chapter | Topic |
|---------|-------|
| [4.1](./4.1-kernel-heap-and-global-allocation.md) | Kernel heap and global `new/delete` |
| [4.2](./4.2-physical-memory-and-paging.md) | PMM and paging |

## Phase 5 — Processes and Syscalls

| Chapter | Topic |
|---------|-------|
| [5.1](./5.1-processes-scheduler-and-pit.md) | Process model, PIT, and scheduler |
| [5.2](./5.2-syscalls-and-ring3-shell.md) | Syscall ABI and Ring 3 shell |

## Phase 6 — Storage Stack

| Chapter | Topic |
|---------|-------|
| [6.1](./6.1-storage-stack-ata-mbr-fat32.md) | ATA → MBR → FAT32 |

## Phase 7 — Network Stack

| Chapter | Topic |
|---------|-------|
| [7.1](./7.1-network-stack-rtl8139-to-tcp.md) | RTL8139 through TCP and network syscalls |

## Phase 8 — UI and Input

| Chapter | Topic |
|---------|-------|
| [8.1](./8.1-vga-gui-terminal-and-input-path.md) | VGA, GUI, terminal, and input path |

## Phase 9 — Build, Debug, Next Steps

| Chapter | Topic |
|---------|-------|
| [9.1](./9.1-build-debug-and-next-milestones.md) | Debug workflow and practical roadmap |

---

If you want, the next expansion can be a **lab edition** with one hands-on exercise sheet per chapter and expected outputs for QEMU runs.
