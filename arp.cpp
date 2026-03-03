#include "arp.h"

struct ARPCacheEntry {
    IPv4Address ip;
    MACAddress  mac;
    bool        valid;
};

static ARPCacheEntry arp_cache[128];

ARPHandler::ARPHandler(EthernetDriver* eth, IPv4Address local_ip)
    : eth(eth), local_ip(local_ip)
{
}

void ARPHandler::CacheSet(IPv4Address ip, MACAddress mac) {
    for (int i = 0; i < 128; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            arp_cache[i].mac = mac;
            return;
        }
    }
    for (int i = 0; i < 128; i++) {
        if (!arp_cache[i].valid) {
            arp_cache[i].ip    = ip;
            arp_cache[i].mac   = mac;
            arp_cache[i].valid = true;
            return;
        }
    }
}

bool ARPHandler::CacheGet(IPv4Address ip, MACAddress* out) {
    for (int i = 0; i < 128; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            *out = arp_cache[i].mac;
            return true;
        }
    }
    return false;
}

void ARPHandler::HandleEthernetPayload(uint8_t* payload, uint32_t size,
                                        MACAddress src, MACAddress dst) {
    if (size < sizeof(ARPPacket)) return;
    ARPPacket* pkt = (ARPPacket*)payload;
    CacheSet(pkt->sender_ip, pkt->sender_mac);
    if (ntohs(pkt->opcode) == ARP_REQUEST && pkt->target_ip == local_ip)
        SendReply(pkt);
}

void ARPHandler::SendRequest(IPv4Address target_ip) {
    ARPPacket pkt;
    pkt.hardware_type = htons(0x0001);
    pkt.protocol_type = htons(0x0800);
    pkt.hardware_size = 6;
    pkt.protocol_size = 4;
    pkt.opcode        = htons(ARP_REQUEST);
    pkt.sender_mac    = eth->GetMAC();
    pkt.sender_ip     = local_ip;
    MACAddress zero_mac;
    for (int i = 0; i < 6; i++) zero_mac.bytes[i] = 0;
    pkt.target_mac = zero_mac;
    pkt.target_ip  = target_ip;
    eth->Send(MACAddress::Broadcast(), ETHERTYPE_ARP,
              (uint8_t*)&pkt, sizeof(ARPPacket));
}

void ARPHandler::SendReply(ARPPacket* request) {
    ARPPacket reply;
    reply.hardware_type = htons(0x0001);
    reply.protocol_type = htons(0x0800);
    reply.hardware_size = 6;
    reply.protocol_size = 4;
    reply.opcode        = htons(ARP_REPLY);
    reply.sender_mac    = eth->GetMAC();
    reply.sender_ip     = local_ip;
    reply.target_mac    = request->sender_mac;
    reply.target_ip     = request->sender_ip;
    eth->Send(request->sender_mac, ETHERTYPE_ARP,
              (uint8_t*)&reply, sizeof(ARPPacket));
}

bool ARPHandler::Resolve(IPv4Address ip, MACAddress* out) {
    return CacheGet(ip, out);
}
