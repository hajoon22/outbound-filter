#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>

#define ACTION_ADD "add"
#define ACTION_REMOVE "remove"

#define PORT_FILTER "port"
#define NETMASK_FILTER "netmask"

#define PROTOCOL_UDP "udp"
#define PROTOCOL_TCP "tcp"

enum filter_type {
    FILTER_PORT,
    FILTER_NETMASK,
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

    token = strtok(NULL, ",");
    if (!token) return -1;

    filter->port = parse_port(token);
    if (filter->port == 0) return -1;

    return 0;
}

int parse_netmask_filter(char *token, struct netmask_filter *filter) {
    if (!token) return -1;
    char *netmask = strtok(token, "/");

    filter->address = inet_addr(netmask);
    if (filter->address < 0) return -1;
    
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
        char *token = strtok(buf, ":");
        if (strcmp(token, PORT_FILTER) == 0) {
            token = strtok(NULL, ",");

            struct port_filter filter;
            if (parse_port_filter(token, &filter) < 0) {
                goto error;
            }

            struct filter *tmp = realloc(filters, sizeof(struct filter)*(*filters_len+1));
            if (!tmp) goto error;
            filters = tmp;
            (*filters_len)++;

            filters[*filters_len-1] = (struct filter){
                .type = FILTER_PORT,
                .port = filter,
            };
        } else if (strcmp(token, NETMASK_FILTER) == 0) {
            token = strtok(NULL, ",");

            struct netmask_filter filter;
            if (parse_netmask_filter(token, &filter) < 0) {
                goto error;
            }

            struct filter *tmp = realloc(filters, sizeof(struct filter)*(*filters_len+1));
            if (!tmp) goto error;
            filters = tmp;
            (*filters_len)++;

            filters[*filters_len-1] = (struct filter){
                .type = FILTER_NETMASK,
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

        
    } else if (strcmp(action, ACTION_REMOVE) == 0) {
        // TODO: 제거 관련 코드 작성하기
    } else {
        printf("error: unknown action\nvalid actions: add, remove\n");

        return 1;
    }
}
