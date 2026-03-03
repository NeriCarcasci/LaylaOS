#include "udp.h"

uint8_t UDPSocket::tx_buf[1472];

UDPSocket::UDPSocket(IPv4Handler* ipv4, uint16_t local_port)
    : ipv4(ipv4), local_port(local_port)
{
}

void UDPSocket::HandleIPv4Payload(uint8_t* payload, uint32_t size,
                                   IPv4Address src, IPv4Address dst,
                                   uint8_t protocol) {
    if (size < sizeof(UDPHeader)) return;
    UDPHeader* hdr = (UDPHeader*)payload;
    if (ntohs(hdr->dst_port) != local_port) return;
    uint32_t data_size = ntohs(hdr->length) - sizeof(UDPHeader);
    OnReceive(src, ntohs(hdr->src_port), payload + sizeof(UDPHeader), data_size);
}

bool UDPSocket::Send(IPv4Address dst_ip, uint16_t dst_port,
                     uint8_t* data, uint32_t size) {
    UDPHeader* hdr  = (UDPHeader*)tx_buf;
    hdr->src_port   = htons(local_port);
    hdr->dst_port   = htons(dst_port);
    hdr->length     = htons((uint16_t)(sizeof(UDPHeader) + size));
    hdr->checksum   = 0;
    for (uint32_t i = 0; i < size; i++) tx_buf[sizeof(UDPHeader) + i] = data[i];
    return ipv4->Send(dst_ip, IPV4_PROTO_UDP, tx_buf, sizeof(UDPHeader) + size);
}

void UDPSocket::OnReceive(IPv4Address src_ip, uint16_t src_port,
                          uint8_t* data, uint32_t size) {
}
