#ifndef __TCP_H
#define __TCP_H

#include "types.h"
#include "net.h"
#include "ipv4.h"

struct TCPHeader {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq_num;
    uint32_t ack_num;
    uint8_t  data_offset;
    uint8_t  flags;
    uint16_t window_size;
    uint16_t checksum;
    uint16_t urgent_ptr;
} __attribute__((packed));

struct TCPPseudoHeader {
    IPv4Address src;
    IPv4Address dst;
    uint8_t     zero;
    uint8_t     protocol;
    uint16_t    tcp_length;
} __attribute__((packed));

const uint8_t TCP_FIN = 0x01;
const uint8_t TCP_SYN = 0x02;
const uint8_t TCP_RST = 0x04;
const uint8_t TCP_PSH = 0x08;
const uint8_t TCP_ACK = 0x10;

enum TCPState {
    TCP_CLOSED, TCP_LISTEN, TCP_SYN_SENT, TCP_SYN_RECEIVED,
    TCP_ESTABLISHED, TCP_FIN_WAIT_1, TCP_FIN_WAIT_2,
    TCP_CLOSE_WAIT, TCP_LAST_ACK, TCP_TIME_WAIT
};

class TCPSocket : public IPv4PayloadHandler {
public:
    TCPSocket(IPv4Handler* ipv4, uint16_t local_port);
    void HandleIPv4Payload(uint8_t* payload, uint32_t size,
                            IPv4Address src, IPv4Address dst,
                            uint8_t protocol) override;
    bool     Connect(IPv4Address dst_ip, uint16_t dst_port);
    bool     Send(uint8_t* data, uint32_t size);
    void     Close();
    TCPState State() const;
    virtual void OnConnected();
    virtual void OnReceive(uint8_t* data, uint32_t size);
    virtual void OnClosed();
private:
    IPv4Handler* ipv4;
    uint16_t     local_port;
    uint16_t     remote_port;
    IPv4Address  remote_ip;
    TCPState     state;
    uint32_t     seq;
    uint32_t     ack;

    static uint8_t rx_buf[8192];
    static uint8_t tx_buf[1480];

    void     SendSegment(uint8_t flags, uint8_t* data, uint32_t size);
    uint16_t ComputeChecksum(IPv4Address src, IPv4Address dst,
                              uint8_t* segment, uint32_t seg_len);
};

#endif
