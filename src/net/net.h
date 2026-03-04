#ifndef __NET_H
#define __NET_H

#include "types.h"

static inline uint16_t htons(uint16_t x) {
    return (x >> 8) | (x << 8);
}
static inline uint32_t htonl(uint32_t x) {
    return ((x & 0xFF000000) >> 24)
         | ((x & 0x00FF0000) >>  8)
         | ((x & 0x0000FF00) <<  8)
         | ((x & 0x000000FF) << 24);
}
static inline uint16_t ntohs(uint16_t x) { return htons(x); }
static inline uint32_t ntohl(uint32_t x) { return htonl(x); }

struct MACAddress {
    uint8_t bytes[6];
    bool operator==(const MACAddress& o) const {
        for (int i = 0; i < 6; i++)
            if (bytes[i] != o.bytes[i]) return false;
        return true;
    }
    bool IsBroadcast() const {
        for (int i = 0; i < 6; i++)
            if (bytes[i] != 0xFF) return false;
        return true;
    }
    static MACAddress Broadcast() {
        MACAddress m;
        for (int i = 0; i < 6; i++) m.bytes[i] = 0xFF;
        return m;
    }
} __attribute__((packed));

struct IPv4Address {
    uint8_t bytes[4];
    bool operator==(const IPv4Address& o) const {
        return bytes[0] == o.bytes[0] && bytes[1] == o.bytes[1]
            && bytes[2] == o.bytes[2] && bytes[3] == o.bytes[3];
    }
    static IPv4Address FromBytes(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
        IPv4Address ip;
        ip.bytes[0] = a; ip.bytes[1] = b;
        ip.bytes[2] = c; ip.bytes[3] = d;
        return ip;
    }
} __attribute__((packed));

static inline uint16_t InternetChecksum(const void* data, uint32_t length) {
    uint32_t sum = 0;
    const uint16_t* ptr = (const uint16_t*)data;
    while (length > 1) { sum += *ptr++; length -= 2; }
    if (length) sum += *(const uint8_t*)ptr;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~(uint16_t)sum;
}

#endif
