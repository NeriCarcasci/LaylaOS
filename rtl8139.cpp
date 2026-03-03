#include "rtl8139.h"
#include "vga.h"

static uint8_t rx_buffer[8192 + 16 + 1500];
static uint8_t tx_buffers[4][1536];
static int     tx_current;

RTL8139Driver::RTL8139Driver(PCIDeviceDescriptor& desc, InterruptManager* interrupts)
    : InterruptHandler(interrupts->HardwareInterruptOffset() + desc.interrupt, interrupts),
      cr_port     (desc.port_base[0] + 0x37),
      isr_port    (desc.port_base[0] + 0x3E),
      imr_port    (desc.port_base[0] + 0x3C),
      rbstart_port(desc.port_base[0] + 0x30),
      tcr_port    (desc.port_base[0] + 0x40),
      rcr_port    (desc.port_base[0] + 0x44),
      tsd         { Port32Bit(desc.port_base[0] + 0x10),
                    Port32Bit(desc.port_base[0] + 0x14),
                    Port32Bit(desc.port_base[0] + 0x18),
                    Port32Bit(desc.port_base[0] + 0x1C) },
      tsad        { Port32Bit(desc.port_base[0] + 0x20),
                    Port32Bit(desc.port_base[0] + 0x24),
                    Port32Bit(desc.port_base[0] + 0x28),
                    Port32Bit(desc.port_base[0] + 0x2C) },
      config1_port(desc.port_base[0] + 0x50),
      capr_port   (desc.port_base[0] + 0x3A),
      io_base     ((uint16_t)desc.port_base[0]),
      rx_read_ptr(0)
{
    tx_current = 0;
}

void RTL8139Driver::HwReset() {
    cr_port.Write(0x10);
    while (cr_port.Read() & 0x10);
}

void RTL8139Driver::ReadMAC() {
    for (int i = 0; i < 6; i++) {
        Port8Bit p(io_base + i);
        mac.bytes[i] = p.Read();
    }
}

void RTL8139Driver::Activate() {
    HwReset();
    ReadMAC();
    rbstart_port.Write((uint32_t)rx_buffer);
    config1_port.Write(0x00);
    cr_port.Write(0x0C);
    rcr_port.Write(0x0F | (1 << 7));
    tcr_port.Write(0x00);
    imr_port.Write(0x0005);

    VGA::Print("RTL8139: MAC ");
    for (int i = 0; i < 6; i++) {
        VGA::PrintHex(mac.bytes[i]);
        if (i < 5) VGA::Print(":");
    }
    VGA::Print("\n");
}

bool RTL8139Driver::Send(MACAddress dst, uint16_t ethertype,
                          uint8_t* payload, uint32_t size) {
    uint8_t* buf = tx_buffers[tx_current];
    for (int i = 0; i < 6; i++) buf[i]     = dst.bytes[i];
    MACAddress src = GetMAC();
    for (int i = 0; i < 6; i++) buf[6 + i] = src.bytes[i];
    buf[12] = (ethertype >> 8) & 0xFF;
    buf[13] =  ethertype       & 0xFF;
    for (uint32_t i = 0; i < size; i++) buf[14 + i] = payload[i];

    uint32_t frame_size = 14 + size;
    if (frame_size < 60) frame_size = 60;

    tsad[tx_current].Write((uint32_t)buf);
    tsd[tx_current].Write(frame_size);

    tx_current = (tx_current + 1) % 4;
    return true;
}

MACAddress RTL8139Driver::GetMAC() {
    return mac;
}

uint32_t RTL8139Driver::HandleInterrupt(uint32_t esp) {
    uint16_t status = isr_port.Read();
    isr_port.Write(status);
    if (status & 0x01) HandleReceive();
    return esp;
}

void RTL8139Driver::HandleReceive() {
    while (!(cr_port.Read() & 0x01)) {
        uint8_t*  entry  = rx_buffer + (rx_read_ptr % 8192);
        uint16_t  status = *(uint16_t*)(entry + 0);
        uint16_t  length = *(uint16_t*)(entry + 2);

        if (status & 0x01)
            Dispatch(entry + 4, length - 4);

        rx_read_ptr = (rx_read_ptr + length + 4 + 3) & ~3u;
        capr_port.Write((uint16_t)((rx_read_ptr - 16) % 8192));
    }
}
