#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "filter.h"

char *parse_mask_to_str(uint32_t mask) {
    switch (mask) {
        case 0xFFFFFFFF: return "/32";
        case 0xFFFFFF00: return "/24";
        case 0xFFFF0000: return "/16";
        case 0xF0000000: return "/8";
    }

    return "unknown";
}

uint32_t parse_mask_to_32(char *mask) {
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

    filter->mask = parse_mask_to_32(netmask);
    if (filter->mask < 0) return -1;

    return 0;
}

struct filter *read_and_parse(char *path, size_t *filters_len) {
    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;

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
