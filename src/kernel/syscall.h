#ifndef __SYSCALL_H
#define __SYSCALL_H

#include "types.h"

class RTL8139Driver;
class UDPSocket;
class TCPSocket;
class IPv4Handler;

extern RTL8139Driver* global_nic;
extern UDPSocket* global_udp;
extern TCPSocket* global_tcp;
extern IPv4Handler* global_ipv4;

uint32_t SyscallDispatch(uint32_t esp);

#endif
