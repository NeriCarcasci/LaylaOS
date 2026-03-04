#ifndef __UDP_H
#define __UDP_H

#include "types.h"
#include "net.h"
#include "ipv4.h"

struct UDPHeader {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed));

class UDPSocket : public IPv4PayloadHandler {
public:
    UDPSocket(IPv4Handler* ipv4, uint16_t local_port);
    void HandleIPv4Payload(uint8_t* payload, uint32_t size,
                            IPv4Address src, IPv4Address dst,
                            uint8_t protocol) override;
    bool Send(IPv4Address dst_ip, uint16_t dst_port, uint16_t src_port,
              uint8_t* data, uint32_t size);
    virtual void OnReceive(IPv4Address src_ip, uint16_t src_port,
                           uint8_t* data, uint32_t size);
private:
    IPv4Handler* ipv4;
    uint16_t     local_port;
    static uint8_t tx_buf[1472];
};

#endif
