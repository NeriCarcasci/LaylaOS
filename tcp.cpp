#include "tcp.h"

uint8_t TCPSocket::rx_buf[8192];
uint8_t TCPSocket::tx_buf[1480];

TCPSocket::TCPSocket(IPv4Handler* ipv4, uint16_t local_port)
    : ipv4(ipv4), local_port(local_port),
      remote_port(0), state(TCP_CLOSED), seq(0), ack(0)
{
    IPv4Address zero = IPv4Address::FromBytes(0,0,0,0);
    remote_ip = zero;
}

TCPState TCPSocket::State() const { return state; }
void TCPSocket::OnConnected() {}
void TCPSocket::OnReceive(uint8_t* data, uint32_t size) {}
void TCPSocket::OnClosed() {}

uint16_t TCPSocket::ComputeChecksum(IPv4Address src, IPv4Address dst,
                                     uint8_t* segment, uint32_t seg_len) {
    static uint8_t scratch[1500];
    TCPPseudoHeader* pseudo = (TCPPseudoHeader*)scratch;
    pseudo->src        = src;
    pseudo->dst        = dst;
    pseudo->zero       = 0;
    pseudo->protocol   = IPV4_PROTO_TCP;
    pseudo->tcp_length = htons((uint16_t)seg_len);
    for (uint32_t i = 0; i < seg_len; i++)
        scratch[sizeof(TCPPseudoHeader) + i] = segment[i];
    return InternetChecksum(scratch, sizeof(TCPPseudoHeader) + seg_len);
}

void TCPSocket::SendSegment(uint8_t flags, uint8_t* data, uint32_t size) {
    TCPHeader* hdr   = (TCPHeader*)tx_buf;
    hdr->src_port    = htons(local_port);
    hdr->dst_port    = htons(remote_port);
    hdr->seq_num     = htonl(seq);
    hdr->ack_num     = htonl(ack);
    hdr->data_offset = (5 << 4);
    hdr->flags       = flags;
    hdr->window_size = htons(8192);
    hdr->checksum    = 0;
    hdr->urgent_ptr  = 0;
    for (uint32_t i = 0; i < size; i++) tx_buf[20 + i] = data[i];

    uint32_t seg_len = 20 + size;
    hdr->checksum = ComputeChecksum(ipv4->GetLocalIP(), remote_ip, tx_buf, seg_len);
    ipv4->Send(remote_ip, IPV4_PROTO_TCP, tx_buf, seg_len);

    if ((flags & TCP_SYN) || (flags & TCP_FIN))
        seq++;
    else
        seq += size;
}

bool TCPSocket::Connect(IPv4Address dst_ip, uint16_t dst_port) {
    remote_ip   = dst_ip;
    remote_port = dst_port;
    seq         = 0xDEADBEEF;
    ack         = 0;
    state       = TCP_SYN_SENT;
    SendSegment(TCP_SYN, nullptr, 0);
    return true;
}

bool TCPSocket::Send(uint8_t* data, uint32_t size) {
    if (state != TCP_ESTABLISHED) return false;
    SendSegment(TCP_PSH | TCP_ACK, data, size);
    return true;
}

void TCPSocket::Close() {
    if (state == TCP_ESTABLISHED) {
        state = TCP_FIN_WAIT_1;
        SendSegment(TCP_FIN | TCP_ACK, nullptr, 0);
    }
}

void TCPSocket::HandleIPv4Payload(uint8_t* payload, uint32_t size,
                                   IPv4Address src, IPv4Address dst,
                                   uint8_t protocol) {
    if (size < 20) return;
    TCPHeader* hdr = (TCPHeader*)payload;
    if (ntohs(hdr->dst_port) != local_port) return;

    uint32_t in_seq = ntohl(hdr->seq_num);
    uint32_t in_ack = ntohl(hdr->ack_num);
    uint8_t  flags  = hdr->flags;
    uint32_t hdr_len     = (hdr->data_offset >> 4) * 4;
    uint32_t payload_size = (size > hdr_len) ? (size - hdr_len) : 0;
    uint8_t* data = payload + hdr_len;

    switch (state) {
    case TCP_SYN_SENT:
        if ((flags & TCP_SYN) && (flags & TCP_ACK)) {
            ack   = in_seq + 1;
            seq   = in_ack;
            state = TCP_ESTABLISHED;
            SendSegment(TCP_ACK, nullptr, 0);
            OnConnected();
        }
        break;

    case TCP_ESTABLISHED:
        if (flags & TCP_FIN) {
            ack = in_seq + 1;
            SendSegment(TCP_ACK, nullptr, 0);
            SendSegment(TCP_FIN | TCP_ACK, nullptr, 0);
            state = TCP_LAST_ACK;
        } else if (payload_size > 0) {
            ack = in_seq + payload_size;
            uint32_t copy = payload_size;
            if (copy > sizeof(rx_buf)) copy = sizeof(rx_buf);
            for (uint32_t i = 0; i < copy; i++) rx_buf[i] = data[i];
            SendSegment(TCP_ACK, nullptr, 0);
            OnReceive(rx_buf, copy);
        }
        break;

    case TCP_LAST_ACK:
        if (flags & TCP_ACK) {
            state = TCP_CLOSED;
            OnClosed();
        }
        break;

    case TCP_FIN_WAIT_1:
        if (flags & TCP_ACK)
            state = TCP_FIN_WAIT_2;
        break;

    case TCP_FIN_WAIT_2:
        if (flags & TCP_FIN) {
            ack = in_seq + 1;
            SendSegment(TCP_ACK, nullptr, 0);
            state = TCP_CLOSED;
            OnClosed();
        }
        break;

    default:
        break;
    }
}
