#ifndef PARSER_H
#define PARSER_H

#include "../filter.h"

struct filter *read_and_parse(char *path, size_t *filters_len);

#endif
