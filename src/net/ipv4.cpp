#include "ipv4.h"

uint8_t IPv4Handler::tx_buf[1500];

IPv4Handler::IPv4Handler(EthernetDriver* eth, ARPHandler* arp, IPv4Address local_ip)
    : eth(eth), arp(arp), local_ip(local_ip), id_counter(0), handler_count(0)
{
    for (int i = 0; i < MAX_HANDLERS; i++) {
        handlers[i].protocol = 0;
        handlers[i].handler  = nullptr;
    }
}

IPv4Address IPv4Handler::GetLocalIP() const {
    return local_ip;
}

void IPv4Handler::RegisterHandler(uint8_t protocol, IPv4PayloadHandler* handler) {
    if (handler_count < MAX_HANDLERS) {
        handlers[handler_count].protocol = protocol;
        handlers[handler_count].handler  = handler;
        handler_count++;
    }
}

void IPv4Handler::HandleEthernetPayload(uint8_t* payload, uint32_t size,
                                         MACAddress src, MACAddress dst) {
    if (size < sizeof(IPv4Header)) return;
    IPv4Header* hdr = (IPv4Header*)payload;
    if ((hdr->version_ihl >> 4) != 4) return;
    if (!(hdr->dst == local_ip)) return;
    if (InternetChecksum(hdr, 20) != 0) return;

    uint32_t payload_size = ntohs(hdr->total_length) - 20;
    for (int i = 0; i < handler_count; i++) {
        if (handlers[i].protocol == hdr->protocol && handlers[i].handler != nullptr)
            handlers[i].handler->HandleIPv4Payload(
                payload + 20, payload_size, hdr->src, hdr->dst, hdr->protocol);
    }
}

bool IPv4Handler::Send(IPv4Address dst_ip, uint8_t protocol,
                        uint8_t* payload, uint32_t size) {
    MACAddress dst_mac;
    if (!arp->Resolve(dst_ip, &dst_mac)) return false;

    IPv4Header* hdr = (IPv4Header*)tx_buf;
    hdr->version_ihl    = 0x45;
    hdr->dscp_ecn       = 0;
    hdr->total_length   = htons((uint16_t)(20 + size));
    hdr->identification = htons(id_counter++);
    hdr->flags_fragment = 0;
    hdr->ttl            = 64;
    hdr->protocol       = protocol;
    hdr->checksum       = 0;
    hdr->src            = local_ip;
    hdr->dst            = dst_ip;
    hdr->checksum       = InternetChecksum(hdr, 20);

    for (uint32_t i = 0; i < size; i++)
        tx_buf[20 + i] = payload[i];

    return eth->Send(dst_mac, ETHERTYPE_IPV4, tx_buf, 20 + size);
}
