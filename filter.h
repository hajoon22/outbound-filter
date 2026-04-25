#ifndef FILTER_H
#define FILTER_H

#include <stdint.h>

#define ACTION_ADD "add"
#define ACTION_REMOVE "remove"

#define PORT_FILTER "port"
#define NETMASK_FILTER "netmask"

#define PROTOCOL_UDP "udp"
#define PROTOCOL_TCP "tcp"

enum filter_type {
    SET_PORT_FILTER = 0,
    REMOVE_PORT_FILTER = 1,

    SET_NETMASK_FILTER = 2,
    REMOVE_NETMASK_FILTER = 3,
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
