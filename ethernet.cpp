#include "ethernet.h"

EthernetDriver::EthernetDriver() : handler_count(0) {
    for (int i = 0; i < MAX_HANDLERS; i++) {
        handlers[i].ethertype = 0;
        handlers[i].handler   = nullptr;
    }
}

void EthernetDriver::RegisterHandler(uint16_t ethertype, EthernetPayloadHandler* handler) {
    if (handler_count < MAX_HANDLERS) {
        handlers[handler_count].ethertype = ethertype;
        handlers[handler_count].handler   = handler;
        handler_count++;
    }
}

void EthernetDriver::Dispatch(uint8_t* frame_data, uint32_t size) {
    if (size < 14) return;
    EthernetFrame* frame = (EthernetFrame*)frame_data;
    uint16_t et = ntohs(frame->ethertype);
    uint32_t payload_size = size - 14;
    for (int i = 0; i < handler_count; i++) {
        if (handlers[i].ethertype == et && handlers[i].handler != nullptr)
            handlers[i].handler->HandleEthernetPayload(
                frame->payload, payload_size, frame->src, frame->dst);
    }
}
