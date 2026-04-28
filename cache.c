#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>

#include "filter.h"

int add_filter_cache(struct filter *f) {
    FILE *fp = fopen(".filter.cache", "ab");
    if (!fp) return -1;

    int fd = fileno(fp);
    if (flock(fd, LOCK_EX) == 0) {
        fwrite(f, sizeof(struct filter), 1, fp);
        fclose(fp);
    }

    return 0;    
}

struct filter *read_cache(size_t *len) {
    FILE *fp = fopen(".filter.cache", "rb");
    if (!fp) return NULL;

    int fd = fileno(fp);
    if (flock(fd, LOCK_EX) == 0) {
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        struct filter *filters = malloc(size);
        if (!filters) {
            fclose(fp);
            return NULL;
        }

        *len = size/sizeof(struct filter);
        fread(filters, sizeof(struct filter), *len, fp);
        
        fclose(fp);
        return filters;
    }    

    return NULL;
}

int remove_filter_cache(struct filter *f) {
    size_t len = 0;
    struct filter *filters = read_cache(&len);
    if (!filters) return -1;

    for (int i = 0; i < len; i++) {
        if (filters[i].type == f->type) {
            switch (f->type) {
                case PORT_FILTER: {
                    if (filters[i].port.protocol == f->port.protocol && filters[i].port.port == f->port.port) {
                         for (int j = i; j < len; j++) {
                            filters[j] = filters[j+1];
                         }

                         len--;
                         filters = realloc(filters, len*sizeof(struct filter));
                         if (!filters) return -1;

                         goto found;
                    }

                    break;
                }

                case NETMASK_FILTER: {
                    if (filters[i].netmask.address == f->netmask.address && filters[i].netmask.mask == f->netmask.mask) {
                        for (int j = i; j < len; j++) {
                            filters[j] = filters[j+1];
                        }

                        len--;
                        filters = realloc(filters, len*sizeof(struct filter));
                        if (!filters) return -1;

                        goto found;
                    }

                    break;
                }
            }
        }
    }
    free(filters);

    return -1;
    found:
    FILE *fp = fopen(".filter.cache", "wb");
    if (!fp) {
        free(filters);
        return -1;
    }

    int fd = fileno(fp);
    if (flock(fd, LOCK_EX) == 0) {
        fwrite(filters, sizeof(struct filter), len, fp);
    }

    fclose(fp);
    free(filters);

    return 0;
}
