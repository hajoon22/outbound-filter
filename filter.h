#ifndef FILTER_H
#define FILTER_H

#include <stdint.h>

#define PORT_FILTER_STR "port"
#define NETMASK_FILTER_STR "netmask"

#define PROTOCOL_UDP "udp"
#define PROTOCOL_TCP "tcp"

enum filter_type {
    PORT_FILTER = 0,
    NETMASK_FILTER = 2,
};

struct port_filter {
    uint8_t protocol;
    uint16_t port;
};

struct netmask_filter {
    uint32_t address;
    uint32_t mask;
};

struct filter {
    enum filter_type type;
    union {
        struct port_filter port;
        struct netmask_filter netmask;
    };
};

#endif
