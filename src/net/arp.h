#ifndef __ARP_H
#define __ARP_H

#include "types.h"
#include "net.h"
#include "ethernet.h"

const uint16_t ARP_REQUEST = 0x0001;
const uint16_t ARP_REPLY   = 0x0002;

struct ARPPacket {
    uint16_t    hardware_type;
    uint16_t    protocol_type;
    uint8_t     hardware_size;
    uint8_t     protocol_size;
    uint16_t    opcode;
    MACAddress  sender_mac;
    IPv4Address sender_ip;
    MACAddress  target_mac;
    IPv4Address target_ip;
} __attribute__((packed));

class ARPHandler : public EthernetPayloadHandler {
public:
    ARPHandler(EthernetDriver* eth, IPv4Address local_ip);
    void HandleEthernetPayload(uint8_t* payload, uint32_t size,
                                MACAddress src, MACAddress dst) override;
    bool Resolve(IPv4Address ip, MACAddress* out);
    void SendRequest(IPv4Address target_ip);
private:
    EthernetDriver* eth;
    IPv4Address     local_ip;
    void SendReply(ARPPacket* request);
    void CacheSet(IPv4Address ip, MACAddress mac);
    bool CacheGet(IPv4Address ip, MACAddress* out);
};

#endif
