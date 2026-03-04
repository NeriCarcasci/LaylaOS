#ifndef __ICMP_H
#define __ICMP_H

#include "types.h"
#include "net.h"
#include "ipv4.h"

const uint8_t ICMP_ECHO_REQUEST = 8;
const uint8_t ICMP_ECHO_REPLY   = 0;

struct ICMPHeader {
    uint8_t  type;
    uint8_t  code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
} __attribute__((packed));

class ICMPHandler : public IPv4PayloadHandler {
public:
    ICMPHandler(IPv4Handler* ipv4);
    void HandleIPv4Payload(uint8_t* payload, uint32_t size,
                            IPv4Address src, IPv4Address dst,
                            uint8_t protocol) override;
    bool SendEchoRequest(IPv4Address target, uint16_t id, uint16_t seq);
private:
    IPv4Handler* ipv4;
    static uint8_t tx_buf[64];
};

#endif
