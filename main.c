#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "cache.h"
#include "filter.h"
#include "parser.h"

size_t build_set_packet(struct filter *f, char **buf) {
    switch (f->type) {
        case SET_PORT_FILTER:
        case REMOVE_PORT_FILTER: {
            *buf = malloc(4);
            if (!*buf) return -1;

            **buf = f->type;
            *(*buf+1) = f->port.protocol;

            uint16_t port = htons(f->port.port);
            memcpy(*buf+2, &port, 2);

            return 4;
        }
        case SET_NETMASK_FILTER:
        case REMOVE_NETMASK_FILTER: {
            *buf = malloc(9);
            if (!*buf) return -1;
            
            **buf = f->type;

            uint32_t address = htonl(f->netmask.address);
            memcpy(*buf+1, &address, 4);

            uint32_t mask = htonl(f->netmask.mask);
            memcpy(*buf+5, &mask, 4);

            return 9;
        }
    };

    return -1;
}

int build_and_send(struct filter *filters, size_t filters_len) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return s;

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(209);
    inet_pton(AF_INET, "127.0.0.1", &sin.sin_addr);

    for (int i = 0; i < filters_len; i++) {
        struct filter *f = &filters[i];
        if (add_filter_cache(f) < 0) {
            close(s);
            return -1;
        }
        
        char *buf = NULL;
        size_t len = build_set_packet(f, &buf);
        if (len < 0) return -1;

        sendto(s, buf, len, 0, (struct sockaddr *)&sin, sizeof(sin));
        free(buf);
    }
    close(s);
    
    return 0;
}

int read_and_print_filters() {
    size_t len = 0;
    struct filter *filters = read_cache(&len);
    if (!filters) return -1;

    for (int i = 0; i < len; i++) {
        struct filter *f = &filters[i];
        switch (f->type) {
            case SET_PORT_FILTER: {
                char *protocol;
                switch (f->port.protocol) {
                    case 6: {
                        protocol = "tcp";
                        break;
                    }

                    case 17: {
                        protocol = "udp";
                        break;
                    }
                }

                printf("port:%s:%d\n", protocol, f->port.port);
                break;
            }
            case SET_NETMASK_FILTER: {
                char address[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &f->netmask.address, address, sizeof(address));
                printf("netmask:%s%s\n", address, parse_mask_to_str(f->netmask.mask));

                break;
            }
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("error: invalid format\nformat: %s [add, remove, filters]\n", argv[0]);
        return 1;
    }

    char *action = argv[1];
    if (strcmp(action, ACTION_ADD) == 0) {
        if (argc < 3) {
            printf("error: invalid format\nformat: %s add [filter_path]\n", argv[0]);
            return 1;
        }

        size_t filters_len = 0;
        struct filter *filters = read_and_parse(argv[2], &filters_len);
        if (!filters) {
            printf("error: read and parse error\n");
            return 1;
        }

        if (build_and_send(filters, filters_len) < 0) {
            printf("error: build and send error\n");
            return 1;
        }

        free(filters);
    } else if (strcmp(action, ACTION_FILTERS) == 0) {
        read_and_print_filters();
    } else {
        printf("error: unknown action\ninvalid actions: add, remove, filters\n");

        return 1;
    }
}
