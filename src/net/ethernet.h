#ifndef __ETHERNET_H
#define __ETHERNET_H

#include "types.h"
#include "net.h"
#include "driver.h"

const uint16_t ETHERTYPE_IPV4 = 0x0800;
const uint16_t ETHERTYPE_ARP  = 0x0806;

struct EthernetFrame {
    MACAddress dst;
    MACAddress src;
    uint16_t   ethertype;
    uint8_t    payload[1500];
} __attribute__((packed));

class EthernetPayloadHandler {
public:
    virtual void HandleEthernetPayload(uint8_t* payload, uint32_t size,
                                        MACAddress src, MACAddress dst) = 0;
};

class EthernetDriver : public Driver {
public:
    EthernetDriver();
    virtual bool       Send(MACAddress dst, uint16_t ethertype,
                            uint8_t* payload, uint32_t size) = 0;
    virtual MACAddress GetMAC() = 0;
    void RegisterHandler(uint16_t ethertype, EthernetPayloadHandler* handler);
protected:
    void Dispatch(uint8_t* frame_data, uint32_t size);
private:
    static const int MAX_HANDLERS = 8;
    struct HandlerEntry {
        uint16_t                ethertype;
        EthernetPayloadHandler* handler;
    };
    HandlerEntry handlers[MAX_HANDLERS];
    int          handler_count;
};

#endif
