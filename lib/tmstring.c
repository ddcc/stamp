#include <string.h>

#include "tmstring.h"

TM_PURE
void *
Pmemcpy(void *dst, const void *src, size_t num) {
    return memcpy(dst, src, num);
}


TM_PURE
int
Pstrncmp(const char *s1, const char *s2, size_t num) {
    return strncmp(s1, s2, num);
}
