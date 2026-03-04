#include "icmp.h"

uint8_t ICMPHandler::tx_buf[64];

ICMPHandler::ICMPHandler(IPv4Handler* ipv4) : ipv4(ipv4) {
}

void ICMPHandler::HandleIPv4Payload(uint8_t* payload, uint32_t size,
                                     IPv4Address src, IPv4Address dst,
                                     uint8_t protocol) {
    if (size < sizeof(ICMPHeader)) return;
    ICMPHeader* hdr = (ICMPHeader*)payload;
    if (hdr->type != ICMP_ECHO_REQUEST) return;

    uint32_t reply_size = size;
    if (reply_size > sizeof(tx_buf)) reply_size = sizeof(tx_buf);

    for (uint32_t i = 0; i < reply_size; i++) tx_buf[i] = payload[i];

    ICMPHeader* reply = (ICMPHeader*)tx_buf;
    reply->type     = ICMP_ECHO_REPLY;
    reply->code     = 0;
    reply->checksum = 0;
    reply->checksum = InternetChecksum(tx_buf, reply_size);

    ipv4->Send(src, IPV4_PROTO_ICMP, tx_buf, reply_size);
}

bool ICMPHandler::SendEchoRequest(IPv4Address target, uint16_t id, uint16_t seq) {
    ICMPHeader* hdr = (ICMPHeader*)tx_buf;
    hdr->type       = ICMP_ECHO_REQUEST;
    hdr->code       = 0;
    hdr->checksum   = 0;
    hdr->identifier = htons(id);
    hdr->sequence   = htons(seq);
    hdr->checksum   = InternetChecksum(tx_buf, sizeof(ICMPHeader));
    return ipv4->Send(target, IPV4_PROTO_ICMP, tx_buf, sizeof(ICMPHeader));
}
