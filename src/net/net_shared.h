#ifndef __NET_SHARED_H
#define __NET_SHARED_H

#include "types.h"

#define NET_SHARED_BASE_ADDR 0x00600000
#define NET_BUF_STATUS_ADDR  (NET_SHARED_BASE_ADDR + 0)
#define NET_BUF_DATA_LEN_ADDR (NET_SHARED_BASE_ADDR + 4)
#define NET_BUF_DATA_ADDR    (NET_SHARED_BASE_ADDR + 8)
#define NET_BUF_MAX_DATA     4088

#define NET_SHARED_BASE      NET_SHARED_BASE_ADDR
#define NET_BUF_STATUS       (*(volatile uint32_t*)NET_BUF_STATUS_ADDR)
#define NET_BUF_DATA_LEN     (*(volatile uint32_t*)NET_BUF_DATA_LEN_ADDR)
#define NET_BUF_DATA         ((uint8_t*)NET_BUF_DATA_ADDR)

#endif
