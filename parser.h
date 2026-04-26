#ifndef PARSER_H
#define PARSER_H

#include "filter.h"

uint32_t parse_mask_to_32(char *mask);
char *parse_mask_to_str(uint32_t mask);
uint8_t parse_protocol(char *protocol_str);
uint16_t parse_port(char *port_str);
int parse_port_filter(char *token, struct port_filter *filter);
int parse_netmask_filter(char *token, struct netmask_filter *filter);
struct filter *read_and_parse(char *path, size_t *filters_len);

#endif
