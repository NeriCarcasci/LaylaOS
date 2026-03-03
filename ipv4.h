#ifndef __IPV4_H
#define __IPV4_H

#include "types.h"
#include "net.h"
#include "ethernet.h"
#include "arp.h"

const uint8_t IPV4_PROTO_ICMP = 0x01;
const uint8_t IPV4_PROTO_TCP  = 0x06;
const uint8_t IPV4_PROTO_UDP  = 0x11;

struct IPv4Header {
    uint8_t     version_ihl;
    uint8_t     dscp_ecn;
    uint16_t    total_length;
    uint16_t    identification;
    uint16_t    flags_fragment;
    uint8_t     ttl;
    uint8_t     protocol;
    uint16_t    checksum;
    IPv4Address src;
    IPv4Address dst;
} __attribute__((packed));

class IPv4PayloadHandler {
public:
    virtual void HandleIPv4Payload(uint8_t* payload, uint32_t size,
                                    IPv4Address src, IPv4Address dst,
                                    uint8_t protocol) = 0;
};

class IPv4Handler : public EthernetPayloadHandler {
public:
    IPv4Handler(EthernetDriver* eth, ARPHandler* arp, IPv4Address local_ip);
    void HandleEthernetPayload(uint8_t* payload, uint32_t size,
                                MACAddress src, MACAddress dst) override;
    bool Send(IPv4Address dst_ip, uint8_t protocol,
              uint8_t* payload, uint32_t size);
    void RegisterHandler(uint8_t protocol, IPv4PayloadHandler* handler);
    IPv4Address GetLocalIP() const;
private:
    EthernetDriver* eth;
    ARPHandler*     arp;
    IPv4Address     local_ip;
    uint16_t        id_counter;

    static const int MAX_HANDLERS = 4;
    struct ProtoEntry { uint8_t protocol; IPv4PayloadHandler* handler; };
    ProtoEntry handlers[MAX_HANDLERS];
    int        handler_count;

    static uint8_t tx_buf[1500];
};

#endif
