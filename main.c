#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

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

uint32_t parse_mask(char *mask) {
    if (strcmp(mask, "32") == 0) return 0xFFFFFFFF;
    if (strcmp(mask, "24") == 0) return 0xFFFFFF00;
    if (strcmp(mask, "16") == 0) return 0xFFFF0000;
    if (strcmp(mask, "8") == 0) return 0xFF000000;

    return 0;
}

uint8_t parse_protocol(char *protocol_str) {
    if (strcmp(protocol_str, PROTOCOL_TCP) == 0) return 6;
    if (strcmp(protocol_str, PROTOCOL_UDP) == 0) return 17;

    return 0;
}

uint16_t parse_port(char *port_str) {
    char *endptr;
    uint16_t port = strtol(port_str, &endptr, 10);
    if (port_str == endptr || *endptr != '\0') return 0;

    return port;
}

int parse_port_filter(char *token, struct port_filter *filter) {
    if (!token) return -1;
    filter->protocol = parse_protocol(token);
    if (filter->protocol == 0) return -1;

    token = strtok(NULL, ":");
    if (!token) return -1;

    filter->port = parse_port(token);
    if (filter->port == 0) return -1;

    return 0;
}

int parse_netmask_filter(char *token, struct netmask_filter *filter) {
    if (!token) return -1;
    char *netmask = strtok(token, "/");

    filter->address = inet_addr(netmask);
    if (filter->address == INADDR_NONE) return -1;
    
    netmask = strtok(NULL, "/");
    if (!netmask) return -1;

    filter->mask = parse_mask(netmask);
    if (filter->mask < 0) return -1;

    return 0;
}

struct filter *read_and_parse(char *path, size_t *filters_len) {
    FILE *fp = fopen(path, "r");
    if (fp == NULL) return NULL;

    char *buf = NULL;
    size_t buf_len = 0;

    struct filter *filters = NULL;
    while (getline(&buf, &buf_len, fp) != -1) {
        buf[strcspn(buf, "\n")] = 0;

        char *token = strtok(buf, ":");
        if (strcmp(token, PORT_FILTER) == 0) {
            token = strtok(NULL, ":");

            struct port_filter filter;
            if (parse_port_filter(token, &filter) < 0) {
                goto error;
            }

            struct filter *tmp = realloc(filters, sizeof(struct filter)*(*filters_len+1));
            if (!tmp) goto error;
            filters = tmp;
            (*filters_len)++;

            filters[*filters_len-1] = (struct filter){
                .type = SET_PORT_FILTER,
                .port = filter,
            };
        } else if (strcmp(token, NETMASK_FILTER) == 0) {
            token = strtok(NULL, ":");

            struct netmask_filter filter;
            if (parse_netmask_filter(token, &filter) < 0) {
                goto error;
            }

            struct filter *tmp = realloc(filters, sizeof(struct filter)*(*filters_len+1));
            if (!tmp) goto error;
            filters = tmp;
            (*filters_len)++;

            filters[*filters_len-1] = (struct filter){
                .type = SET_NETMASK_FILTER,
                .netmask = filter,
            };
        }
    }
    free(buf);
    fclose(fp);

    return filters;

    error:
    free(filters);
    free(buf);
    fclose(fp);

    return NULL;
}

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
        
        char *buf = NULL;
        size_t len = build_set_packet(f, &buf);
        if (len < 0) return -1;

        sendto(s, buf, len, 0, (struct sockaddr *)&sin, sizeof(sin));
        free(buf);
    }
    close(s);
    
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("error: invalid format\nformat: %s [add/remove] [filter path]\n", argv[0]);
        return 1;
    }

    char *action = argv[1], *path = argv[2];
    if (strcmp(action, ACTION_ADD) == 0) {
        size_t filters_len = 0;
        struct filter *filters = read_and_parse(path, &filters_len);
        if (!filters) {
            printf("error: read and parse error\n");
            return 1;
        }

        if (build_and_send(filters, filters_len) < 0) {
            printf("error: build and send error\n");
            return 1;
        }
        free(filters);        
    } else if (strcmp(action, ACTION_REMOVE) == 0) {
        // TODO: 제거 관련 코드 작성하기
    } else {
        printf("error: unknown action\nvalid actions: add, remove\n");

        return 1;
    }
}
