#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "../filter.h"

char *parse_mask_to_str(uint32_t mask) {
    char *buf = malloc(8);
    if (!buf) return NULL;

    int mask_int = __builtin_popcount(mask);
    snprintf(buf, 8, "/%d", mask_int);

    return buf;
}
 
int parse_mask_to_32(char *mask_str, uint32_t *result) {
    int mask = atoi(mask_str);
    if (mask <= 0 || mask > 32) {
        return ERROR_INVALID_NETMASK;
    }

    for (int i = 0; i < mask/8; i++) {
        *result |= (0xFF << (24-i*8));
    }

    if (mask%8 > 0) {
        *result |= (((0xFF << (8-mask%8)) & 0xFF) << (24-(mask/8)*8));
    }

    return 0;
}

uint8_t parse_protocol(char *protocol_str) {
    if (strcmp(protocol_str, "tcp") == 0) return 6;
    if (strcmp(protocol_str, "udp") == 0) return 17;
    if (strcmp(protocol_str, "icmp") == 0) return 1;
    
    return 0;
}

int parse_port(char *port_str) {
    char *endptr;
    long port = strtol(port_str, &endptr, 10);
    if (port_str == endptr || *endptr != '\0') {
        return ERROR_INVALID_PORT;
    } else if (port < 1 || port > 65535) {
        return ERROR_INVALID_PORT;
    }

    return port;
}


int parse_port_filter(char *token, struct port_filter *filter) {
    if (!token) return -1;
    filter->protocol = parse_protocol(token);
    if (filter->protocol == 0) return ERROR_INVALID_PROTOCOL;

    token = strtok(NULL, ":");
    if (!token) return ERROR_INVALID_FORMAT;

    int n = parse_port(token);
    if (n < 0) return n;
    
    filter->port = (uint16_t)n;
    
    return 0;
}

int parse_netmask_filter(char *token, struct netmask_filter *filter) {
    if (!token) return -1;
    char *netmask = strtok(token, "/");

    filter->address = inet_addr(netmask);
    if (filter->address == INADDR_NONE) return ERROR_INVALID_ADDRESS;
    
    netmask = strtok(NULL, "/");
    if (!netmask) return ERROR_INVALID_NETMASK;

    uint32_t mask = 0;
    int n = parse_mask_to_32(netmask, &mask);
    if (n < 0) return n;
    
    filter->mask = mask;
    
    return 0;
}

char *read_and_parse_signature(char *path, size_t *len) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }

    long size = ftell(fp);
    if (size < 0) {
        fclose(fp);
        return NULL;
    }
    *len = (size_t)size;
    rewind(fp);


    char *buffer = malloc(size);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }

    if (fread(buffer, 1, size, fp) != size) {
        free(buffer);
        fclose(fp);
        return NULL;
    }
    fclose(fp);

    return buffer;
}

// signature:path
// [type(1byte)][signature[nbyte]]
int parse_signature_filter(char *token, struct signature_filter *filter) {
    if (!token) return -1;
    
    size_t signature_len = 0;
    filter->signature = read_and_parse_signature(token, &signature_len);
    if (!filter->signature) {
        return -1; // READ_ERROR
    }
    filter->signature_len = signature_len;

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
        if (strcmp(token, PORT_FILTER_STR) == 0) {
            token = strtok(NULL, ":");

            struct port_filter filter;
            switch (parse_port_filter(token, &filter)) {
                case ERROR_INVALID_PORT:
                printf("error: parse port filter: invalid port\n");
                goto error;
                case ERROR_INVALID_PROTOCOL:
                printf("error: parse port filter: invalid protocol\n");
                goto error;
                case ERROR_INVALID_FORMAT:
                printf("error: parse port filter: invalid format\n");
                goto error;
            }

            struct filter *tmp = realloc(filters, sizeof(struct filter)*(*filters_len+1));
            if (!tmp) goto error;
            filters = tmp;
            (*filters_len)++;

            filters[*filters_len-1] = (struct filter){
                .type = PORT_FILTER,
                .port = filter,
            };
        } else if (strcmp(token, NETMASK_FILTER_STR) == 0) {
            token = strtok(NULL, ":");

            struct netmask_filter filter;
            switch (parse_netmask_filter(token, &filter)) {
                case ERROR_INVALID_ADDRESS:
                printf("error: parse netmask filter: invalid address\n");
                goto error;
                case ERROR_INVALID_NETMASK:
                printf("error: parse netmask filter: invalid netmask\n");
                goto error;                
            }

            struct filter *tmp = realloc(filters, sizeof(struct filter)*(*filters_len+1));
            if (!tmp) goto error;
            filters = tmp;
            (*filters_len)++;

            filters[*filters_len-1] = (struct filter){
                .type = NETMASK_FILTER,
                .netmask = filter,
            };
        } else if (strcmp(token, SIGNATURE_FILTER_STR) == 0) {
            token = strtok(NULL, ":");

            struct signature_filter filter;
            if (parse_signature_filter(token, &filter) < 0) {
                printf("error: signature filter parse error\n");
                goto error;
            }

            struct filter *tmp = realloc(filters, sizeof(struct filter)*(*filters_len+1));
            if (!tmp) goto error;
            filters = tmp;
            (*filters_len)++;

            filters[*filters_len-1] = (struct filter){
                .type = SIGNATURE_FILTER,
                .signature = filter,
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
