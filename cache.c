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

        struct filter *f = malloc(size);
        if (!f) {
            fclose(fp);
            return NULL;
        }

        *len = size/sizeof(struct filter);
        fread(f, sizeof(struct filter), *len, fp);
        
        fclose(fp);
        return f;
    }    

    return NULL;
}
