#ifndef CACHE_H
#define CACHE_H

#include <stdio.h>
#include "filter.h"

int add_filter_cache(struct filter *f);
struct filter *read_cache(size_t *len);

#endif
