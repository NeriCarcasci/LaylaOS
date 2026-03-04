#ifndef __NET_HOOKS_H
#define __NET_HOOKS_H

#include "udp.h"
#include "tcp.h"

class SyscallUDPSocket : public UDPSocket {
public:
    SyscallUDPSocket(IPv4Handler* ipv4, uint16_t local_port);
    void OnReceive(IPv4Address src_ip, uint16_t src_port,
                   uint8_t* data, uint32_t size) override;
};

class SyscallTCPSocket : public TCPSocket {
public:
    SyscallTCPSocket(IPv4Handler* ipv4, uint16_t local_port);
    void OnReceive(uint8_t* data, uint32_t size) override;
};

#endif
